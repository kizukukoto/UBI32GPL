<?php
/**
 * Class Status
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
 * Status Class extends the Rest Component and accesses the firmware version file.
 *
 * @version Release: @package_version@
 *
 */
class Status extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get status of processes
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		$apps = array(
			'communicationmanager' => '/usr/local/orion/communicationmanager/communicationmanagerd',
			'mediacrawler'         => '/usr/local/mediacrawler/mediacrawlerd',
			'miocrawler'           => '/usr/local/orion/miocrawler/miocrawlerd',
		);
		foreach($apps as $app => $program) {
			$command = "ps -ef | grep $app | grep -v grep";
			$output  = exec($command);
			$status = !empty($output) ? 'running' : 'not running';
			$results[$app] = $status;
		}
		$this->generateSuccessOutput(200, 'status', $results, $output_format);
	}
}
?>