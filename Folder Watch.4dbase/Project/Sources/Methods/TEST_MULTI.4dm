//%attributes = {}
$path1:=System folder:C487(Desktop:K41:16)+"test2"+Folder separator:K24:12
CREATE FOLDER:C475($path1; *)

$path2:=System folder:C487(Desktop:K41:16)+"test1"+Folder separator:K24:12
CREATE FOLDER:C475($path2; *)


ARRAY TEXT:C222($paths; 2)
$paths{1}:=$path1
$paths{2}:=$path2

$success:=FW Set watch paths($paths; 60)
$success:=FW Set watch method("FOLDER_METHOD_CALLED_ON_EVENT")

FW GET WATCH PATHS($paths)
