<?php
/**
 * Class Users
 *
 * PHP version 5.2
 *
 * @category  Classes
 * @package   Orion
 * @author    WDMV Software Engineering
 * @copyright 2011 (c) Western Digital Technologies, Inc. - All rights reserved
 * @license   http://support.wdc.com/download/netcenter/gpl.txt GNU License
 * @link      http://www.wdc.com/
 *
 */
require_once('restcomponent.inc');

/**
 * MetaDBSummary Class extends the Rest Component and accesses the mediacrawler database.
 *
 * @version Release: @package_version@
 *
 */
class MetaDBSummary extends RestComponent {

	function __construct() {
		require_once('fileinfo.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('util.inc');
	}


	/**
	 * Get summary of specifed directory.
	 * Security checks: Verifies readable share and valid path name.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $outputFormat='xml') {
		$sharePath = getSharePath();
		$shareName = trim($urlPath[0]);
		$urlPath   = array_slice($urlPath, 1);
		$reqPath   = implode('/', $urlPath);
		$path      = !empty($reqPath) ? '/'.$shareName.'/'.$reqPath : '/'.$shareName;

		$recursive = isset($queryParams['recursive'])  ? trim($queryParams['recursive'])  : 'true';
		$recursive = ($recursive === 'true' || $recursive === '1'  || $recursive === 1) ? 'true' : 'false';

		$startTime = isset($queryParams['start_time']) ? trim($queryParams['start_time']) : '';
		$startTime = !is_numeric($startTime) ? strtotime($startTime) : $startTime;

		$includeHidden = isset($queryParams['include_hidden'])  ? trim($queryParams['include_hidden'])  : false;
		$includeHidden = ($includeHidden === 'true' || $includeHidden === '1'  || $includeHidden === 1) ? true : false;
		
		
		$mediaType = isset($queryParams['media_type']) ? strtolower(trim($queryParams['media_type'])) : '';

		$mediaTypes = array('all', 'videos', 'music', 'photos', 'docs');

		$category = 0;
		foreach ($mediaTypes as $type) {
			if ($mediaType == $type) {
				break;
			}
			$category++;
		}

		$category = $category < count($mediaTypes) ? $category : 0;

		$volInfo   = getMediaVolumesInfo();
		$dbPath    = isset($volInfo[0]['DbPath']) ? $volInfo[0]['DbPath'] : '';
		$lastPurgeTime = getLastPurgeTime($dbPath);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'volInfo', print_r($volInfo,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'dbPath', $dbPath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'lastPurgeTime', $lastPurgeTime);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sharePath', $sharePath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'shareName', $shareName);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'reqPath', $reqPath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'path', $path);

		if (!empty($startTime) && $startTime < $lastPurgeTime){
			$this->generateErrorOutput(400, 'metadb_summary', 'INVALID_START_TIME', $outputFormat);
			return;
		}

		if (empty($shareName)){
			$this->generateErrorOutput(400, 'metadb_summary', 'SHARE_NAME_MISSING', $outputFormat);
			return;
		}

		if (!file_exists($sharePath.'/'.$shareName)) {
			$this->generateErrorOutput(400, 'metadb_summary', 'INVALID_SHARE', $outputFormat);
			return;
		}

		if (!isShareAccessible($shareName, false)) {
			$this->generateErrorOutput(401, 'metadb_summary', 'USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}

		if (!isset($volInfo[0])) {
			$this->generateErrorOutput(400, 'metadb_summary', 'VOLUME_INFO_MISSING', $outputFormat);
			return;
		}

		if (empty($dbPath)) {
			$this->generateErrorOutput(400, 'metadb_summary', 'DB_PATH_MISSING', $outputFormat);
			return;
		}

		$results = getDirSummary($dbPath, $path, $recursive, $startTime, $category, $includeHidden);

		$results['path'] = $path;
		$this->generateItemOutput(200, 'metadb_summary', $results, $outputFormat);
	}
}
?>