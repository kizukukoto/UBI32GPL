<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once("restcomponent.inc");

class DeviceUserNotify extends RestComponent{

	function DeviceUserNotify() {
		require_once("util.inc");
		require_once("globalconfig.inc");
		require_once("security.inc");
		require_once("outputwriter.inc");
	}

	function get($urlPath, $queryParams=null, $output_format='xml') {
		$results = array('status' => 'success');
		$this->generateItemOutput(200, 'DeviceUserNotify', $results, $output_format);
	}

}
?>
