<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

class RestComponent {

	function __construct()
	{
		require_once('util.inc');
		require_once('outputwriter.inc');
	}

	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		setHttpStatusCode(405,'GET is not supported');
	}

	function post($urlPath, $queryParams=null, $output_format='xml')
	{
		setHttpStatusCode(405,'POST is not supported');
	}

	function put($urlPath, $queryParams=null, $output_format='xml')
	{
		setHttpStatusCode(405, 'PUT is not supported');
	}

	function delete($urlPath, $queryParams=null, $output_format='xml')
	{
		setHttpStatusCode(405, 'DELETE is not supported');
	}

	/**
	 *
	 * @param $compName
	 * @param $itemName
	 * @param $items
	 * @param $output_format
	 */
	function generateCollectionOutput($statusCode, $compName, $itemName, $items, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);
		$output->pushArray($itemName);
		foreach($items as $key => $item){
			//$output->pushElement($itemName);
			$output->pushArrayElement();
			foreach($item as $key => $val){
				$output->element($key, $val);
			}
			//$output->popElement();
			$output->popArrayElement();
		}
		$output->popArray();
		$output->popElement();
		$output->close();
	}

	/**
	 *
	 * @param $compName
	 * @param $itemName
	 * @param $items
	 * @param $output_format
	 */
	function generateCollectionOutputWithType($statusCode, $compName, $itemName, $items, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);
		$output->pushArray($itemName);
		foreach($items as $key => $item){
			//$output->pushElement($itemName);
			$output->pushArrayElement();
			foreach($item as $key => $val){
				if($val['TYPE'] === 'NUMBER'){
					$output->numberElement($key, $val['VALUE']);
				}else{
					$output->element($key, $val['VALUE']);
				}
			}
			//$output->popElement();
			$output->popArrayElement();
		}
		$output->popArray();
		$output->popElement();
		$output->close();
	}


	/**
	 * Generates output for multiple collections.
	 *
	 * @param $compName
	 * @param $collectionList is list with name and collection
	 * @param $output_format
	 */
	function generateMultipleCollectionOutputWithType($statusCode, $compName, $collectionList, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);

		foreach($collectionList as $collectionName => $collectionItems){
			//$output->pushElement($collectionName);
			$output->pushArray($collectionName);
			foreach($collectionItems as $key => $infoItem){
				foreach($infoItem as $itemKey => $itemVal){
					//$output->pushElement($key);
					$output->pushArrayElement();
					foreach($itemVal as $elmKey => $elmVal){
						if($elmVal['TYPE'] === 'NUMBER'){
							$output->numberElement($elmKey, $elmVal['VALUE']);
						}else{
							$output->element($elmKey, $elmVal['VALUE']);
						}
					}
					//$output->popElement();
					$output->popArrayElement();
				}
			}
			//$output->popElement();
			$output->popArray();
		}
		$output->popElement();
		$output->close();
	}


	/**
	 * Generates output for multiple collections.
	 *
	 * @param $compName
	 * @param $collectionList is list with name and collection
	 * @param $output_format
	 */
	function generateMultipleCollectionOutputWithTypeAndCollectionName($statusCode, $compName, $collectionList, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);

		foreach($collectionList as $collectionName => $collectionItems){
			$output->pushElement($collectionName);
			//$output->pushArray($collectionName);
			foreach($collectionItems as $key => $infoItem){
				foreach($infoItem as $itemKey => $itemVal){
					$output->pushElement($key);
					//$output->pushArrayElement();
					foreach($itemVal as $elmKey => $elmVal){
						if(is_numeric($elmVal)){
							$output->numberElement($elmKey, $elmVal);
						}else{
							$output->element($elmKey, $elmVal);
						}
					}
					$output->popElement();
					//$output->popArrayElement();
				}
			}
			$output->popElement();
			//$output->popArray();
		}
		$output->popElement();
		$output->close();
	}


	/**
	 *
	 * @param $compName
	 * @param $itemName
	 * @param $items
	 * @param $output_format
	 */
	function generateItemOutput($statusCode, $itemName, $item, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($itemName);
		foreach($item as $key => $val){
			$output->element($key, $val);
		}
		$output->popElement();
		$output->close();
	}

	/**
	 *
	 * @param $compName
	 * @param $itemName
	 * @param $items
	 * @param $output_format
	 */
	function generateItemOutputWithType($statusCode, $itemName, $item, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($itemName);
		foreach($item as $key => $val){
			if($val['TYPE'] === 'NUMBER'){
				$output->numberElement($key, $val['VALUE']);
			}else{
				$output->element($key, $val['VALUE']);
			}
		}
		$output->popElement();
		$output->close();
	}	

	/**
	 * generateSuccessOutput
	 *
	 * @param $compName Name of the component
	 * @param $elmValues Array of Name and Value pairs to generate output
	 * @param $output_format
	 */
	function generateSuccessOutput($statusCode, $compName, $elmValues, $output_format)
	{
		setHttpStatusCode($statusCode);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);
		foreach($elmValues as $key => $value){
			$output->element($key, $value);
		}
		$output->popElement();
		$output->close();
	}


	/**
	 * Compile and generate the error elements.
	 *
	 * @param $status_code
	 * @param $comp_name
	 * @param $error_name
	 * @param $output_format
	 */
	function generateErrorOutput($statusCode, $compName, $errorName, $output_format)
	{
		$comp       = getComponentCodes($compName);
		$error      = getErrorCodes($errorName);
		$compCode   = isset($comp['code'])        ? $comp['code']        : '';
		$errorCode  = isset($error['error_code']) ? $error['error_code'] : '';
		$statusName = setHttpStatusCode($statusCode, '', $compCode, $errorCode);

		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement($compName);
		$output->element('error_code', $statusCode);
		if ($error) {
			$output->element('error_message', $error['error_desc']);
		} else {
			$output->element('error_message', $errorName);
		}
		$output->popElement();
		$output->close();
	}
	
	function getPutData(){
		if(strtoupper($_SERVER['REQUEST_METHOD']) === 'PUT'){
			parse_str(file_get_contents("php://input"), $putData);
			return $putData;
		}
		return null;
	}
}
?>