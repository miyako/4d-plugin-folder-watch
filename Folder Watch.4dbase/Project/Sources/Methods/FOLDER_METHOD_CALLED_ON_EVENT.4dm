//%attributes = {"invisible":true}
//FOLDER_METHOD_CALLED_ON_EVENT

C_TEXT:C284($1; $path)
C_REAL:C285($2; $unixtime)
C_LONGINT:C283($3)

$path:=$1
$unixtime:=$2
$flags:=$3

$isDir:=0#($flags & Folder Item is directory)
$isFile:=0#($flags & Folder Item is file)
$isNew:=0#($flags & Folder Item created)
$isModified:=0#($flags & Folder Item modified)
$isGone:=0#($flags & Folder Item removed)
$isRenamed:=0#($flags & Folder Item renamed)

