<?php
/**
 * Class Drives
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
 * Drives Class extends the Rest Component and accesses the low-level Drive info.
 *
 * @version Release: @package_version@
 *
 */
class Drives extends RestComponent {

	function __construct()
	{
		require_once('driveinfo.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get drive information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 * @return array $results
	 */
	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$serialno = !empty($urlPath[0]) ? $urlPath[0] : '';

		$driveinfo = new DriveInfo();

		if (!empty($serialno)) {
			$drive = $driveinfo->select($serialno);
			if (empty($drive)) {
				$this->generateErrorOutput(404, 'drives', 'DRIVE_NOT_FOUND', $output_format);
				return;
			}
			$this->generateItemOutput(200, 'drive', $drive, $output_format);
			return;
		}

		$drives = $driveinfo->select();

		if (empty($drives)) {
			$this->generateErrorOutput(404, 'drives', 'NO_DRIVES_FOUND', $output_format);
			return;
		}

		$this->generateCollectionOutput(200, 'drives', 'drive', $drives, $output_format);
	}


	/**
	 * Update drive information.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 * @return array $results
	 */
	function put($urlPath, $queryParams=null, $output_format='xml')
	{
		$serialno = !empty($urlPath[0]) ? $urlPath[0] : '';
		$password = isset($queryParams['password']) ? trim($queryParams['password']) : '';
		$eject    = isset($queryParams['eject'])    ? trim($queryParams['eject'])    : '';

		if (empty($serialno)) {
			$this->generateErrorOutput(400, 'drives', 'PARAMETER_MISSING', $output_format);
			return;
		}

		$driveinfo = new DriveInfo();

		$drive = $driveinfo->select($serialno);
		if (empty($drive)) {
			$this->generateErrorOutput(404, 'drives', 'DRIVE_NOT_FOUND', $output_format);
			return;
		}

		try {
			$status = $driveinfo->update($serialno, $password, $eject);
			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'drives', $results, $output_format);
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'drives', $e->getMessage(), $output_format);
		}
	}
}
?>