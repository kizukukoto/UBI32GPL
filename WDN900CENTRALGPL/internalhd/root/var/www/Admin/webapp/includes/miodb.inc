<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('contents.inc');
require_once("globalconfig.inc");

/**
 * 
 */
function readMioDBFile($compress, $deviceType = null){
	
	//Get DB file path from config file
	$dbConfig = getGlobalConfig('db');
	
	
	if ($deviceType == STINGRAY_DEVICE_TYPE) {
		$dbFilePath = $dbConfig['MIO_DATA_BASE_FILE_PATH'];
	} else {
		$dbFilePath = $dbConfig['DATA_BASE_FILE_PATH'];
	}
	

	// TODO : Generate user specific db file. 
	// Also add file cache for user specific DB files. 
	// Check for valid file name.

	if(!file_exists($dbFilePath)){
		// Generate error message.
		return array ("code" => 404, "msg" => "Database File Not Found, path: " . $dbFilePath);
	}else {
		//JS - take a copy of the DB file - so we don't slow down reads/writes
		//or risk causing a lock exception

		$dbFilePathDl = $dbFilePath . ".dl";
		if (!copy($dbFilePath, $dbFilePathDl )) {
			return array ("code" => 404, 
						"msg" => "Database Copy Failed, path: " . $dbFilePath);
		}else {
			if ($compress) {
				//create zip file
				exec("rm ".$dbFilePathDl . ".gz");
				$result = exec("gzip " . $dbFilePathDl );
				$dbFilePathDl = $dbFilePathDl . ".gz";
				if (!file_exists($dbFilePathDl)) {
					return array ("code" => 404, 
						"msg" => "Failed to compress Database file: " . $dbFilePath);
				}
			}
			// Read DB file copy - sends file to output stream

			readFileFromPath($dbFilePathDl, NULL);
			return true;
		}
	}
}	

?>
