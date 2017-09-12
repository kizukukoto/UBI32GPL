<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once("restcomponent.inc");

class FileInfo extends RestComponent{

	function FileInfo() {
		require_once("util.inc");
		require_once("globalconfig.inc");
		require_once("outputwriter.inc");
		require_once("fileinfo.inc");
	}
	

	/**
	 * Get attributes of specifed file.
	 * Security checks: Verifies readable share and valid path name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		
		$shareName = $urlPath[0];
		$urlPath = array_slice($urlPath, 0);
		$requestPath = '/'.implode("/", $urlPath);
		$sharePath = getSharePath();
		$requestPath = $sharePath.$requestPath;
		
		if (!isset($shareName)){
			$this->generateErrorOutput(400, 'fileinfo', "Share name is missing.", $output_format);
			return;
		}
		
		//TODO : Check whether user has read access to share
		
		$recursive = false;
		if (isset($queryParams['recursive'])) {
			$recursive = $queryParams['recursive'];
		}
		
		$startTime = NULL;
		if (isset($queryParams['startTime'])) {
			$startTime = $queryParams['startTime'];
		}
		
		$columns = getFileInfoColumns();
		if (isset($queryParams['columns'])) {
			$columns = $queryParams['columns'];
		}
		
		if(empty($columns)){
			$this->generateErrorOutput(400, 'fileinfo', "Missing columns.", $output_format);
			return;
		}
		
		// fileRowOffset
		$fileRowOffset = NULL;
		if (isset($queryParams['fileRowOffset'])) {
			$fileRowOffset = $queryParams['fileRowOffset'];
			if($fileRowOffset < 0){
				$this->generateErrorOutput(400, 'fileinfo', "Invalid fileRowOffset", $output_format);
				return;
			}
		}
		
		// fileRowOffset
		$fileRowCount = NULL;
		if (isset($queryParams['fileRowCount'])) {
			$fileRowCount = $queryParams['fileRowCount'];
			if($fileRowCount < 0){
				$this->generateErrorOutput(400, 'fileinfo', "Invalid fileRowCount", $output_format);
				return;
			}
		}
		
		// Get Volume infor, from media crawler config.
		$volInfo = getMediaVolumesInfo();
		if(!isset($volInfo[0])){
			$this->generateErrorOutput(400, 'fileinfo', "Voulme informartion is missing. Check media crawler configuration.", $output_format);
			return;
		}
		
		$fileList = getFileInfoList($volInfo[0]["DbPath"], $requestPath, $startTime, 
							$columns, $recursive, $fileRowOffset, $fileRowCount);
		if(isset($fileList)){
			$items = array();
			$rowIndex = 0;
			$resultStartIndex = $fileRowOffset-1;
			$resultRowCount = 0;
			foreach($fileList as $fileInfo) {
				if($rowIndex >= $resultStartIndex){
					$item = mapFileItem($fileInfo, $sharePath);
					array_push($items, $item);
					$resultRowCount++;
					if($fileRowCount == $resultRowCount)
						break;
				}
				$rowIndex++;
			}
			$this->generateCollectionOutputWithType(200, 'files', 'file', $items, $output_format);
		}else{
			$this->generateErrorOutput(500, 'fileinfo', "Error in getting file list.", $output_format);
		}
	}
}
?>