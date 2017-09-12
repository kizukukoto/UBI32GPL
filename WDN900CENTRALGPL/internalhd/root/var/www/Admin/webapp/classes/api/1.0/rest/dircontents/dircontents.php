<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class DirContents extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('dir.inc');
		require_once('zip.inc');
	}

	/**
	 * Get directory listing of specifed share.
	 * Security checks: Verifies readable share and valid path name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {

		$sharePath = getSharePath();
		if (empty($urlPath[0])) {
			array_shift($urlPath);
		}
		$shareName = isset($urlPath[0]) ? trim($urlPath[0]) : null;

		$urlPath     = array_slice($urlPath, 1);
		$dirPath     = implode('/', $urlPath);
		$dirPath     = rtrim($dirPath, '/');
		$fullPath    = $sharePath . '/' . $shareName . '/' . $dirPath;
		$requestPath =  '/' . $shareName . '/' . $dirPath;

		$includeHidden = isset($queryParams['include_hidden'])  ? trim($queryParams['include_hidden'])  : false;
		$includeHidden = ($includeHidden === 'true' || $includeHidden === '1'  || $includeHidden === 1) ? true : false;
		
		if (!isset($shareName)) {
			$this->generateErrorOutput(400, 'dir_contents', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (!file_exists($sharePath.'/'.$shareName)) {
			$this->generateErrorOutput(404, 'dir_contents', 'SHARE_NOT_FOUND', $output_format);
			return;
		}

		if (!isShareAccessible($shareName, false)) {
			$this->generateErrorOutput(401, 'dir_contents', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		if (!isPathLegal($dirPath)) {
			$this->generateErrorOutput(400, 'dir_contents', 'PATH_NOT_VALID', $output_format);
			return;
		}

		if (!file_exists($fullPath)) {
			$this->generateErrorOutput(404, 'dir_contents', 'PATH_NOT_FOUND', $output_format);
			return;
		}

		if (!is_dir($fullPath)) {
			$this->generateErrorOutput(400, 'dir_contents', 'PATH_NOT_DIRECTORY', $output_format);
			return;
		}

		set_time_limit(0);
		
		$fileList = getChildListForDir($fullPath, $requestPath, $includeHidden);
		$zipFileName = basename($fullPath).".zip";
		generateZipSteram($zipFileName, $fileList);
		set_time_limit(ini_get('max_execution_time'));
	}

	/**
	 * Creates directory under specifed share.
	 * Supports recursive directories.
	 * eg : /Public/a/b/c, creates nested directories.
	 * Security checks: Verifies writable share and valid path name.
	 *
	 * @param unknown_type $urlPath
	 * @param unknown_type $queryParams
	 * @param unknown_type $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {

		$sharePath = getSharePath();
		if (empty($urlPath[0])) {
			array_shift($urlPath);
		}
		$shareName = isset($urlPath[0]) ? trim($urlPath[0]) : null;
		$urlPath   = array_slice($urlPath, 1);
		$dirPath   = implode('/', $urlPath);
		$fullPath  = $sharePath . '/' . $shareName . '/' . $dirPath;

		if (empty($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (!file_exists($sharePath . '/' . $shareName)) {
			$this->generateErrorOutput(404, 'dir', 'SHARE_NOT_FOUND', $output_format);
			return;
		}

		if (strpos($dirPath, ':') !== false ||
			strpos($dirPath, '<') !== false ||
			strpos($dirPath, '>') !== false ||
			strpos($dirPath, '|') !== false) {
			$this->generateErrorOutput(400, 'dir', 'INVALID_CHARACTER', $output_format);
			return;
		}

		if (!isShareAccessible($shareName, true)) {
			$this->generateErrorOutput(401, 'dir', 'SHARE_INACCESSIBLE', $output_format);
			return;
		}

		if (empty($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'DIR_NAME_MISSING', $output_format);
			return;
		}

		if (!isPathLegal($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'PATH_NOT_VALID', $output_format);
			return;
		}

		if (is_dir($fullPath)) {
			$this->generateErrorOutput(403, 'dir', 'DIRECTORY_EXISTS', $output_format);
			return;
		}

		if (is_file($fullPath)) {
			$this->generateErrorOutput(403, 'dir', 'FILE_EXISTS', $output_format);
			return;
		}

		if (!@mkdir($fullPath, 0777 ,true)) {
			$this->generateErrorOutput(500, 'dir', 'CREATE_DIR_FAILED', $output_format);
			return;
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(201, 'dir', $results, $output_format);
	}


	/**
	 * Change the name of specified directory.
	 * Security checks: Verifies writable share and valid path name and valid new name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {

		$sharePath = getSharePath();
		if (empty($urlPath[0])) {
			array_shift($urlPath);
		}
		$shareName = isset($urlPath[0]) ? trim($urlPath[0]) : null;

		$recursive = isset($queryParams['recursive'])  ? trim($queryParams['recursive'])  : false;
		$recursive = ($recursive === 'true' || $recursive === '1'  || $recursive === 1) ? true : false;
		
		$doCopy = isset($queryParams['copy'])  ? trim($queryParams['copy'])  : false;
		$doCopy = ($doCopy === 'true' || $doCopy === '1'  || $doCopy === 1) ? true : false;
		
		$overwrite = isset($queryParams['overwrite'])  ? trim($queryParams['overwrite'])  : false;
		$overwrite = ($overwrite === 'true' || $overwrite === '1'  || $overwrite === 1) ? true : false;
		
		$mtime		= $queryParams['mtime'];
		
		
		if (empty($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (!file_exists($sharePath . '/' . $shareName)) {
			$this->generateErrorOutput(404, 'dir', 'SHARE_NOT_FOUND', $output_format);
			return;
		}

		if (!isShareAccessible($shareName, true)) {
			$this->generateErrorOutput(401, 'dir', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		$urlPath = array_slice($urlPath, 1);
		$dirPath = implode('/', $urlPath);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'dirPath', $dirPath);

		if (empty($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'DIR_NAME_MISSING', $output_format);
			return;
		}

		if (!isPathLegal($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'PATH_NOT_VALID', $output_format);
			return;
		}

		$fullPath = $sharePath . '/' . $shareName . '/' . $dirPath;

		if (!file_exists($fullPath)) {
			$this->generateErrorOutput(404, 'dir', 'PATH_NOT_FOUND', $output_format);
			return;
		}

		if (!is_dir($fullPath)) {
			$this->generateErrorOutput(400, 'dir', 'PATH_NOT_DIRECTORY', $output_format);
			return;
		}

			// Setting mtime for existing file.
		if(isset($mtime)){
			if(!is_numeric($mtime)){
				$this->generateErrorOutput(400, 'dir', 'INVALID_MTIME', $output_format);
				return;
			}

			$status = @touch($fullPath, $mtime);
			if (!$status) {
				$this->generateErrorOutput(500, 'dir', 'MTIME_UPDATE_FAILED', $output_format);
				return;
			}
		}

		if (isset($queryParams['new_path'])) {
			$newPath = isset($queryParams['new_path']) ? trim($queryParams['new_path']) : null;
			$newPath = ltrim($newPath, '/');
			$newPath = rtrim($newPath, '/');
	
			if (empty($newPath)) {
				$this->generateErrorOutput(400, 'dir', 'NEW_PATH_MISSING', $output_format);
				return;
			}
	
			if (!isPathLegal($newPath)) {
				$this->generateErrorOutput(400, 'dir', 'NEW_PATH_INVALID', $output_format);
				return;
			}
	
			if (strpos($newPath, ':') !== false ||
				strpos($newPath, '<') !== false ||
				strpos($newPath, '>') !== false) {
				$this->generateErrorOutput(400, 'dir', 'INVALID_CHARACTER', $output_format);
				return;
			}
	
			$newPaths     = explode('/', $newPath);
			$newShareName = $newPaths[0];
	
			if (empty($newShareName)) {
				$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
				return;
			}
	
			if (!file_exists($sharePath . '/' . $newShareName)) {
				$this->generateErrorOutput(404, 'dir', 'SHARE_NOT_FOUND', $output_format);
				return;
			}
	
			if (!isShareAccessible($newShareName, true)) {
				$this->generateErrorOutput(401, 'dir', 'USER_NOT_AUTHORIZED', $output_format);
				return;
			}
	
			$newPaths     = array_slice($newPaths, 1);
			$newDir       = implode('/', $newPaths);
	
			if (empty($newDir)) {
				$this->generateErrorOutput(400, 'dir', 'DIR_NAME_MISSING', $output_format);
				return;
			}
	
			$newDirs      = array_slice($newPaths, 0, -1);
			$newDirsPath  = implode('/', $newDirs);
			$newFullDir   = $sharePath . '/' . $newShareName . '/' . $newDirsPath;
	
			if (!file_exists($newFullDir)) {
				$this->generateErrorOutput(404, 'dir', 'NEW_DIR_NOT_FOUND', $output_format);
				return;
			}
	
			$newFullPath  = $sharePath . '/' . $newPath;
			
			// $overwrite is true delete existing
			if($overwrite){
				rmdirRecursive($newFullPath);
			}
			
			if (file_exists($newFullPath)) {
				$this->generateErrorOutput(403, 'dir', 'NEW_PATH_EXISTS', $output_format);
				return;
			}
	
			if( $doCopy ) {
				if( !$recursive && (getChildCount($fullPath) > 0) ) {
					$this->generateErrorOutput(403, 'dir', 'DIRECTORY_NOT_EMPTY', $output_format);
					return;
				}
				if ( !copyDirRecursive($fullPath, $newFullPath) ) {
					$this->generateErrorOutput(500, 'dir', 'COPY_DIR_FAILED', $output_format);
					return;
				}
			} else {
				if ( !@rename($fullPath, $newFullPath) ) {
					$this->generateErrorOutput(500, 'dir', 'MOVE_DIR_FAILED', $output_format);
					return;
				}
			}
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'dir', $results, $output_format);
	}


	/**
	 * Deletes directory under specifed share.
	 * Security checks: Verifies writable share and valid path name.
	 *
	 * @param $urlPath
	 * @param $queryParams
	 * @param $output_format
	 */
	function delete($urlPath, $queryParams=null, $output_format='xml') {

		$sharePath = getSharePath();
		if (empty($urlPath[0])) {
			array_shift($urlPath);
		}
		$shareName = isset($urlPath[0]) ? trim($urlPath[0]) : null;

		$urlPath     = array_slice($urlPath, 1);
		$dirPath     = implode('/', $urlPath);
		$fullpath    = $sharePath . '/' . $shareName . '/' . $dirPath;
		$requestPath =  '/' . $shareName . '/' . $dirPath;
		
		$recursive = isset($queryParams['recursive'])  ? trim($queryParams['recursive'])  : false;
		$recursive = ($recursive === 'true' || $recursive === '1'  || $recursive === 1) ? true : false;
		
		if (!isset($shareName)) {
			$this->generateErrorOutput(400, 'dir', 'SHARE_NAME_MISSING', $output_format);
			return;
		}

		if (!file_exists($sharePath . '/' . $shareName)) {
			$this->generateErrorOutput(404, 'dir', 'SHARE_NOT_FOUND', $output_format);
			return;
		}

		if (!isShareAccessible($shareName, true)) {
			$this->generateErrorOutput(401, 'dir', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		if (empty($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'DIR_NAME_MISSING', $output_format);
			return;
		}

		if (!isPathLegal($dirPath)) {
			$this->generateErrorOutput(400, 'dir', 'PATH_NOT_VALID', $output_format);
			return;
		}

		if(!file_exists($fullpath)){
			$this->generateErrorOutput(404, 'dir', 'PATH_NOT_FOUND', $output_format);
			return;
		}

		if (!is_dir($fullpath)) {
			$this->generateErrorOutput(403, 'dir', 'PATH_NOT_DIRECTORY', $output_format);
			return;
		}

		if(!$recursive) {
			$dirList = getDirList($fullpath, $requestPath, false);
	
			if (count($dirList) > 0) {
				$this->generateErrorOutput(403, 'dir', 'DIRECTORY_NOT_EMPTY', $output_format);
				return;
			}
	
			if (!@rmdir($fullpath)) {
				$this->generateErrorOutput(500, 'dir', 'DELETE_DIR_FAILED', $output_format);
				return;
			}
		} else {
			if(! rmdirRecursive($fullpath) ) {
				$this->generateErrorOutput(500, 'dir', 'DELETE_DIR_FAILED', $output_format);
				return;
			}
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'dir', $results, $output_format);
	}
}

/*
//test code
$urlpathbits = array();
array_push($urlpathbits, 'joe');
array_push($urlpathbits, 'My Documents');
array_push($urlpathbits, 'fileSystem');
$dirobj = new Dir();
$xmlout = $dirobj->get($urlpathbits);
*/
?>