<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once("restcomponent.inc");

class DirInfo extends RestComponent{

	function DirInfo() {
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
			$this->generateErrorOutput(400, 'dirinfo', "Share name is missing.", $output_format);
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
		
		$columns = getDirInfoColumns();
		if (isset($queryParams['columns'])) {
			$columns = $queryParams['columns'];
		}
		
		if(empty($columns)){
			$this->generateErrorOutput(400, 'dirinfo', "Missing columns.", $output_format);
			return;
		}
		
		// dirRowOffset
		$dirRowOffset = NULL;
		if (isset($queryParams['dirRowOffset'])) {
			$dirRowOffset = $queryParams['dirRowOffset'];
			if($dirRowOffset < 0){
				$this->generateErrorOutput(400, 'dirinfo', "Invalid dirRowOffset", $output_format);
				return;
			}
		}
		
		// dirRowOffset
		$dirRowCount = NULL;
		if (isset($queryParams['dirRowCount'])) {
			$dirRowCount = $queryParams['dirRowCount'];
			if($dirRowCount < 0){
				$this->generateErrorOutput(400, 'dirinfo', "Invalid dirRowCount", $output_format);
				return;
			}
		}
				
		// Get Volume infor, from media crawler config.
		$volInfo = getMediaVolumesInfo();
		if(!isset($volInfo[0])){
			$this->generateErrorOutput(400, 'dirinfo', "Voulme informartion is missing. Check media crawler configuration.", $output_format);
			return;
		}
		
		$dirList = getDirInfoList($volInfo[0]["DbPath"], $requestPath, $startTime, $columns, 
							$recursive, $dirRowOffset, $dirRowCount);
		if(isset($dirList)){
			$items = array();
			$rowIndex = 0;
			$resultStartIndex = $dirRowOffset-1;
			$resultRowCount = 0;
			foreach($dirList as $dirInfo) {
				if($rowIndex >= $resultStartIndex){
					$item = mapDirItem($dirInfo, $sharePath);
					array_push($items, $item);
					$resultRowCount++;
					if($dirRowCount == $resultRowCount)
						break;
				}
				$rowIndex++;
			}			
			$this->generateCollectionOutputWithType(200, 'dirs', 'dir', $items, $output_format);
		}else{
			$this->generateErrorOutput(500, 'dirinfo', "Error in getting dir list.", $output_format);
		}
	}
}
?>