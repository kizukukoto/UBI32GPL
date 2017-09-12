<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class MetaDbInfo extends RestComponent {

	public function __construct() {
		require_once('fileinfo.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('util.inc');
	}


	/**
	 * Get attributes of specifed file.
	 * Security checks: Verifies readable share and valid path name.
	 *
 	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	public function get($urlPath, $queryParams=null, $output_format='xml') {
		$sharePath = getSharePath();
		$shareName = trim($urlPath[0]);
		$urlPath   = array_slice($urlPath, 1);
		$reqPath   = implode('/', $urlPath);
		$path      = !empty($reqPath) ? '/'.$shareName.'/'.$reqPath : '/'.$shareName;

		$includeHidden = isset($queryParams['include_hidden'])  ? trim($queryParams['include_hidden'])  : false;
		$includeHidden = ($includeHidden === 'true' || $includeHidden === '1'  || $includeHidden === 1) ? true : false;
		
		$recursive = isset($queryParams['recursive'])  ? trim($queryParams['recursive'])  : 'true';
		$recursive = ($recursive === 'true' || $recursive === '1'  || $recursive === 1) ? 'true' : 'false';

		$singleDir = isset($queryParams['single_dir'])  ? trim($queryParams['single_dir'])  : false;
		$singleDir = ($singleDir === 'true' || $singleDir === '1'  || $singleDir === 1) ? true: false;
		
		$startTime = isset($queryParams['start_time']) ? trim($queryParams['start_time']) : '';
		$startTime = !is_numeric($startTime) ? strtotime($startTime) : $startTime;

		$volInfo   = getMediaVolumesInfo();
		$dbPath    = isset($volInfo[0]['DbPath']) ? $volInfo[0]['DbPath'] : '';
		$lastPurgeTime = getLastPurgeTime($dbPath);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'volInfo', print_r($volInfo,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'xmlFile', $xmlFile);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'lastPurgeTime', $lastPurgeTime);

		if (!empty($startTime) && $startTime < $lastPurgeTime){
			$this->generateErrorOutput(400, 'metadb_info', 'INVALID_START_TIME', $output_format);
			return;
		}

		if (empty($shareName)){
			$this->generateErrorOutput(400, 'metadb_info', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (!file_exists($sharePath.'/'.$shareName)) {
			$this->generateErrorOutput(400, 'metadb_info', 'INVALID_SHARE', $output_format);
			return;
		}

		if (!isShareAccessible($shareName, false)) {
			$this->generateErrorOutput(401, 'metadb_info', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		if (!isset($volInfo[0])) {
			$this->generateErrorOutput(400, 'metadb_info', 'VOLUME_INFO_MISSING', $output_format);
			return;
		}

		if (empty($dbPath)) {
			$this->generateErrorOutput(400, 'metadb_info', 'DB_PATH_MISSING', $output_format);
			return;
		}

		// wdfiles is supported value for filecolumns.
		// This will be changed to supported different sets for bali project.
		$fileColumns = getFileInfoColumns();
		if (!empty($queryParams['file_columns'])) {
			if($queryParams['file_columns'] != 'wdfiles'){
				$this->generateErrorOutput(400, 'metadb_info', 'INVALID_PARAMETER', $output_format);
				return;
			}
			$fileColumns = getFileInfoColumns();
		}

		// wdfiles is supported value for filecolumns.
		// This will be changed to supported different sets for bali project.
		$dirColumns = getDirInfoColumns();
		if (!empty($queryParams['dir_columns'])) {
			if($queryParams['dir_columns'] != 'wdfiles'){
				$this->generateErrorOutput(400, 'metadb_info', 'INVALID_PARAMETER', $output_format);
				return;
			}
			$dirColumns = getDirInfoColumns();
		}

		$files = $queryParams['files'] == 'false' ? false : true;
		$dirs  = $queryParams['dirs']  == 'false' ? false : true;

		if ($files == false && $dirs == false) {
			$this->generateErrorOutput(400, 'metadb_info', 'INVALID_PARAMETER', $output_format);
			return;
		}

		$fileRowOffset = !empty($queryParams['file_row_offset']) ? $queryParams['file_row_offset'] : null;
		$fileRowCount  = !empty($queryParams['file_row_count'])  ? $queryParams['file_row_count']  : null;
		$dirRowOffset  = !empty($queryParams['dir_row_offset'])  ? $queryParams['dir_row_offset']  : null;
		$dirRowCount   = !empty($queryParams['dir_row_count'])   ? $queryParams['dir_row_count']   : null;

		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement('metadb_info');
		$output->element('generated_time', time());
		$output->element('last_purge_time',$lastPurgeTime);

		//Generate output for files.
		if($singleDir) //If info on an individual directory is requested, no file information is needed
			$files = false;
		if ($files) {
			$fileList = generateFileInfoList($dbPath, $path, $startTime, $fileColumns, $recursive, $fileRowOffset, $fileRowCount, $output, $sharePath, $includeHidden);
		}

		//Generate output for dirs.
		if ($dirs) {
			$dirList = generateDirInfoList($dbPath, $path, $startTime, $dirColumns, $recursive, $singleDir, $dirRowOffset, $dirRowCount, $output, $sharePath, $includeHidden);
		}

		$output->popElement();
		$output->close();
	}
}
?>