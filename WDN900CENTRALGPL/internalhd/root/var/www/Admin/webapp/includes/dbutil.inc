<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('globalconfig.inc');

function getDB() {
	if(!array_key_exists('db', $GLOBALS)){
		$db = openDB();
		if ($db) {
			$GLOBALS['db'] = $db;
		}
	}
	return $GLOBALS['db'];
}

function openDB() {
	$dbConfig = getGlobalConfig('db');
	$dbFilePath = $dbConfig['DATA_BASE_FILE_PATH'];
	
	return openDbByPath($dbFilePath);
}

function openDbByPath($dbFilePath) {
	if (file_exists($dbFilePath)) {
		$pdo = new PDO('sqlite:' . $dbFilePath);
		
		return $pdo;
	} else {
		echo(__FUNCTION__ + 'DB file does not exist: ' . $dbFilePath);
		return null;
	}
	
}

function openMioDB() {
	$dbConfig = getGlobalConfig('db');
	$deviceType = getDeviceType();
	if ($deviceType == STINGRAY_DEVICE_TYPE) {
		$dbFilePath = $dbConfig['MIO_DATA_BASE_FILE_PATH'];
	} else {
		$dbFilePath = $dbConfig['DATA_BASE_FILE_PATH'];
	}
	return openDbByPath($dbFilePath);
	
}

function closeDB() {
	if(array_key_exists('db', $GLOBALS)){
		$GLOBALS['db'] = null;
		unset($GLOBALS['db']);
	}
}

function openMediaDb($mediaDbFilePath) {
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'mediaDbFilePath', $mediaDbFilePath);
	$pdo =  new PDO('sqlite:' . $mediaDbFilePath);
	$stmt = $pdo->prepare("PRAGMA CASE_SENSITIVE_LIKE=ON");
	$ret = $stmt->execute();
	return $pdo; 	
}

function closeMediaDb($mediaDb) {
	$mediaDb = null;
}
?>
