<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');
require_once('fileUtil.inc');
class File extends RestComponent {

	function __construct() {
		require_once('contents.inc');
		require_once('dir.inc');
		require_once('globalconfig.inc');
		require_once('util.inc');
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
		$shareName = $urlPath[0];
		if (!isset($shareName)) {
			$this->generateErrorOutput(400, 'file', 'SHARE_NAME_MISSING', $output_format);
		} else {
			$urlPath      = array_slice($urlPath, 1);
			$dirPath      = implode('/', $urlPath);
			$sharePath    = getSharePath();
			$fileFullPath = $sharePath . '/' . $shareName . '/' . $dirPath;
			$requestPath  =  '/' . $shareName . '/' . $dirPath;

			$includePermissions = isset($queryParams['include_permissions'])  ? trim($queryParams['include_permissions'])  : false;
			$includePermissions = ($includePermissions === 'true' || $includePermissions === '1'  || $includePermissions === 1) ? true : false;

			$attributes   = getAttributes($fileFullPath, $requestPath, null, $includePermissions);
			if (!$attributes) {
				$this->generateErrorOutput(404, 'file', 'FILE_NOT_FOUND', $output_format);
				return;
			}
			$this->generateItemOutputWithType(200, 'file', $attributes, $output_format);
		}
	}


	/**
	 * Change the name of specified file.
	 * Security checks: Verifies path is legal.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {
		$sharePath  = getSharePath();
		$shareName    = $urlPath[0];
		$newPaths     = explode('/', stripslashes($queryParams['new_path']));
		$newPathShare = $newPaths[0];
		$newPath      = $sharePath . '/' . stripslashes($queryParams['new_path']);
		$urlPath      = array_slice($urlPath, 1);
		$dirPath      = implode('/', $urlPath);
		$currentRequestPath  = '/' . $shareName . '/' . $dirPath;
		$currentPath  = $sharePath . $currentRequestPath;
		$newDirs = $newPaths;
		array_pop($newDirs);
		$newDirPath   = $sharePath . '/' . implode('/', $newDirs);
		$mtime		= $queryParams['mtime'];

		$doCopy = isset($queryParams['copy'])  ? trim($queryParams['copy'])  : false;
		$doCopy = ($doCopy === 'true' || $doCopy === '1'  || $doCopy === 1) ? true : false;

		$overwrite = isset($queryParams['overwrite'])  ? trim($queryParams['overwrite'])  : false;
		$overwrite = ($overwrite === 'true' || $overwrite === '1'  || $overwrite === 1) ? true : false;

		if (!isset($shareName)) {
			$this->generateErrorOutput(400, 'file', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if(!isset($mtime) && !isset($queryParams['new_path'])) {
			$this->generateErrorOutput(400, 'file', 'INVALID_PARAMETER', $output_format);
			return;
		}

		if (!file_exists($currentPath)) {
			$this->generateErrorOutput(404, 'file', 'FILE_NOT_FOUND', $output_format);
			return;
		}

		if (is_dir($currentPath)) {
			$this->generateErrorOutput(404, 'file', 'PATH_NOT_FILE', $output_format);
			return;
		}

		if (!isPathLegal($currentPath)) {
			$this->generateErrorOutput(404, 'file', 'PATH_NOT_VALID', $output_format);
			return;
		}

		// Setting mtime for existing file.
		if(isset($mtime)){
			if(!is_numeric($mtime)){
				$this->generateErrorOutput(400, 'file', 'INVALID_MTIME', $output_format);
				return;
			}

			$status = @touch($currentPath, $mtime);
			if (!$status) {
				$this->generateErrorOutput(500, 'file', 'MTIME_UPDATE_FAILED', $output_format);
				return;
			}
		}

		if (isset($queryParams['new_path'])) {

			if (strpos($newPath, ':') !== false ||
				strpos($newPath, '<') !== false ||
				strpos($newPath, '>') !== false ||
				strpos($path, '\\') !== false ||
				strpos($newPath, '|') !== false) {
				$this->generateErrorOutput(400, 'file', 'INVALID_CHARACTER', $output_format);
				return;
			}

			if (!file_exists($sharePath . '/' . $newPathShare)) {
				$this->generateErrorOutput(403, 'dir', 'INVALID_SHARE', $output_format);
				return;
			}

			if (!file_exists($newDirPath)) {
				$this->generateErrorOutput(404, 'file', 'DIR_NOT_EXIST', $output_format);
				return;
			}

			// $overwrite is true delete existing
			if($overwrite && file_exists($newPath)){
				$status = @unlink($newPath);
			}

			if (file_exists($newPath)) {
				$this->generateErrorOutput(403, 'file', 'FILE_EXISTS', $output_format);
				return;
			}

			if (!isPathLegal($newPath)) {
				$this->generateErrorOutput(404, 'file', 'INVALID_PATH', $output_format);
				return;
			}
			set_time_limit(0);
			if( $doCopy ) {
				
				$status = orionCopy($currentPath, $newPath);
				if (!$status) {
					$this->generateErrorOutput(500, 'file', 'COPY_FILE_FAILED', $output_format);
					return;
				}
			} else {
				$status = @rename($currentPath, $newPath);
				if (!$status) {
					$this->generateErrorOutput(500, 'file', 'MOVE_FILE_FAILED', $output_format);
					return;
				}
			}
		}

		$results = array('status' => 'success');
		if (!empty($mtime)) {
        	$attributes   = getAttributes($currentPath, $currentRequestPath, null, null);
            $results['mtime'] = $attributes['mtime']['VALUE'];
        }
		$this->generateSuccessOutput(200, 'file', $results, $output_format);
	}


	/**
	 * Deletes file under specifed share.
	 * Security checks: Verifies valid share.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function delete($urlPath, $queryParams=null, $output_format='xml') {
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

		try {
			deleteFileFromShare($shareName, $filePath);
			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'file', $results, $output_format);
		} catch(Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'file', $e->getMessage(), $output_format);
		}
	}
}
?>