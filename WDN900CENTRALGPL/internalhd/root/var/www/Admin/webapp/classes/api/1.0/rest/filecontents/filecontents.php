<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class FileContents extends RestComponent {

	function __construct() {
		require_once('contents.inc');
		require_once('globalconfig.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	/**
	 * FileContents::get - Download transcoded contents for specifed file.
	 * Security checks: Verifies readable share and valid path name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		$sharePath = getSharePath();
		$shareName = $urlPath[0];
		$urlPath   = array_slice($urlPath, 1);
		$filePath  = implode('/', $urlPath);
		$fullPath  = $sharePath . '/' . $shareName . '/' . $filePath;

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sharePath', $sharePath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'shareName', $shareName);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'filePath', $filePath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'fullPath', $fullPath);

		if (empty($shareName)){
			$this->generateErrorOutput(400, 'file_contents', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (empty($filePath)){
			$this->generateErrorOutput(400, 'file_contents', 'FILE_PATH_MISSING', $output_format);
			return;
		}

		if (!file_exists($fullPath)){
			$this->generateErrorOutput(404, 'file_contents', 'FILE_NOT_FOUND', $output_format);
			return;
		}

		if (!is_file($fullPath)){
			$this->generateErrorOutput(400, 'file_contents', 'PATH_NOT_FILE', $output_format);
			return;
		}

		if (isset($queryParams['HTTP_RANGE'])) {
			$_SERVER['HTTP_RANGE'] = $queryParams['HTTP_RANGE'];
		}

		if (!empty($queryParams['http_range'])) {
			$_SERVER['HTTP_RANGE'] = $queryParams['http_range'];
		}

		$transcodingType = !empty($queryParams['tn_type']) ? $queryParams['tn_type'] : null;

		try {
			readFileFromShare($shareName, $filePath, $transcodingType);
		} catch (Exception $e){
			$this->generateErrorOutput($e->getCode(), 'file_contents', $e->getMessage(), $output_format);
		}
	}

	/**
	 * Upload specifed file to device.
	 * Security checks: Verifies readable share and valid path name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {
		$sharePath = getSharePath();
		$shareName = $urlPath[0];
		$urlPath   = array_slice($urlPath, 1);
		$urlDir    = array_slice($urlPath, 0, -1);
		$urlFile   = array_slice($urlPath, -1);
		$newPath   = implode('/', $urlPath);
		$newDir    = implode('/', $urlDir);
		$newFile   = implode('/', $urlFile);
		$dir       = $sharePath.'/'.$shareName.'/'.$newDir;
		$path      = $sharePath.'/'.$shareName.'/'.$newPath;
		$mtime		= $queryParams['mtime'];

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sharePath', $sharePath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'shareName', $shareName);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '$newPath', $newPath);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '$newDir', $newDir);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '$newFile', $newFile);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '$dir', $dir);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '$path', $path);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '_FILES', print_r($_FILES,true));

		if (!isset($_FILES['file']) || (isset($_FILES['file']['error']) && $_FILES['file']['error'] == 4)) {
			$this->generateErrorOutput(400, 'dir', 'SOURCE_FILE_MISSING', $output_format);
			return;
		}

		if (empty($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
			return;
		}


		if (!is_dir($dir)) {
			$this->generateErrorOutput(404, 'dir', 'DIR_NOT_EXIST', $output_format);
			return;
		}

		if (file_exists($path) && !is_dir($path)) {
			$this->generateErrorOutput(403, 'dir', 'FILE_EXISTS', $output_format);
			return;
		}

		if (empty($newDir) && strpos($newFile, '.') === false) {
			$this->generateErrorOutput(404, 'dir', 'DIR_NOT_EXIST', $output_format);
			return;
		}

		if (isset($queryParams['tn_type'])) {
			$this->generateErrorOutput(405, 'file_contents', 'INVALID_PARAMETER', $output_format);
			return;
		}
		if(isset($mtime) && !is_numeric($mtime)){
			$this->generateErrorOutput(400, 'file_contents', 'INVALID_MTIME', $output_format);
			return;
		}

		try {
			$overwrite = 0;
			$status = writeFileFromShare($shareName, $newPath, $overwrite, $mtime);
			if ($status !== true) {
				$this->generateErrorOutput(500, 'file_contents', 'FILE_UPLOAD_FAILED', $output_format);
				return;
			}

			$results = array('status' => 'success');
			$this->generateItemOutput(201, 'file_contents', $results, $output_format);

		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'file_contents', $e->getMessage(), $output_format);
		}
	}

	/**
	 * Modify or replace existing file.
	 * Security checks: Verifies path is legal.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {
		$shareName = $urlPath[0];
		$urlPath   = array_slice($urlPath, 1);
		$urlDir    = array_slice($urlPath, 0, -1);
		$urlFile   = array_slice($urlPath, -1);
		$newPath   = implode('/', $urlPath);
		$newDir    = implode('/', $urlDir);
		$newFile   = implode('/', $urlFile);
		$mtime		= $queryParams['mtime'];

		$directoriesConfig = getGlobalConfig('directories');
		$sharesPath  = $directoriesConfig['SHARES_PATH'];
		$newDirPath = $sharesPath . '/' . $shareName . '/' . $newDir;
		$newFilePath = $sharesPath . '/' . $shareName . '/' . $newPath;

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '_FILES', print_r($_FILES,true));

		if (!isset($_FILES['file']) || (isset($_FILES['file']['error']) && $_FILES['file']['error'] == 4)) {
			$this->generateErrorOutput(400, 'dir', 'SOURCE_FILE_MISSING', $output_format);
			return;
		}

		/*
		if (isset($_FILES['file']['error']) && $_FILES['file']['error'] == 4) {
			$this->generateErrorOutput(400, 'dir', 'SOURCE_FILE_MISSING', $output_format);
			return;
		}
		*/

		if (empty($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
			return;
		}



		if (!is_dir($newDirPath)) {
			$this->generateErrorOutput(404, 'dir', 'DIR_NOT_EXIST', $output_format);
			return;
		}

		if (isset($queryParams['tn_type'])) {
			$this->generateErrorOutput(405, 'file_contents', 'INVALID_PARAMETER', $output_format);
			return;
		}

		if (isset($queryParams['HTTP_RANGE'])) {
			$_SERVER['HTTP_RANGE'] = $queryParams['HTTP_RANGE'];
		}

		if (!empty($queryParams['http_range'])) {
			$_SERVER['HTTP_RANGE'] = $queryParams['http_range'];
		}
		if(isset($mtime) && !is_numeric($mtime)){
			$this->generateErrorOutput(400, 'file_contents', 'INVALID_MTIME', $output_format);
			return;
		}

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, '_SERVER', print_r($_SERVER,true));

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'HTTP_RANGE0', print_r($_SERVER['HTTP_RANGE'],true));

		if (!empty($_SERVER['HTTP_RANGE'])) {

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'HTTP_RANGE', print_r($_SERVER['HTTP_RANGE'],true));

			list($start, $end) = explode('-', $_SERVER['HTTP_RANGE']);
			if (!file_exists($newFilePath) && $start > 0) {
				$this->generateErrorOutput(404, 'dir', 'FILE_NOT_FOUND', $output_format);
				return;
			}
		}

		try {
			$status = writeFileFromShare($shareName, $newPath, $overwrite=1, $mtime);

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', $status);

			if ($status !== true) {
				$this->generateErrorOutput(500, 'file_contents', 'FILE_UPLOAD_FAILED', $output_format);
				return;
			}
			$results = array('status' => 'success');
			$this->generateItemOutput(200, 'file_contents', $results, $output_format);
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'file_contents', $e->getMessage(), $output_format);
		}
	}
	
	private function _invalidFat32Path($newPath) {
			if (strpos($newPath, ':') !== false ||
			strpos($newPath, '<') !== false ||
			strpos($newPath, '>') !== false ||
			strpos($newPath, '\\') !== false ||
			strpos($newPath, '|') !== false) {
			
			return true;
		}
	}
}
?>