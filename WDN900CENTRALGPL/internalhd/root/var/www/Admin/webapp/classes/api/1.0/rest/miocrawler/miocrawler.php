<?php
// Copyright (c) 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class MioCrawler extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
	}
	
		
	function put($urlPath, $queryParams=null, $output_format='xml') {

		$action = isset($queryParams['action']) ? trim($queryParams['action']) : null;

		if (empty($action)) {
			$this->generateErrorOutput(400, 'miocrawler', 'PARAMETER_MISSING', $output_format);
			return;
		}

		$remoteAccess = getRemoteAccess();	
		if ( strcasecmp($remoteAccess,"FALSE") == 0) {
			$this->generateErrorOutput(400, 'miocrawler', 'REMOTE_ACCESS_DISABLED', $output_format);
			return;			
		}
		
		if (strcasecmp($action,"reset") == 0){
			$mioCrawlerpath = getMioCrawlerPath();
			
			// Stop mio crawler
			$cmd = $mioCrawlerpath." stop";
			exec($cmd, $output, $retVal);
			
			// Reset mio crawler
			$cmd = $mioCrawlerpath." reset";
			exec($cmd, $output, $retVal);
			
			// Start mio crawler
			$cmd = $mioCrawlerpath." start";
			exec($cmd, $output, $retVal);
		}else{
			$this->generateErrorOutput(400, 'miocrawler', 'INVALID_PARAMETER', $output_format);
			return;			
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'miocrawler', $results, $output_format);
	}
	
}
?>
