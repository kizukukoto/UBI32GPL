<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once("globalconfig.inc");

function getFileExtensionStr($fullFilePath) {
	$subStr = strrchr($fullFilePath, '.');
	if($subStr == false)
		return false;
	return substr($subStr, 1);
}

function getTranscodingExtensionStr($transcodingType) {
	switch($transcodingType) {
		case 'tn96s1': 
		case 'tn96s2': 
		case 'i480s1': 
		case 'i1024s1': 
			return 'jpg';
	}
	return false;
}

/**
 * Miocrawler uses a string which contains the transcoding details in generating the hashed file name for transcoded contents.
 * This routine maps from a transcodingType to the string which contains the transcoding details.
 * @param String $transcodingType one of the standard types of transcoding (e.g. tn96s1)
 * @return String parameters of transcoding
 */
function getTranscodingParamStr($transcodingType) {
	if(!isset($GLOBALS[$transcodingType])){
		$mioIniPath = getMioCrawlerXcodeIniPath();
		if(isset($mioIniPath)){
			$iniFile = parse_ini_file($mioIniPath, true);
			$GLOBALS['tn96s1'] = $iniFile['tn96s1'];
			$GLOBALS['i480s1'] = $iniFile['i480s1'];
			$GLOBALS['i1024s1'] = $iniFile['i1024s1'];
		}
	}
	switch($transcodingType) {
		case 'tn96s1': 
			//return 'type=imageScaling width=96 height=96 color=000000 format=jpg quality=0.80 canvas_width=96 canvas_height=96 fit=FIT_MINIMUM meta_data=MD_NO_META_DATA auto_orient=true';
			return $GLOBALS['tn96s1'];
		case 'tn96s2': 
			return '--UNKNOWN--';
		case 'i480s1': 
			//return 'type=imageScaling width=480 height=480 color=000000 format=jpg quality=0.85 fit=FIT_MAXIMUM meta_data=MD_NO_THUMBNAIL auto_orient=true';
			return $GLOBALS['i480s1'];
		case 'i1024s1': 
			//return 'type=imageScaling width=1024 height=1024 color=000000 format=jpg quality=0.85 fit=FIT_MAXIMUM meta_data=MD_NO_THUMBNAIL auto_orient=true';
			return $GLOBALS['i1024s1'];
	}
	return false;
}

/**
 * Returns the name of the transcoded file based on the original path and the type of transcoding (e.g. thumbnail1)
 * @param $fullFilePath the full path to the non-transcoded file (case sensitive)
 * @param $transcodingType the standard identifier for the type of transcoding
 * @return String $transcodedFileName the name of the transcoded file
 */
function getTranscodedFileName($fullFilePath, $transcodingType) {

	if (!getTranscodingExtensionStr($transcodingType)) {
		return 400;
	}

	if (!file_exists($fullFilePath)) {
		return false;
	}

	$fileStats = @stat($fullFilePath);
	$dateStr = $fileStats['mtime'];
	
	//The combination of the file path and the type of transcoding
	//$tmpFilepath = str_replace("c:/shares/Public/Shared Pictures","/shares/Public/Shared Pictures", $fullFilePath);
	$directoriesConfig = getGlobalConfig("directories");
	$windowsDemo = $directoriesConfig["WINDOWS_DEMO"];
	if($windowsDemo == "true"){
		$fullFilePath = str_replace("c:"," ", $fullFilePath);
		$identifyingInfo = trim($fullFilePath).' '.getTranscodingParamStr($transcodingType);//TRANS_STRING[nametype];
	}else{
		/*
		$wdtvDemo = $directoriesConfig["WDTV_DEMO"];
		if($wdtvDemo == "true"){
			$fullFilePath = str_replace("/tmp/media/usb/Local/WDTVLiveHub/WD TV Photos", "/shares/Public/Shared Pictures", $fullFilePath);	
		}
		*/
		$identifyingInfo = $fullFilePath.' '.getTranscodingParamStr($transcodingType);//TRANS_STRING[nametype];
	}
	//A hash of the identifying information
	$hashStr = substr(strtoupper(md5($identifyingInfo)), 0, 32);

	$transcodedFileName = $hashStr.'.'.strtoupper(dechex($dateStr)).'.'.getTranscodingExtensionStr($transcodingType);
	return $transcodedFileName;
}

/**
 * Returns the full path to the transcoded file based on the original path and the type of transcoding
 * @param $fullFilePath the full path to the non-transcoded file (case sensitive)
 * @param $transcodingType the standard identifier for the type of transcoding
 * @return String $transcodedFilePath the full path to the transcoded file
 */
function getTranscodedFilePath($fullFilePath, $transcodingType) {
	$transcodedFileName = getTranscodedFileName($fullFilePath, $transcodingType);

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'transcodedFileName', $transcodedFileName);

	if (is_numeric($transcodedFileName) && intval($transcodedFileName) == 400) {
		return 400;
	}
	if (empty($transcodedFileName)) {
		return false;
	}
	//Look up path to transcoded files
	$directoriesConfig = getGlobalConfig("directories");
	$transcodedFilesPath = $directoriesConfig["TRANSCODED_FILES_PATH"];
	return $transcodedFilesPath.'/'.$transcodedFileName;
}
?>