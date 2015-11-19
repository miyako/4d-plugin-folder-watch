/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Folder Watch II
 #	author : miyako
 #	2015/06/16
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

int MONITOR_FOLDER_NOT_FOLDER_ERROR = -1;
int MONITOR_FOLDER_INVALID_PATH_ERROR = -2;
int MONITOR_FOLDER_INVALID_METHOD_NAME_ERROR = -3;

process_number_t MONITOR_FOLDER_METHOD_PROCESS_ID;
process_stack_size_t MONITOR_FOLDER_STACK_SIZE;
process_name_t MONITOR_FOLDER_METHOD_PROCESS_NAME;

method_id_t MONITOR_FOLDER_METHOD_ID;

bool MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE;
bool MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD;

ARRAY_TEXT MONITOR_FOLDER_WATCH_PATH;
ARRAY_TEXT MONITOR_FOLDER_WATCH_PATH_POSIX;
C_TEXT MONITOR_FOLDER_EVENT_PATH;
C_TEXT MONITOR_FOLDER_WATCH_METHOD;
C_TEXT MONITOR_FOLDER_METHOD_PROCESS_NAME_INTERNAL;

#if VERSIONMAC 
NSTimeInterval MONITOR_LATENCY;
FSEventStreamRef MONITOR_STREAM;
void gotEvent(FSEventStreamRef stream, 
                 void *callbackInfo, 
                 size_t numEvents, 
                 void *eventPaths, 
                 const FSEventStreamEventFlags eventFlags[], 
                 const FSEventStreamEventId eventIds[]
                 ) {
                 
    NSArray *paths = (NSArray *)(CFArrayRef)eventPaths;         
    NSMutableString *pathsString = [[NSMutableString alloc]init];
    
    for(uint32_t i = 0; i < [paths count] ; ++i){
        if(i){
            [pathsString appendString:@"\n"];    
        }
        [pathsString appendString:(NSString *)[paths objectAtIndex:i]];
    }
               
    MONITOR_FOLDER_EVENT_PATH.setUTF16String(pathsString);
    [pathsString release];
    
    listenerLoopExecute();
}
#else

#endif 

