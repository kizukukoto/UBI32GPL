<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Dir extends RestComponent {

	function _construct()
	{
		require_once('dirxml.inc');
		require_once('globalconfig.inc');
		require_once('util.inc');
	}


	/**
	 * Creates directory under specifed share.
	 * Supports recursive directories.
	 * eg : /Public/a/b/c, creates nested directories.
	 *
	 * @param unknown_type $urlPath
	 * @param unknown_type $queryParams
	 * @param unknown_type $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {
		$shareName = $urlPath[0];
		if (!isset($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
		} else {
			$urlPath = array_slice($urlPath, 1);
			$dirPath = implode('/', $urlPath);

			//Look up path to share
			$directoriesConfig = getGlobalConfig('directories');
			$sharesPath = $directoriesConfig['SHARES_PATH'];
			$dirFullPath = $sharesPath . '/' . $shareName . '/' . $dirPath;
			if (mkdir($dirFullPath,0,true)) {
				$results = array('status' => 'success');
				$this->generateSuccessOutput(201, 'dir', $results, $output_format);
			} else {
				$this->generateErrorOutput(500, 'dir', 'DIR_CREATE_FAILED'.$dirPath, $output_format);
			}
		}
	}


	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		switch($output_format) {
			case 'xml'	:
				$result =  $this->getXmlOutput($urlPath, $queryParams);
				break;
			case 'json' :
				$result =  $this->getJsonOutput($urlPath, $queryParams);
				break;
			case 'text' :
				$result =  $this->getTextOutput($urlPath, $queryParams);
				break;
		};

		if ( ($result === NULL) || (sizeof($result) == 0)) {
			setHttpStatusCode(404);
		}
		echo($result);
	}


	function getXmlOutput($urlPath, $queryParams)
	{
		if ($urlPath !== NULL && (sizeof($urlPath) > 0))
		{
			$getChildList = false;
			$js_childlist = $queryParams['child_list'];

			$config = getGlobalConfig('directories');
			if (isset($config))
			{
				if (isset($js_childlist))
				{
					$getChildList = stristr($js_childlist,'true') ? true : false;
				}
				$dirPath = implode('/', $urlPath);
				if ($getChildList)
				{
					$dirXML = genDirContentsXML($dirPath);
				}
				else
				{
					$dirXML = genDirTreeXMLRecursive($dirPath);
				}
				return ($dirXML);
			}
		}
		return NULL;
	}

	function getJsonOutput($urlPath, $queryParams)
	{
		echo('getJsonOutput function called!!!');
	}

	function getTextOutput($urlPath, $queryParams)
	{
		echo('getTextOutput function called!!!');
	}
}
?>