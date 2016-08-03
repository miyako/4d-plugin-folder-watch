# 4d-plugin-folder-watch-2
Folder watch 4D plugin for Mac, 2nd edition.

##Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|🆗|🆗|🚫|🚫|

###Remarks

Unlike [folder-watch](https://github.com/miyako/4d-plugin-folder-watch) which uses [Grand Central Dispatch (GCD)](https://developer.apple.com/library/ios/documentation/Performance/Reference/GCD_libdispatch_Ref/), this plugin uses [FSEvents](https://developer.apple.com/library/mac/documentation/Darwin/Reference/FSEvents_Ref/index.html#//apple_ref/doc/constant_group/FSEventStreamCreateFlags).

Here is a quick comparision of features:

|  | folder-watch | folder-watch-2 |
| :------------- | :-------------: | :-------------: |
| Watch multiple paths | × | ◎ |
| Watch subfolders | × | ◎ |
| OS X minimum version | 10.7 | 10.6 |

Commands
---

```c
FW_Set watch path
FW_GET WATCH PATHS
FW_Set watch method
FW_Get watch method
FW_Set watch paths
```

Examples
---

```
FW GET WATCH PATHS ($folders)
If (Size of array($folders)=0)
$folder:=System folder(Desktop)
Else 
$folder:=$folders{1}
End if 

$folder:=Select folder("Where do you want to watch?";$folder;Use sheet window)

If (OK=1)

APPEND TO ARRAY($folders;$folder)
$success:=FW Set watch paths ($folders)
$success:=FW Set watch method ("FOLDER_METHOD_CALLED_ON_EVENT")

End if 
```

In ``$2`` to ``FW Set watch paths`` or ``FW Set watch path``, you can pass an optional ``latency``, which is the number of seconds to wait before an event is sent. By default the latency is 1.0, the maximum is 60.0. A longer latency may contribute to better efficiency.

```
  //FOLDER_METHOD_CALLED_ON_EVENT

C_TEXT($1)
C_REAL($2)

$watchpaths:=$1
$unixtime:=$2

$i:=1

ARRAY LONGINT($pos;0)
ARRAY LONGINT($len;0)

ARRAY TEXT(<>DOCUMENTS;0)

While (Match regex("(.+)";$watchpaths;$i;$pos;$len))
APPEND TO ARRAY(<>DOCUMENTS;Substring($watchpaths;$pos{1};$len{1}))
$i:=$pos{1}+$len{1}
End while 

CALL PROCESS(-1)
```

Callback method will receive in ``$1`` the path where a change was detected. If multiple paths are passed, they will be delimiter by 0x0A (line feed).

**Compatibility notice**: Paths are now represented in HFS format; previously they were POSIX.
