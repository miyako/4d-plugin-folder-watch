# 4d-plugin-folder-watch
Folder watch 4D plugin for Mac and Windows

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940649/21945000-8645-11e6-86ed-4a0f800e5a73.png" width="32" height="32" /> <img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" /> <img src="https://user-images.githubusercontent.com/1725068/41266195-ddf767b2-6e30-11e8-9d6b-2adf6a9f57a5.png" width="32" height="32" />

### Releases 

[3.1](https://github.com/miyako/4d-plugin-folder-watch/releases/tag/3.1)

![preemption xx](https://user-images.githubusercontent.com/1725068/41327179-4e839948-6efd-11e8-982b-a670d511e04f.png)

except 

* ``FW Set watch path``

* ``FW Set watch paths``

### Features

Watch 1 or more folder paths on the local file system. Any changes (create, delete, update) will trigger your registered callback method. Changes to its subpath are also reported.

### Getting Started

Register a folder path (text) to watch. ``FW Set watch path`` is a convenient shorthand for ``FW Set watch paths`` with 1 element. 

Watching is aborted if one of the following conditions is met:

* An empty string is passed to ``FW Set watch path``
* An empty array (size 0 or 1) is passed to ``FW Set watch paths``

#### Platform considerations

On Mac, you can change the paths to watch while the monitor is running.

On Windows, you must first cancel the running monitor. Calling ``FW Set watch path`` or ``FW Set watch paths`` while a monitor is running will automatically cancel it (the paths are ignored). In effect, you need to call the commands twice.

Register a callback method with ``FW Set watch method``. 

The callback method is called with 3 parameters; TEXT, REAL, **LONGINT**.

**Compatibility notice** v2 to v3: 

The 1st parameter has been changed to return a single path. Previously, it was a ``\n`` separated list.

The 3rd parameter has been changed from TEXT to LONGINT and returns a single flag. Previously, it was a comma separated list.

**Compatibility notice** v1 to v2: 

Paths are now represented in HFS format; previously they were POSIX.

``$2`` is the unixtime of the change.

``$3`` is a ``Line feed`` delimited list of flags.  

The flags indicate what exactly happend at the path.

List of flags 

```c
Folder item created 256
Folder item removed 512
Folder item meta modified 1024
Folder item renamed 2048
Folder item modified 4096
Folder item info modified 8192
Folder item owner changed 16384
Folder item attribute modified 32768
Folder item is file 65536
Folder item is directory 131072
Folder item is symlink 262144
```

#### Platform considerations

On Mac, trashing an item qualifies as a rename event, as it is not really removed at the system level.

On Windows, restoring an item from the trash or moving an item to/from a monitored folder generates create and modify events. **Moving an item across monitored folders may generate misleading events**.

On Windows, only a subset of the flags are reported (created, removed, renamed, modified, is file, is directory).

On Windows, there is no way of knowing if the path was a file or folder, **when it is removed**.

On both platforms, a rename event will fire twice; once for the old name and once more for the new name.

### Example Callback

```
C_TEXT($1;$path)
C_REAL($2;$unixtime)
C_LONGINT($3;$flags)
```

### Examples

```
$success:=FW Set watch path (System folder(Desktop))
$success:=FW Set watch method ("FOLDER_METHOD_CALLED_ON_EVENT")
```

In ``$2`` to ``FW Set watch paths`` or ``FW Set watch path``, you can pass an optional ``latency``, which is the number of seconds to wait before an event is sent. By default the latency is 1.0, the maximum is 60.0. A longer latency may contribute to better efficiency (Mac only).
