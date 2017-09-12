<?php
/**
 * Class Version
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
 * Version Class extends the Rest Component and accesses the firmware version file.
 *
 * @version Release: @package_version@
 *
 */
class Version extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	/**
	 * Get version of release
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		$latestVersion = !empty($urlPath[0]) ? $urlPath[0] : '';
		if(getDeviceType() === '5'){
			$file = '/etc/config/buildver';
		}else{
			$file = '/etc/version';
		}
		
		if (!file_exists($file)) {
			$this->generateErrorOutput(404, 'version', 'FILE_NOT_FOUND', $output_format);
			return;
		}

		$contents = file_get_contents($file);
		$firmware = trim($contents);
		$results  = array('firmware' => $firmware);

		if (!empty($latestVersion)) {
			if ($firmware == $latestVersion) {
				$upToDate = 'true';
			} else {
				$upToDate = 'false';
			}
			$results['up_to_date'] = $upToDate;
		}
		$this->generateSuccessOutput(200, 'version', $results, $output_format);
	}
}
?>