void generateUuid(C_TEXT &returnValue){

#if VERSIONMAC
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    returnValue.setUTF16String([[[NSUUID UUID]UUIDString]stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#else
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    NSString *uuid_str = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuid);
    returnValue.setUTF16String([uuid_str stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#endif    
#else

#endif
}

#pragma mark -

bool IsProcessOnExit(){    
    C_TEXT name;
    PA_long32 state, time;
    PA_GetProcessInfo(PA_GetCurrentProcessNumber(), name, &state, &time);
    CUTF16String procName(name.getUTF16StringPtr());
    CUTF16String exitProcName((PA_Unichar *)"$\0x\0x\0\0\0");
    return (!procName.compare(exitProcName));
}

void OnStartup(){
    CUTF8String name((const uint8_t *)"$FOLDER_WATCH_II");
    MONITOR_FOLDER_METHOD_PROCESS_NAME_INTERNAL.setUTF8String(&name);
    MONITOR_FOLDER_METHOD_PROCESS_NAME = (process_name_t)MONITOR_FOLDER_METHOD_PROCESS_NAME_INTERNAL.getUTF16StringPtr();
        
    MONITOR_FOLDER_METHOD_PROCESS_ID = 0;
    MONITOR_FOLDER_STACK_SIZE = 0;
    MONITOR_FOLDER_METHOD_ID = 0;
}

void OnCloseProcess(){
    if(IsProcessOnExit()){
        PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL); 
    }
}

#pragma mark -

void listenerLoop(){
    
    MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD = false;
    MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE = false;
        
    while(!MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE)
    { 
        PA_YieldAbsolute();
        
        if(MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD){
            
            MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD = false;
            
            C_TEXT processName;
            generateUuid(processName);
            PA_NewProcess((void *)listenerLoopExecuteMethod, 
                          MONITOR_FOLDER_STACK_SIZE, 
                          (PA_Unichar *)processName.getUTF16StringPtr());        

        }

        if(!MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE){
            PA_FreezeProcess(PA_GetCurrentProcessNumber());  
        }else{
            MONITOR_FOLDER_METHOD_PROCESS_ID = 0;
        }
    }
    PA_KillProcess();
}

void listenerLoopStart(){

    if(!MONITOR_FOLDER_METHOD_PROCESS_ID){
        
        MONITOR_FOLDER_METHOD_PROCESS_ID = PA_NewProcess((void *)listenerLoop, 
                                                         MONITOR_FOLDER_STACK_SIZE, 
                                                         MONITOR_FOLDER_METHOD_PROCESS_NAME);  
        
#if VERSIONMAC  

        uint32_t i, length = MONITOR_FOLDER_WATCH_PATH_POSIX.getSize();
        NSMutableArray *paths = [[NSMutableArray alloc]initWithCapacity:length];
        
        for(i = 0; i < length; ++i){
            NSString *path = MONITOR_FOLDER_WATCH_PATH_POSIX.copyUTF16StringAtIndex(i);
            if([path length]){
                [paths addObject:path];
            }
            [path release];
        } 
        
        FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};
        NSTimeInterval latency = MONITOR_LATENCY;
        
        MONITOR_STREAM = FSEventStreamCreate(NULL,
            (FSEventStreamCallback)gotEvent,
            &context,
            (CFArrayRef)paths,
            kFSEventStreamEventIdSinceNow, 
            (CFAbsoluteTime)latency,
            kFSEventStreamCreateFlagUseCFTypes 
        //  | kFSEventStreamCreateFlagFileEvents 
            | kFSEventStreamCreateFlagNoDefer 
            | kFSEventStreamCreateFlagIgnoreSelf
        );
          
        FSEventStreamScheduleWithRunLoop(MONITOR_STREAM, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamStart(MONITOR_STREAM);
        
        [paths release];
#else
        
#endif         
    }
}

void listenerLoopFinish(){

    if(MONITOR_FOLDER_METHOD_PROCESS_ID){
        //uninstall handler
#if VERSIONMAC  
        FSEventStreamStop(MONITOR_STREAM);
        FSEventStreamUnscheduleFromRunLoop (MONITOR_STREAM, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamInvalidate(MONITOR_STREAM);
        FSEventStreamRelease(MONITOR_STREAM);
        MONITOR_STREAM = 0;
#else
        
#endif 
        //set flags
        MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE = true;
        MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD = false;
        PA_YieldAbsolute();  
        //tell listener to die      
        while(MONITOR_FOLDER_METHOD_PROCESS_ID){
            PA_YieldAbsolute();
            PA_UnfreezeProcess(MONITOR_FOLDER_METHOD_PROCESS_ID);
        }
    }
} 

void listenerLoopExecute(){
    MONITOR_FOLDER_METHOD_PROCESS_SHOULD_TERMINATE = false;
    MONITOR_FOLDER_METHOD_PROCESS_SHOULD_EXECUTE_METHOD = true;
    PA_UnfreezeProcess(MONITOR_FOLDER_METHOD_PROCESS_ID);
}

void listenerLoopExecuteMethod(){
    
    if(MONITOR_FOLDER_METHOD_ID){
        
        PA_Variable	params[1];
        params[0] = PA_CreateVariable(eVK_Unistring);	 
        PA_Unistring path = PA_CreateUnistring((PA_Unichar *)MONITOR_FOLDER_EVENT_PATH.getUTF16StringPtr());
        PA_SetStringVariable(&params[0], &path);
        
        PA_ExecuteMethodByID(MONITOR_FOLDER_METHOD_ID, params, 1);
        
        PA_DisposeUnistring(&path);
        
        PA_ClearVariable(&params[0]);//added 15.11.19
    }
    
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
    
        case kInitPlugin :
        case kServerInitPlugin :            
            OnStartup();
            break;    

        case kCloseProcess :            
            OnCloseProcess();
            break;  
                
// --- Settings

		case 1 :
			FW_Set_watch_path(pResult, pParams);
			break;

		case 2 :
			FW_GET_WATCH_PATHS(pResult, pParams);
			break;

		case 3 :
			FW_Set_watch_method(pResult, pParams);
			break;

		case 4 :
			FW_Get_watch_method(pResult, pParams);
			break;

		case 5 :
			FW_Set_watch_paths(pResult, pParams);
			break;

	}
}

// ----------------------------------- Settings -----------------------------------


void FW_Set_watch_path(sLONG_PTR *pResult, PackagePtr pParams)
{
    C_TEXT Param1;
    C_LONGINT Param2;
	C_LONGINT returnValue;

	Param1.fromParamAtIndex(pParams, 1);
    Param2.fromParamAtIndex(pParams, 2);
    
    if(!Param1.getUTF16Length()){
        
        returnValue.setIntValue(1);
        
        if(MONITOR_FOLDER_WATCH_PATH.getSize() > 1){
            
            MONITOR_FOLDER_WATCH_PATH.setSize(0);
            MONITOR_FOLDER_WATCH_PATH_POSIX.setSize(0);
            
            if(MONITOR_FOLDER_WATCH_METHOD.getUTF16Length()){
                PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);
            } 
        }

    }else{
    
#if VERSIONMAC    
        
        BOOL isDirectory = false;    
        NSString *path = Param1.copyPath();
        NSString *pathHFS = Param1.copyUTF16String();
            
        if([[NSFileManager defaultManager]fileExistsAtPath:path isDirectory:&isDirectory]){
            
            if(isDirectory){
                
                returnValue.setIntValue(1); 
                
                MONITOR_LATENCY = Param2.getIntValue();
                if(MONITOR_LATENCY < 1){
                    MONITOR_LATENCY = 1.0;
                }
                if(MONITOR_LATENCY > 60){
                    MONITOR_LATENCY = 60.0;
                }
            
                MONITOR_FOLDER_WATCH_PATH.setSize(0);
                MONITOR_FOLDER_WATCH_PATH.appendUTF16String(@"");
                MONITOR_FOLDER_WATCH_PATH.appendUTF16String(pathHFS);
                
                MONITOR_FOLDER_WATCH_PATH_POSIX.setSize(0);
                MONITOR_FOLDER_WATCH_PATH_POSIX.appendUTF16String(@"");
                MONITOR_FOLDER_WATCH_PATH_POSIX.appendUTF16String(path);
                
                if(MONITOR_FOLDER_WATCH_METHOD.getUTF16Length()){
                    PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL); 
                    PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopStart, NULL);         
                }
                
            }else{
                returnValue.setIntValue(MONITOR_FOLDER_NOT_FOLDER_ERROR);
            }
            
        }else{
            returnValue.setIntValue(MONITOR_FOLDER_INVALID_PATH_ERROR);
        }
        
        [path release];
        [pathHFS release];
#else
        
#endif    
        
    }
    
	returnValue.setReturn(pResult);

}

