![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-32%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-folder-watch)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-folder-watch/total)

# 4d-plugin-folder-watch

Folder Watch lets a 4D application monitor one or more folders on the local file system and receive a callback every time something inside them is created, deleted, or updated (including changes to subpaths). On macOS this is driven by `FSEventStreamRef` (FSEvents); on Windows it's driven by `ReadDirectoryChangesW` on a dedicated thread per watched folder. Every reported change delivers a path, a Unix timestamp, and a bitmask of flags describing exactly what happened.

## Commands

| Command | Returns | Purpose |
|---|---|---|
| [FW Set watch path](#fw-set-watch-path) | Longint | Watch a single folder |
| [FW GET WATCH PATHS](#fw-get-watch-paths) | — | Get the folder(s) currently being watched |
| [FW Set watch method](#fw-set-watch-method) | Longint | Register the project method called on each event |
| [FW Get watch method](#fw-get-watch-method) | Text | Get the name of the currently registered method |
| [FW Set watch paths](#fw-set-watch-paths) | Longint | Watch one or more folders at once |

**Platforms:** macOS (Intel & Apple Silicon), Windows (32- and 64-bit) — 4D v18+. On 4D v17, move `manifest.json` up one level from `/RESOURCES/`.

---

## Requirements & platform notes

- **Changing watched paths while a monitor is running behaves differently per platform.** On **macOS**, you can call `FW Set watch path`/`FW Set watch paths` again at any time to change what's watched — the existing FSEvents stream is torn down and a new one started for you. On **Windows**, calling either setter while a monitor is already running **cancels the running monitor and ignores the paths you just passed** — you need to call the command a second time to actually start watching the new paths.
- **`latency` only affects macOS.** Both setter commands accept an optional `latency` parameter (seconds), clamped to the range 1–60 (values below 1, including an omitted parameter, are raised to 1.0). It's passed straight through to FSEvents' coalescing window on macOS. On Windows the value is still validated and stored, but nothing in the Windows code path reads it back — it has no effect on timing there.
- **An empty path (or an effectively empty array) stops the monitor rather than erroring.** Passing `""` to `FW Set watch path`, or an array of size 0 (or size 1 containing only an empty string) to `FW Set watch paths`, cancels any running monitor and returns `1` (success) — this is the documented way to stop watching, not a failure case.
- **`FW GET WATCH PATHS`'s array always has a blank first element.** The array this command fills always reserves element 1 as an empty string; the actual watched path(s) start at element 2. This is an existing quirk of how the internal path list is built (both setters insert a blank placeholder before the real paths), not a bug you need to work around defensively — just don't mistake element 1 for a real watched path.
- **A batch call to `FW Set watch paths` still watches whichever paths were valid, even if others weren't.** If some entries in the array aren't valid folders, those entries are skipped, the valid ones are watched, and the command returns the error code for the invalid entry rather than `1` — check the return value if you need to know whether every path in the batch was accepted.
- **Rename events fire twice** (once for the old name, once for the new one) on both platforms.
- **On Windows, only a subset of the flags below is ever reported** (created, removed, renamed, modified, is-file, is-directory) — the metadata/owner/attribute/symlink flags are macOS-only, because they come straight from FSEvents.
- **On Windows, there's no way to know whether a removed path was a file or a folder** — the is-file/is-directory flags are only set for non-removal events.
- **On macOS, moving an item to the Trash is reported as a rename**, since the item isn't actually removed at the file-system level. **On Windows**, restoring an item from the Trash, or moving an item into/out of a watched folder, generates create/modify events, and moving an item *between* two watched folders can generate misleading events.
- **macOS file-system access.** FSEvents watching a folder outside your application's own sandbox (e.g. the user's Desktop or Documents) may require the host application to have the appropriate file-access permission granted by the user, depending on how 4D itself is packaged/entitled on the machine it runs on — this isn't something the plugin can configure for you.

---

## FW Set watch path

### Syntax

```4d
FW Set watch path ( path ; latency ) → Longint
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | Folder to watch. Pass an empty string to stop the currently running monitor. |
| `latency` | Longint | Seconds to wait before an event is delivered (1–60). Optional — an omitted or out-of-range value is clamped to 1. macOS only; ignored on Windows (see Requirements above). |
| Result | Longint | `1` on success (including the "stop watching" case). `-1` if `path` exists but isn't a folder. `-2` if `path` doesn't exist. `-4` if an unexpected internal error occurred. |

### Description

`FW Set watch path` is shorthand for [`FW Set watch paths`](#fw-set-watch-paths) with a single-element array. It replaces whatever was previously being watched — it does not add a second folder alongside an existing one.

Paths are represented in HFS format on macOS internally; you can pass either a POSIX or HFS-style path in `path` and the plugin resolves it against the file system either way, but the path handed back to your callback method is HFS-formatted on macOS.

**On Windows**, if a monitor is already running, calling this command cancels it and ignores the `path` you passed in that same call — call it again to actually start watching the new folder.

### Example

```4d
$success:=FW Set watch path (System folder(Desktop))
$success:=FW Set watch method ("FOLDER_METHOD_CALLED_ON_EVENT")
```
*(From the plugin's own README usage example.)*

```4d
// watch with an explicit 5-second latency
$success:=FW Set watch path ("/Users/me/Documents/Inbox";5)
```

```4d
// stop watching
$success:=FW Set watch path ("")
```

---

## FW GET WATCH PATHS

### Syntax

```4d
FW GET WATCH PATHS ( arrPaths )
```

| Parameter | Type | Description |
|---|---|---|
| `arrPaths` | Array (Text), by reference | Filled with the folder(s) currently being watched. |

This command has no function result.

### Description

`arrPaths`'s first element is always an empty string (an internal placeholder); the actual watched path(s) start at element 2. If nothing is currently being watched, the array will just contain that one blank element.

### Example

```4d
ARRAY TEXT($paths;0)
FW GET WATCH PATHS ($paths)
For ($i;2;Size of array($paths))
	ALERT($paths{$i})
End for
```

---

## FW Set watch method

### Syntax

```4d
FW Set watch method ( methodName ) → Longint
```

| Parameter | Type | Description |
|---|---|---|
| `methodName` | Text | Name of the project method to call for every watch event. Pass an empty string to clear the registered method. |
| Result | Longint | `1` on success. `-4` if an unexpected internal error occurred. |

### Description

The registered method is called once per reported change, with 3 parameters:

```4d
C_TEXT($1;$path)
C_REAL($2;$unixtime)
C_LONGINT($3;$flags)
```

| Parameter | Type | Description |
|---|---|---|
| `$1` | Text | The affected path (HFS-formatted on macOS). |
| `$2` | Real | Unix timestamp of the change. |
| `$3` | Longint | Bitmask describing what happened — see the flag table under [FW Set watch paths](#fw-set-watch-paths) below. |

### Example

```4d
C_TEXT($1;$path)
C_REAL($2;$unixtime)
C_LONGINT($3;$flags)

If ($3 & 256)
	 // item created
End if
```

---

## FW Get watch method

### Syntax

```4d
FW Get watch method → Text
```

| Parameter | Type | Description |
|---|---|---|
| Result | Text | The currently registered method name, or an empty string if none is set. |

### Example

```4d
$currentMethod:=FW Get watch method
If ($currentMethod="")
	 // no method registered yet
End if
```

---

## FW Set watch paths

### Syntax

```4d
FW Set watch paths ( arrPaths ; latency ) → Longint
```

| Parameter | Type | Description |
|---|---|---|
| `arrPaths` | Array (Text), by reference | Folders to watch. Pass an array of size 0, or size 1 containing only an empty string, to stop the currently running monitor. |
| `latency` | Longint | Same as `FW Set watch path`'s `latency` — 1–60 seconds, macOS only, ignored on Windows. |
| Result | Longint | `1` if every path in the array was valid and is now being watched. `-1`/`-2` if at least one path was invalid (folder is skipped; the other valid paths are still watched). `-4` if an unexpected internal error occurred. |

### Description

Replaces whatever was previously being watched with the array of folders passed in. Any entry that doesn't exist or isn't a folder is dropped from the list (the rest are still watched), and the return value reflects that at least one entry failed — check it if your code needs to know whether the whole batch succeeded.

**On Windows**, if a monitor is already running, calling this command cancels it and ignores the paths you just passed — call it again to start watching the new set.

**Event flags** (the `$3` parameter delivered to your watch method):

| Flag | Value | Meaning |
|---|---|---|
| Folder item created | 256 | |
| Folder item removed | 512 | |
| Folder item meta modified | 1024 | macOS only |
| Folder item renamed | 2048 | fires twice — once per name |
| Folder item modified | 4096 | |
| Folder item info modified | 8192 | macOS only |
| Folder item owner changed | 16384 | macOS only |
| Folder item attribute modified | 32768 | macOS only |
| Folder item is file | 65536 | not set on Windows removal events |
| Folder item is directory | 131072 | not set on Windows removal events |
| Folder item is symlink | 262144 | macOS only |

### Example

```4d
ARRAY TEXT($paths;2)
$paths{1}:=System folder(Desktop)
$paths{2}:=System folder(Documents)
$success:=FW Set watch paths ($paths;1)
```

```4d
// stop watching
ARRAY TEXT($paths;0)
$success:=FW Set watch paths ($paths;1)
```

---

## Error handling & troubleshooting

- **`-1`/`-2` returns from either setter mean the path didn't validate**, not that the command failed outright — `-1` means the path exists but isn't a folder, `-2` means it doesn't exist at all. With `FW Set watch paths`, any other valid paths in the same array are still watched despite this return code.
- **`-4` means an unexpected internal error was caught rather than crashing the host** — this shouldn't come up in normal use; if you see it, it's worth reporting along with what you passed in.
- **Silent no-op above 64 folders.** `FW Set watch paths` is limited to as many folders as `WaitForMultipleObjects` can wait on at once on Windows (64). Passing more than that doesn't return an error — the monitor simply doesn't start. Keep batches at or below 64 folders.
- **Changing paths mid-run behaves differently per platform** — see the Requirements section above; on Windows you must call the setter twice (once to cancel, once to actually apply the new paths).
- **`latency` has no effect on Windows** — don't rely on it to tune event timing there; it only matters on macOS.
- **A registered watch method that no longer exists (renamed/deleted project method) won't raise an error from these commands** — the failure would surface at the point 4D tries to call it, not from `FW Set watch method` itself.
- **Rename events always arrive as a pair** — expect two calls to your method (old name, then new name) for a single rename, on both platforms.
- **On Windows, you can't tell whether a *removed* path was a file or a folder** — the is-file/is-directory flags aren't available for removal events there.

---

## Quick reference

```4d
// watch a single folder
$success:=FW Set watch path (System folder(Desktop);1)
$success:=FW Set watch method ("FOLDER_METHOD_CALLED_ON_EVENT")

// watch several folders at once
ARRAY TEXT($paths;2)
$paths{1}:=System folder(Desktop)
$paths{2}:=System folder(Documents)
$success:=FW Set watch paths ($paths;1)

// inspect what's currently watched
ARRAY TEXT($current;0)
FW GET WATCH PATHS ($current)

// stop watching
$success:=FW Set watch path ("")

// callback method (e.g. "FOLDER_METHOD_CALLED_ON_EVENT")
C_TEXT($1;$path)
C_REAL($2;$unixtime)
C_LONGINT($3;$flags)
If ($3 & 512)
	 // item removed
End if
```
