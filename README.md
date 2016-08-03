# 4d-plugin-folder-watch-2
Folder watch 4D plugin for Mac, 2nd edition.

##Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|🆗|🆗|🚫|🚫|

##Features

Watch 1 or more folder paths on the local file system. Any changes (create, delete, update) will trigger your registered callback method. Changes to its subpath is also reported.

###Remarks

Unlike [folder-watch](https://github.com/miyako/4d-plugin-folder-watch) which uses [Grand Central Dispatch (GCD)](https://developer.apple.com/library/ios/documentation/Performance/Reference/GCD_libdispatch_Ref/), this plugin uses [FSEvents](https://developer.apple.com/library/mac/documentation/Darwin/Reference/FSEvents_Ref/index.html#//apple_ref/doc/constant_group/FSEventStreamCreateFlags).

Here is a quick comparision of features:

|  | folder-watch | folder-watch-2 |
| :------------- | :-------------: | :-------------: |
| Watch multiple paths | × | ◎ |
| Watch subfolders | × | ◎ |
| OS X minimum version | 10.7 | 10.6 |

###Commands

```c
FW Set watch path
FW GET WATCH PATHS
FW Set watch method
FW Get watch method
FW Set watch paths
```

###Getting Started

Register a folder path (text) to watch. ``FW Set watch path`` is a convenient shorthand for ``FW Set watch paths`` with 1 element. 

Watching is aborted if one of the following conditions is met:

* An empty string is passed to ``FW Set watch path``
* An empty array (size 0 or 1) is passed to ``FW Set watch paths``

Note that watch paths are not stacked; calling one of the "set" commands will over write the previous settings (except the watch method).

Register a callback method with ``FW Set watch method``. 

The callback method is called with 3 parameters; TEXT, REAL, TEXT.

**Compatibility notice**: Earlier versions passed only 2.

``$1`` is a ``Line feed`` delimited list of paths. 

**Compatibility notice**: Paths are now represented in HFS format; previously they were POSIX.

``$2`` is the unixtime of the change.

``$3`` is a ``Line feed`` delimited list of flags.  

The flags indicate what exactly happend at the path.

###Example Callback

```
  //FOLDER_METHOD_CALLED_ON_EVENT

C_TEXT($1)
C_REAL($2)
C_TEXT($3)

$unixtime:=$2

C_LONGINT($pos;$len)

ARRAY LONGINT($flags;0)
$i:=1
While (Match regex("\\d+";$3;$i;$pos;$len))
APPEND TO ARRAY($flags;Num(Substring($3;$pos;$len)))
$i:=$pos+$len
End while 

ARRAY TEXT($paths;0)
$i:=1
While (Match regex(".+";$1;$i;$pos;$len))
APPEND TO ARRAY($paths;Substring($1;$pos;$len))
$i:=$pos+$len
End while 

ARRAY TEXT($events;0)

For ($i;1;Size of array($flags))

APPEND TO ARRAY($events;"")

If ($flags{$i} & Folder Item created)#0
$events{$i}:=$events{$i}+"create, "
End if 

If ($flags{$i} & Folder Item removed)#0
$events{$i}:=$events{$i}+"remove, "
End if 

If ($flags{$i} & Folder Item renamed)#0
$events{$i}:=$events{$i}+"rename, "
End if 

If ($flags{$i} & Folder Item attribute modified)#0
$events{$i}:=$events{$i}+"attribute modified, "
End if 

If ($flags{$i} & Folder Item info modified)#0
$events{$i}:=$events{$i}+"info modified, "
End if 

If ($flags{$i} & Folder Item meta modified)#0
$events{$i}:=$events{$i}+"meta modified, "
End if 

If ($flags{$i} & Folder Item owner changed)#0
$events{$i}:=$events{$i}+"owner changed, "
End if 

If ($flags{$i} & Folder Item is file)#0
$events{$i}:=$events{$i}+"file, "
End if 

If ($flags{$i} & Folder Item is directory)#0
$events{$i}:=$events{$i}+"folder, "
End if 

If ($flags{$i} & Folder Item is symlink)#0
$events{$i}:=$events{$i}+"symlink, "
End if 

$events{$i}:=Substring($events{$i};1;Length($events{$i})-2)

End for 
```


###Examples

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


