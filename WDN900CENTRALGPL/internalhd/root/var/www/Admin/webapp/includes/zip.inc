<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('zipstream.php');

function generateZipSteram($zipFileName, $fileList){
	if(isset($fileList) && sizeof($fileList) > 0){
		$options = array('large_file_size' => 2 * 1024 * 1024, 
						'large_file_method' => 'store');
		$zip = new ZipStream($zipFileName, $options);
	    foreach($fileList as $file => $filePth) {
	    	$zip->add_file_from_path($file, $filePth);
		}
		$zip->finish();
		return true;
	}else{
		return false;
	}
}

?>