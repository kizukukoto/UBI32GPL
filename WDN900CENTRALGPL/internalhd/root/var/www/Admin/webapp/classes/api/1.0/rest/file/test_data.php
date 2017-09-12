<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class TestData extends RestComponent {

	function __construct() {
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
		header("Content-Type: application/octet-stream");

		$size = isset($queryParams['size']) ? trim($queryParams['size']) : '';
		$sizeK = isset($queryParams['size_k']) ? trim($queryParams['size_k']) : '';
		$sizeM = isset($queryParams['size_m']) ? trim($queryParams['size_m']) : '';

		$junk = str_repeat('X', $size);
		echo $junk;
		if(intval($sizeK) > 0 ) {
			$junkK = str_repeat('K', 1024);		
			for($index=0; $index<$sizeK; $index++) {
				echo $junkK;
			}
		}
		
		if(intval($sizeM) > 0 ) {
			$junkK = str_repeat('M', 1024);		
			$junkM = str_repeat($junkK, 1024);
			for($index=0; $index<$sizeM; $index++) {
				echo $junkM;
			}
		}
	}
}
?>
