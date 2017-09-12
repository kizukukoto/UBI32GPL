<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class MioCrawlerStatus extends RestComponent {

	function __construct() {
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('statusdb.inc');
		require_once('util.inc');
	}


	/**
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$statusdb = new StatusDB();
		$status = $statusdb->getStatus();
		if (!empty($status)) {
			$this->generateItemOutput(200, 'miocrawler_status', $status, $output_format);
		} else {
			$this->generateErrorOutput(500, 'miocrawler_status', 'MIOCRAWLER_STATUS_FAILED', $output_format);
		}
	}

}
?>