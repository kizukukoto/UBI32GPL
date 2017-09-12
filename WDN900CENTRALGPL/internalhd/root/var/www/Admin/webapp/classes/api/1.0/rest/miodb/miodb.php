<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class MioDB extends RestComponent{

	function __construct() {
		require_once('miodb.inc');
		require_once('outputwriter.inc');
	}


	/**
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$compress = strtolower(trim($queryParams['compress']));
		if (empty($queryParams) || empty($compress) || ( (strcmp($compress,'true') != 0) && (strcmp($compress,'false') != 0))) {
			$this->generateErrorOutput(400, 'miodb', 'PARAMETERS_MISSING', $output_format);
			return;
		}
		$doCompress = (strcmp($queryParams['compress'], 'true') == 0);
		try {
			$results = readMioDBFile($doCompress, STINGRAY_DEVICE_TYPE);
			if ($results != true){
				$this->generateErrorOutput($retVal['code'], 'miodb', $results['msg'], $output_format);
			}
		} catch(Exception $e) {
			$this->generateErrorOutput(500, 'miodb', 'MIODB_FAILED', $output_format);
		}
	}
}
?>