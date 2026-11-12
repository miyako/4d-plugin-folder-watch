/*
 * Synthetic 4D engine boundary for Folder Watch ownership regressions.
 *
 * Links the real plugin source (4DPlugin.cpp) and the bundled, unmodified
 * 4D Plugin API (4DPluginAPI.c + Classes). Only gCall4D is replaced: it
 * models engine-side allocations (Unistrings, handles) with malloc/free so
 * ASan reports double frees and use-after-free, and keeps a live set of
 * every engine string so leaks are detected exactly.
 *
 * The FSEvents boundary is stubbed: no FSEventStream is ever created and no
 * folder is watched. Tests call the plugin's real FSEvents callback
 * (gotEvent) with a synthetic CFArray of POSIX paths, exactly as FSEvents
 * would with kFSEventStreamCreateFlagUseCFTypes, then dispatch the queued
 * event with the plugin's real listenerLoopExecuteMethod. The paths are
 * never touched on disk. The Windows ReadDirectoryChangesW thread is not
 * compiled here (it feeds the same listenerLoopExecuteMethod).
 *
 * Engine semantics modelled (SDK contract):
 *  - PA_CreateVariable(eVK_Unistring) allocates an empty engine string
 *    (4DPluginAPI.c); PA_SetStringVariable overwrites the variable shallowly.
 *  - PA_ExecuteMethodByID / EXECUTE METHOD (command 1007) read their
 *    parameters; the caller keeps ownership and clears them afterwards.
 *  - PA_ExecuteMethodByID returns a caller-owned result (here: the callback
 *    method's text $0) that must be cleared when the call succeeded; on
 *    error the result is not initialised (poisoned) and must not be cleared.
 *  - PA_ClearVariable releases what the variable owns.
 */

#include "4DPluginAPI.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include "4DPlugin.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

extern "C" Call4DProcPtr gCall4D;

namespace FW2 { extern ARRAY_TEXT WATCH_PATHS; }