void FW_GET_WATCH_PATHS(sLONG_PTR *pResult, PackagePtr pParams)
{
	MONITOR_FOLDER_WATCH_PATH.toParamAtIndex(pParams, 1);
}

void FW_Set_watch_method(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_LONGINT returnValue;

	Param1.fromParamAtIndex(pParams, 1);

    if(!Param1.getUTF16Length()){
        
        returnValue.setIntValue(1);
        
        if(MONITOR_FOLDER_WATCH_METHOD.getUTF16Length()){
            
            MONITOR_FOLDER_WATCH_METHOD.setUTF16String(Param1.getUTF16StringPtr(), Param1.getUTF16Length());
            MONITOR_FOLDER_METHOD_ID = 0;
            
            if(MONITOR_FOLDER_WATCH_PATH.getSize() > 1){
                PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);  
            }            
        }

    }else{  
        
        method_id_t methodId = PA_GetMethodID((PA_Unichar *)Param1.getUTF16StringPtr());
        
        if(methodId){
            
            returnValue.setIntValue(1);
            
            if(methodId != MONITOR_FOLDER_METHOD_ID){
                
                MONITOR_FOLDER_WATCH_METHOD.setUTF16String(Param1.getUTF16StringPtr(), Param1.getUTF16Length());
                MONITOR_FOLDER_METHOD_ID = methodId;
                
                if(MONITOR_FOLDER_WATCH_PATH.getSize() > 1){
                    PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);  
                    PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopStart, NULL);                  
                }

            }
            
        }else{
            
            returnValue.setIntValue(MONITOR_FOLDER_INVALID_METHOD_NAME_ERROR);
            
        }
      
    }
  
	returnValue.setReturn(pResult);
}

