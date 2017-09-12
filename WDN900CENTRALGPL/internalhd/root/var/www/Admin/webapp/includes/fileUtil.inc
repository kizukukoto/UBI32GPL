<?php 


function orionEscapeArg($arg) {

	return "'".str_replace("'", "'\\''", $arg)."'";

}

function rmdirRecursive($deleteDir) {
	if (! is_dir($deleteDir))
	return;
	$success = true;
	$directory = dir( $deleteDir );
	while ( FALSE !== ( $dirItem = $directory->read() ) ) {
		if ($dirItem == "." || $dirItem == "..")
		continue;
		$deletePath = $deleteDir . '/' . $dirItem;
		if ( is_dir( $deletePath ) )
		$success &= rmdirRecursive($deletePath);
		else
		$success &= unlink($deletePath);
		if(!$success)
		break;
	}
	$directory->close();
	$success &= rmdir($deleteDir);
	return $success;
}

	function orionCopy( $src, $dst ) {
		$success = true;

		$src = orionEscapeArg($src);
		$dst = orionEscapeArg($dst);
		$lines = array();
		exec("cp " . $src . " " . $dst , $lines, $retVal);
		if ($retVal != 0) {
			$success = false;
		}
		return $success;
	}

	function orionCopyDir( $src, $dst ) {
		$success = true;

		$src = orionEscapeArg($src);
		$dst = orionEscapeArg($dst);
		
		$lines = array();
		exec("cp -fr " . $src . " " . $dst , $lines, $retVal);
		if ($retVal != 0) {
			$success = false;
		}
		return $success;
	}	

	function copyDirRecursive( $sourceDir, $destDir ) {
		$success = true;
		if ( is_dir( $sourceDir ) ) {
			$success=orionCopyDir($sourceDir, $destDir);
		} else {
			$success = orionCopy( $sourceDir, $destDir );
		}
		return $success;
	}
	
	function getChildCount($dir) {
		//return count(glob($dir.'/*'));
		return count(scandir($dir)); //we shouldn't include '.' or '..' in our count of children
	}
	
?>