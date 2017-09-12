<?php
// Copyright (c) 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class ManualPortForward extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
	}
	
	function get($urlPath, $queryParams=null, $output_format='xml') {
		$manualportForward	= getManualportForward();
		$manualExternalHttpPort	= getManualExternalHttpPort();
		$manualExternalHttpsPort	= getManualExternalHttpsPort();
		
		$manualportForward = strtolower($manualportForward);

		$results = array(
			'manual_port_forward' => $manualportForward,
			'manual_external_http_port' => $manualExternalHttpPort,
			'manual_external_https_port' => $manualExternalHttpsPort,
		);
		$this->generateSuccessOutput(200, 'manual_port_forward', $results, $output_format);
		
	}	
		
	function put($urlPath, $queryParams=null, $output_format='xml') {

		$manualPortForward = isset($queryParams['manual_port_forward']) ? trim($queryParams['manual_port_forward']) : null;

		if (empty($manualPortForward)) {
			$this->generateErrorOutput(400, 'manual_port_forward', 'PARAMETER_MISSING', $output_format);
			return;
		}

		if (strcasecmp($manualPortForward,"TRUE") == 0){
			$manualExternalHttpPort = isset($queryParams['manual_external_http_port']) ? trim($queryParams['manual_external_http_port']) : null;
			$manualExternalHttpsPort = isset($queryParams['manual_external_https_port']) ? trim($queryParams['manual_external_https_port']) : null;
			
			if (empty($manualExternalHttpPort) || empty($manualExternalHttpsPort)) {
				$this->generateErrorOutput(400, 'manual_port_forward', 'PARAMETER_MISSING', $output_format);
				return;
			}
			
			//Check for http and https port numbers
			if (!is_numeric($manualExternalHttpPort) || !is_numeric($manualExternalHttpsPort)) {
				$this->generateErrorOutput(400, 'manual_port_forward', 'INVALID_PARAMETER', $output_format);
				return;			
			}
			setManualPortForwardConfig($manualPortForward, $manualExternalHttpPort, $manualExternalHttpsPort);			
		}else if(strcasecmp($manualPortForward,"FALSE") == 0){
			clearManualPortForwardConfig();
		}else{
			$this->generateErrorOutput(400, 'manual_port_forward', 'INVALID_PARAMETER', $output_format);
			return;			
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'manual_port_forward', $results, $output_format);
	}
	
}
?>
