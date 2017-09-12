<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class PortTest extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('util.inc');
	}


	/**
	 * Get the device id
	 *
	 * @param string $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */

	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$deviceId = getDeviceId();
		if (isset($deviceId)) {
			$results = array('deviceid' => $deviceId);
			$this->generateSuccessOutput(200, 'port_test', $results, $output_format);
		} else {
			$this->generateErrorOutput(404, 'port_test', 'DEVICE_ID_NOT_FOUND', $output_format);
		}
	}
}
?>