namespace {

typedef std::basic_string<PA_Unichar> u16;

struct Handle { char *data; PA_long32 len; };

std::set<void *> g_strings;
std::set<Handle *> g_handles;

const PA_long32 kCallbackId = 4242;
const u16 kCallbackName = (const PA_Unichar *)u"FW_CALLBACK";
bool g_return_text = false;
bool g_fail_call = false;
void *g_returned = 0;

struct Call { int calls = 0; u16 method; u16 path; double ts = 0; PA_long32 flags = 0; };
Call g_byid, g_cmd;

PA_Unichar kPoisonBuf[] = { 'B', 'A', 'D', '!', 0 };
PA_Unichar *const kPoison = kPoisonBuf;

[[noreturn]] void die(const char *msg) {
    fprintf(stderr, "ENGINE VIOLATION: %s\n", msg);
    fflush(stderr);
    abort();
}

PA_Unistring new_string(const PA_Unichar *src, size_t len) {
    PA_Unichar *buf = (PA_Unichar *)malloc((len + 1) * sizeof(PA_Unichar));
    if(len) memcpy(buf, src, len * sizeof(PA_Unichar));
    buf[len] = 0;
    g_strings.insert(buf);
    PA_Unistring s;
    s.fLength = (PA_long32)len;
    s.fString = buf;
    s.fReserved1 = 0;
    s.fReserved2 = 0;
    return s;
}

void free_string(PA_Unistring *s) {
    if(!s->fString) return;
    if(s->fString == kPoison) die("cleared the result of a failed method call");
    if(!g_strings.erase(s->fString)) die("disposed a string the engine does not own");
    free(s->fString);
    s->fString = 0;
    s->fLength = 0;
}

Handle *new_handle(PA_long32 len) {
    Handle *h = (Handle *)malloc(sizeof(Handle));
    h->data = (char *)calloc(1, len ? len : 1);
    h->len = len;
    g_handles.insert(h);
    return h;
}

Handle *checked(PA_Handle ph) {
    Handle *h = (Handle *)ph;
    if(!g_handles.count(h)) die("used an unknown handle");
    return h;
}

void free_handle(Handle *h) {
    if(!h) return;
    checked((PA_Handle)h);
    g_handles.erase(h);
    free(h->data);
    free(h);
}

void clear_var(PA_Variable *v) {
    switch(v->fType) {
        case eVK_Unistring: free_string(&v->uValue.fString); break;
        case eVK_ArrayUnicode: {
            Handle *h = (Handle *)v->uValue.fArray.fData;
            if(h) {
                PA_Unistring *items = (PA_Unistring *)checked((PA_Handle)h)->data;
                for(PA_long32 i = 0; i <= v->uValue.fArray.fNbElements; ++i) free_string(&items[i]);
                free_handle(h);
            }
            break;
        }
        default: break;
    }
    v->fType = eVK_Undefined;
}

u16 live_text(const PA_Variable &v, const char *what) {
    if(v.fType != eVK_Unistring) die(what);
    if(!g_strings.count(v.uValue.fString.fString)) die("engine received a string it does not own");
    return u16(v.uValue.fString.fString, v.uValue.fString.fLength);
}

void record(Call &c, const PA_Variable *path, const PA_Variable *ts, const PA_Variable *flags) {
    c.calls++;
    c.path = live_text(*path, "path parameter is not text");
    if(ts->fType != eVK_Real) die("timestamp parameter is not real");
    if(flags->fType != eVK_Longint) die("flags parameter is not longint");
    c.ts = ts->uValue.fReal;
    c.flags = flags->uValue.fLongint;
}

void engine(PA_long32 selector, void *p) {
    EngineBlock *eb = (EngineBlock *)p;
    switch(selector) {
        case EX_HANDLE_MANAGER:
            switch(eb->fParam1) {
                case 1: eb->fHandle = (PA_Handle)new_handle((PA_long32)eb->fParam2); eb->fError = 0; return;
                case 2: free_handle((Handle *)eb->fHandle); eb->fError = 0; return;
                case 3: {
                    Handle *h = checked(eb->fHandle);
                    PA_long32 n = (PA_long32)eb->fParam2;
                    h->data = (char *)realloc(h->data, n ? n : 1);
                    if(n > h->len) memset(h->data + h->len, 0, n - h->len);
                    h->len = n; eb->fError = 0; return;
                }
                case 4: eb->fParam2 = checked(eb->fHandle)->len; eb->fError = 0; return;
                case 5: eb->fParam3 = (sLONG_PTR)checked(eb->fHandle)->data; eb->fError = 0; return;
                case 6: checked(eb->fHandle); eb->fError = 0; return;
                default: die("unmodelled handle manager call");
            }
        case EX_CREATE_UNISTRING: {
            const PA_Unichar *s = (const PA_Unichar *)eb->fHandle;
            size_t n = 0; while(s[n]) ++n;
            eb->fUniString1 = new_string(s, n);
            return;
        }
        case EX_DISPOSE_UNISTRING: free_string(&eb->fUniString1); return;
        case EX_CLEAR_VARIABLE: clear_var((PA_Variable *)eb->fPtr1); return;
        case EX_GET_METHOD_ID:
            eb->fParam1 = (u16(eb->fUName) == kCallbackName) ? kCallbackId : 0;
            eb->fError = 0;
            return;
        case EX_CALL_BY_PROCID: {
            if(eb->fParam1 != kCallbackId || eb->fParam2 != 3) die("unexpected method call");
            PA_Variable *v = (PA_Variable *)checked(eb->fHandle)->data;
            record(g_byid, &v[1], &v[2], &v[3]);
            if(g_fail_call) {
                v[0].fType = eVK_Unistring;
                v[0].uValue.fString.fString = kPoison;
                v[0].uValue.fString.fLength = 4;
                eb->fError = -1;
                return;
            }
            if(g_return_text) {
                static const PA_Unichar ret[] = { 'o', 'k' };
                v[0].fType = eVK_Unistring;
                v[0].uValue.fString = new_string(ret, 2);
                g_returned = v[0].uValue.fString.fString;
            }
            eb->fError = 0;
            return;
        }
        case EX_EXECUTE_COMMAND_BY_ID: {
            if(eb->fParam1 != 1007 || eb->fParam2 != 4) die("unexpected command");
            PA_Variable **v = (PA_Variable **)eb->fPtr1;
            g_cmd.method = live_text(*v[0], "method name parameter is not text");
            record(g_cmd, v[1], v[2], v[3]);
            eb->fError = 0;
            return;
        }
        default: {
            char msg[64];
            snprintf(msg, sizeof msg, "unmodelled selector %d", (int)selector);
            die(msg);
        }
    }
}

void expect(bool ok, const char *what) {
    if(!ok) { fprintf(stderr, "FAIL: %s (live engine strings: %zu)\n", what, g_strings.size()); exit(1); }
}

size_t live_strings() { return g_strings.size(); }

void set_watch_method(const PA_Unichar *name) {
    size_t n = 0; while(name[n]) ++n;
    PA_Unistring arg = new_string(name, n);
    void *params[1] = { &arg };
    PA_long32 result = 0;
    PluginBlock pb = { params, &result, 0 };
    PluginMain(3, &pb); /* FW Set watch method */
    expect(result == 1, "FW Set watch method returned 1");
    free_string(&arg);
}

/* Feed one synthetic FSEvents notification through the plugin's real callback. */
void synthetic_fsevent(FSEventStreamEventFlags flags) {
    CFStringRef path = CFSTR("/fw-synthetic/does-not-exist/a.txt");
    CFArrayRef paths = CFArrayCreate(kCFAllocatorDefault, (const void **)&path, 1, &kCFTypeArrayCallBacks);
    FSEventStreamEventFlags f[1] = { flags };
    FSEventStreamEventId ids[1] = { 1 };
    gotEvent(0, 0, 1, (void *)paths, f, ids);
    CFRelease(paths);
}

const FSEventStreamEventFlags kFlags = kFSEventStreamEventFlagItemCreated | kFSEventStreamEventFlagItemIsFile;

bool ends_with(const u16 &s, const char *suffix) {
    u16 t; for(const char *c = suffix; *c; ++c) t.push_back((PA_Unichar)*c);
    return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}

void dispatch_one(const PA_Unichar *method) {
    set_watch_method(method);
    expect(live_strings() == 0, "no engine strings live before dispatch");
    synthetic_fsevent(kFlags);
    listenerLoopExecuteMethod();
}

/* ---- tests ---- */

void defect_byid_param_owner_leak() {
    dispatch_one(kCallbackName.c_str());
    expect(g_byid.calls == 1, "callback called once via PA_ExecuteMethodByID");
    expect(live_strings() == 0, "no engine strings leaked by the ByID callback parameters");
}

void defect_1007_param_owner_leak() {
    dispatch_one((const PA_Unichar *)u"FW_UNRESOLVED");
    expect(g_cmd.calls == 1, "callback called once via EXECUTE METHOD");
    expect(live_strings() == 0, "no engine strings leaked by the EXECUTE METHOD parameters");
}

void defect_byid_result_not_cleared() {
    g_return_text = true;
    dispatch_one(kCallbackName.c_str());
    expect(g_byid.calls == 1, "callback called once");
    expect(g_returned != 0, "callback returned a text result");
    expect(!g_strings.count(g_returned), "callback result released");
}

void guard_byid_error_result_untouched() {
    g_fail_call = true;
    dispatch_one(kCallbackName.c_str());
    expect(g_byid.calls == 1, "callback attempted once");
    /* Clearing the poisoned result would have aborted with ENGINE VIOLATION.
       Only the (pre-existing, separately tested) empty owner may remain. */
    for(void *s : g_strings)
        expect(((PA_Unichar *)s)[0] == 0, "path parameter released after a failed call");
}

void control_byid_args_delivered() {
    dispatch_one(kCallbackName.c_str());
    expect(g_byid.calls == 1 && g_cmd.calls == 0, "ByID path taken");
    expect(ends_with(g_byid.path, "a.txt"), "path delivered");
    expect(g_byid.ts > 0, "timestamp delivered");
    expect(g_byid.flags == (PA_long32)kFlags, "flags delivered");
}

void control_1007_args_delivered() {
    dispatch_one((const PA_Unichar *)u"FW_UNRESOLVED");
    expect(g_cmd.calls == 1 && g_byid.calls == 0, "EXECUTE METHOD path taken");
    expect(g_cmd.method == u16((const PA_Unichar *)u"FW_UNRESOLVED"), "method name delivered");
    expect(ends_with(g_cmd.path, "a.txt"), "path delivered");
    expect(g_cmd.flags == (PA_long32)kFlags, "flags delivered");
}

/* Contract 1/2 control: FW GET WATCH PATHS fills a text array via
   PA_SetStringInArray and must not dispose the transferred strings. */
void control_get_watch_paths_array() {
    FW2::WATCH_PATHS.setSize(0);
    FW2::WATCH_PATHS.appendUTF16String((const PA_Unichar *)u"", 0);
    FW2::WATCH_PATHS.appendUTF16String((const PA_Unichar *)u"Disk:a:", 7);
    FW2::WATCH_PATHS.appendUTF16String((const PA_Unichar *)u"Disk:b:", 7);
    PA_Variable arr = PA_CreateVariable(eVK_ArrayUnicode);
    void *params[1] = { &arr };
    PluginBlock pb = { params, 0, 0 };
    PluginMain(2, &pb); /* FW GET WATCH PATHS */
    expect(arr.fType == eVK_ArrayUnicode && arr.uValue.fArray.fNbElements == 2, "two paths returned");
    PA_Unistring *items = (PA_Unistring *)checked(arr.uValue.fArray.fData)->data;
    expect(u16(items[2].fString, items[2].fLength) == u16((const PA_Unichar *)u"Disk:b:"), "element 2 delivered");
    expect(live_strings() == 3, "three array strings owned by the array");
    PA_ClearVariable(&arr);
    FW2::WATCH_PATHS.setSize(0);
    expect(live_strings() == 0, "array strings released with the array");
}

const std::map<std::string, std::function<void()>> kTests = {
    { "defect_byid_param_owner_leak", defect_byid_param_owner_leak },
    { "defect_1007_param_owner_leak", defect_1007_param_owner_leak },
    { "defect_byid_result_not_cleared", defect_byid_result_not_cleared },
    { "guard_byid_error_result_untouched", guard_byid_error_result_untouched },
    { "control_byid_args_delivered", control_byid_args_delivered },
    { "control_1007_args_delivered", control_1007_args_delivered },
    { "control_get_watch_paths_array", control_get_watch_paths_array },
};

} // namespace

int main(int argc, char **argv) {
    gCall4D = (Call4DProcPtr)engine;
    if(argc == 2 && std::string(argv[1]) == "--list") {
        for(auto &t : kTests) printf("%s\n", t.first.c_str());
        return 0;
    }
    if(argc != 2 || !kTests.count(argv[1])) { fprintf(stderr, "usage: %s --list | <test>\n", argv[0]); return 2; }
    kTests.at(argv[1])();
    printf("PASS %s\n", argv[1]);
    return 0;
}
