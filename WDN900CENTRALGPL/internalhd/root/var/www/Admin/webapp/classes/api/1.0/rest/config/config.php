<?php
// Copyright ?2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Config extends RestComponent {

	function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('util.inc');
	}


	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		//Verify that the request is from the local computer (this service should not be allowed to be called externally; not even on the lan)
		if ($_SERVER['REMOTE_ADDR'] != $_SERVER['SERVER_ADDR']) {
			setHttpStatusCode(401, 'External requests not permitted.');
			return;
		}
		$foundSettings = false;
		if ($queryParams !== null) {
			$paramArray = array();
			if (isset($queryParams['params'])) {
				$paramArray = explode(',',$queryParams['params']);
			} else if (isset($queryParams['param'])) {
				if (!in_array($queryParams['param'],$paramArray)) {
					array_push($paramArray, $queryParams['param']);
				}
			} else {
				setHttpStatusCode(400, 'config param names are missing ');
				return;
			}
			$noParams = sizeof($paramArray);
			if ($noParams > 0) {
				$queryParams['config_id'] = strtoupper($queryParams['config_id']);
				$configId = $queryParams['config_id'];
				if (!isset($configId)) {
					setHttpStatusCode(400, 'config_id is missing');
					return;
				}
				$module = $queryParams['module'];
				if (!isset($module)) {
					setHttpStatusCode(400, 'config module is missing');
					return;
				}
				$configSettings = getConfig(strtoupper($configId),$module);
				if ($configSettings != null) {
					$ow = new OutputWriter(strtoupper($output_format));
					$ow->pushElement('config');
					for ($i=0; $i < $noParams; $i++) {
						if (isset($configSettings[$paramArray[$i]])) {
							$ow->pushElement('param');
							$ow->element('name', $paramArray[$i]);
							$ow->element('value', $this->toString($configSettings[$paramArray[$i]]));
							$ow->popElement();
						}
					}
					$ow->popElement();
					$ow->close();
					$foundSettings = true;
				}
			}
		}
		if (!$foundSettings) {
			setHttpStatusCode(404, 'Not Found');
		}
	}


	function put($urlPath, $queryParams=null, $output_format='xml')
	{
		//Verify that the request is from the local computer (this service should not be allowed to be called externally; not even on the lan)
		if ($_SERVER['REMOTE_ADDR'] != $_SERVER['SERVER_ADDR']) {
			setHttpStatusCode(401, 'External requests not permitted.');
			return;
		}

		//Verify that both of the required parameters are included

		if (empty($queryParams)) {
			setHttpStatusCode(400, 'params are missing');
			return;
		}
		if (!isset($queryParams['config_id'])) {
			setHttpStatusCode(400, 'config_id is missing');
			return;
		}
		if (!isset($queryParams['module'])) {
			setHttpStatusCode(400, 'module is missing');
			return;
		}
		$queryParams['config_id'] = strtoupper($queryParams['config_id']);
		$configId = $queryParams['config_id'];
		$module = $queryParams['module'];
		$paramsSet=array();
		$changedArr = array( 'configId' => $configId,
							 'section'  => $module,
							 'name'     => array());		

	 	if (($paramsSize=(sizeof($queryParams))) > 1 ){
	 		$configId = strtoupper($configId);
	 		$config = getConfig($configId, $module);
			if (!empty($config)) {

				foreach($queryParams as $key => $val) {					
					if ( !preg_match('/format|config_id|module|rest_method|auth_username|auth_password/i', $key)) {
						$changedArr['name'][$key] = $val;
						$paramsSet[$key]=$val;								
					}
				}
			
				// save it in ini file
  				if(!setConfigArray($changedArr)) $paramsSet = null;		
				

				$ow = new OutputWriter(strtoupper($output_format));
				$ow->pushElement('config');
		 		if (isset($paramsSet)) {
	 				//echo-back params and their values
					$ow->element('status','success');
					$keys = array_keys($paramsSet);
					$sizeKeys = sizeof($keys);
					for ($i=0; $i < $sizeKeys; $i++) {
						$ow->pushElement( 'param');
						$ow->element('name', $keys[$i]);
						$ow->element('value', $paramsSet[$keys[$i]]);
						$ow->popElement();
					}
	 			} else {
					setHttpStatusCode(404, 'Not Found');
				}
				$ow->popElement();
				$ow->close();
			} else {
				setHttpStatusCode(404, 'Not Found');
			}
	 	} else {
			setHttpStatusCode(404, 'Not Found');
		}
	}


	function toString($value)
	{
		if (!is_array($value)) {
			return $value;
		}
		$values = '';
		$size = sizeof($value);
		for ($i = 0; $i < $size; $i++) {
			str_replace(',','\,',$value[$i]);
			$values = $values . $value[$i];
			if ($i < $size - 1) {
				 $values = $values . ',';
			}
		}
		return $values;
	}
}
?>