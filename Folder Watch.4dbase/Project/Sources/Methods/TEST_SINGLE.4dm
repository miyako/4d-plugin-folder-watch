//%attributes = {}
$path:=System folder:C487(Desktop:K41:16)+"test"+Folder separator:K24:12

CREATE FOLDER:C475($path; *)

$success:=FW Set watch path($path; 60)
$success:=FW Set watch method("FOLDER_METHOD_CALLED_ON_EVENT")

FW GET WATCH PATHS($paths)