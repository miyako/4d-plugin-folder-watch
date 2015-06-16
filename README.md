# 4d-plugin-folder-watch-2
Folder watch 4D plugin for Mac, 2nd edition.

Unlike [folder-watch](https://github.com/miyako/4d-plugin-folder-watch) which uses [Grand Central Dispatch (GCD)](https://developer.apple.com/library/ios/documentation/Performance/Reference/GCD_libdispatch_Ref/), this plugin uses [FSEvents](https://developer.apple.com/library/mac/documentation/Darwin/Reference/FSEvents_Ref/index.html#//apple_ref/doc/constant_group/FSEventStreamCreateFlags).

Here is a quick comparision of features:

|  | folder-watch | folder-watch-2 |
| :------------- | :-------------: | :-------------: |
| Watch multiple paths | × | ◎ |
| Watch subfolders | × | ◎ |
| OS X minimum version | 10.7 | 10.6 |

Examples
---
**Register folder paths to watch**

```
ARRAY TEXT($paths;2)
$paths{1}:=System folder(Desktop)
$paths{2}:=System folder(Documents folder)

$success:=FW Set watch paths ($paths)
```

In ``$2`` to ``FW Set watch paths`` or ``FW Set watch path``. you can pass an optional ``latency``, which is the number of seconds to wait before an event is sent. By default the latency is 1.0, the maximum is 60.0. A long latency may contribute to better efficiency.

**Install callback method**

```
$success:=FW Set watch method ("WATCH")
```

Install method will receive in ``$1`` the path where a change was detected.

If multiple paths are passed, they will be delimiter by 0x0A (line feed).

