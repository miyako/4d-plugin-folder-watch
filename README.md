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
**Register a callback method**

```
ARRAY TEXT($paths;2)
$paths{1}:=System folder(Desktop)
$paths{2}:=System folder(Documents folder)

$success:=FW Set watch paths ($paths)
```

 