void FW_Get_watch_method(sLONG_PTR *pResult, PackagePtr pParams)
{
	MONITOR_FOLDER_WATCH_METHOD.setReturn(pResult);
}

void FW_Set_watch_paths(sLONG_PTR *pResult, PackagePtr pParams)
{
	ARRAY_TEXT Param1;
    C_LONGINT Param2;
	C_LONGINT returnValue;
    
	Param1.fromParamAtIndex(pParams, 1);
    Param2.fromParamAtIndex(pParams, 2);
    
    if(!Param1.getSize()){
        
        returnValue.setIntValue(1);
        
        if(MONITOR_FOLDER_WATCH_PATH.getSize() > 1){
            
            MONITOR_FOLDER_WATCH_PATH.setSize(0);
            MONITOR_FOLDER_WATCH_PATH_POSIX.setSize(0);
            
            if(MONITOR_FOLDER_WATCH_METHOD.getUTF16Length()){
                PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL);
            } 
        }

    }else{
    
#if VERSIONMAC    

        uint32_t i, length = Param1.getSize();
        
        MONITOR_FOLDER_WATCH_PATH.setSize(0);
        MONITOR_FOLDER_WATCH_PATH_POSIX.setSize(0);

        MONITOR_FOLDER_WATCH_PATH.appendUTF16String(@"");
        MONITOR_FOLDER_WATCH_PATH_POSIX.appendUTF16String(@""); 
               
        for(i = 0; i < length; ++i){
        
            NSString *path = Param1.copyPathAtIndex(i);
            NSString *pathHFS = Param1.copyUTF16StringAtIndex(i);
            
            BOOL isDirectory = false;
            
            if([[NSFileManager defaultManager]fileExistsAtPath:path isDirectory:&isDirectory]){
                
                if(isDirectory){
                
                    MONITOR_FOLDER_WATCH_PATH.appendUTF16String(pathHFS);
                    MONITOR_FOLDER_WATCH_PATH_POSIX.appendUTF16String(path);
                                        
                }else{
                    returnValue.setIntValue(MONITOR_FOLDER_NOT_FOLDER_ERROR);
                }
                
            }else{
                returnValue.setIntValue(MONITOR_FOLDER_INVALID_PATH_ERROR);
            }            
            
            [pathHFS release];            
            [path release];

        }
        
        if(MONITOR_FOLDER_WATCH_PATH.getSize() > 1){
            returnValue.setIntValue(1);
            MONITOR_LATENCY = Param2.getIntValue();
            if(MONITOR_LATENCY < 1){
                MONITOR_LATENCY = 1.0;
            }
            if(MONITOR_LATENCY > 60){
                MONITOR_LATENCY = 60.0;
            }
            if(MONITOR_FOLDER_WATCH_METHOD.getUTF16Length()){
                PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopFinish, NULL); 
                PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopStart, NULL);         
            }        
        }                        

#else
        
#endif    
        
    }
 
	returnValue.setReturn(pResult);
}

