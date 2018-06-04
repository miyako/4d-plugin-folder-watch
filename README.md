# 4d-plugin-folder-watch-2
Folder watch 4D plugin for Mac, 2nd edition.

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|||

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940649/21945000-8645-11e6-86ed-4a0f800e5a73.png" width="32" height="32" /> <img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" />

### Releases 

[2.0](https://github.com/miyako/4d-plugin-folder-watch-2/releases/tag/2.0)

### Features

Watch 1 or more folder paths on the local file system. Any changes (create, delete, update) will trigger your registered callback method. Changes to its subpath is also reported.

### Getting Started

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

List of flags 

```c
Folder Item created 256
Folder Item removed 512
Folder Item meta modified 1024
Folder Item renamed 2048
Folder Item modified 4096
Folder Item info modified 8192
Folder Item owner changed 16384
Folder Item attribute modified 32768
Folder Item is file 65536
Folder Item is directory 131072
Folder Item is symlink 262144
```

**Note**: Trashing an item qualifies as a rename event, as it is not really removed at the system level.

### Example Callback

```
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
```

### Examples

```
$success:=FW Set watch path (System folder(Desktop))
$success:=FW Set watch method ("FOLDER_METHOD_CALLED_ON_EVENT")
```

In ``$2`` to ``FW Set watch paths`` or ``FW Set watch path``, you can pass an optional ``latency``, which is the number of seconds to wait before an event is sent. By default the latency is 1.0, the maximum is 60.0. A longer latency may contribute to better efficiency.


