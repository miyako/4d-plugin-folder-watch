# 4d-plugin-folder-watch-2
Folder watch 4D plugin for Mac, 2nd edition.

Unlike [folder-watch](https://github.com/miyako/4d-plugin-folder-watch), which uses [Grand Central Dispatch (GCD)](https://developer.apple.com/library/ios/documentation/Performance/Reference/GCD_libdispatch_Ref/), this plugin uses [FSEvents](https://developer.apple.com/library/mac/documentation/Darwin/Reference/FSEvents_Ref/index.html#//apple_ref/doc/constant_group/FSEventStreamCreateFlags).

Here is a quick comparision of features:

| |folder-watch|folder-watch-2|
---
|watch multiple paths|⭕️| |
