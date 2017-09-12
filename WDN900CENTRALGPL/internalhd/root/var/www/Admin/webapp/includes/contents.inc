<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('albumitemsdb.inc');
require_once('security.inc');
require_once('transcoding.inc');


/**
 * Verifies a user's permission to access the requested AlbumItem and then serves the data from that AlbumItem.
 * @param integer $albumItemId Primary key of the album_item table in the SQLite database which identifies an individual
 * item within an album.
 * @return no return value
 */
function readFileFromAlbumItem($albumItemId, $transcodingType){

	$albumItemsDb = new AlbumItemsDB();

	//determine whether the user has access to this albumItem
	if(!$albumItemsDb->isAlbumItemAccessible($albumItemId)){
		throw new Exception('USER_NOT_AUTHORIZED', 401);
	}

	//find the absolute path to this file's contents from the DB
	$filePath = $albumItemsDb->getFilePath($albumItemId);

	if(empty($filePath) || !file_exists($filePath)){
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'FILE_NOT_FOUND: filePath', $filePath);
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'filePath', $filePath);

	readFileFromPath($filePath, $transcodingType);
}


/**
 * This function takes an uploaded file and writes it to the specified location.
 * Function gives error response (e.g. 404) in the case where it fails.
 * @param $shareName Share to which the file is to be written.  The current session must have write permission for this share. Examples: "public", "eric".
 * @param $relativePath Location relative to the share where the file is to be written (e.g. "dir1/dir2/foo.txt").
 * @param $overwrite Boolean indicating whether any existing file with the same name should be overwritten.
 * @return none
 */
function writeFileFromShare($shareName, $relativePath, $overwrite, $mtime=null){

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'shareName', $shareName);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'relativePath', $relativePath);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'overwrite', $overwrite);

	if(!isShareAccessible($shareName, true)){
		throw new Exception('USER_NOT_AUTHORIZED', 401);
	}

	if(!isPathLegal($relativePath)){
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	$directoriesConfig = getGlobalConfig("directories");
	$sharesPath = $directoriesConfig["SHARES_PATH"];

	$fullPath = $sharesPath . '/' . $shareName . '/' . $relativePath;
	//echo "Path: " . $fullPath."<br>";

	$status = writeFileFromPath($fullPath, $overwrite);
	if(isset($mtime)){
		@touch($fullPath, $mtime);
	}
	return $status;
}


/**
 * Deletes the specified file from the specified share.
 * @param $shareName Share to which the file is to be deleted.  The current session must have write permission for this share. Examples: "public", "eric".
 * @param $relativePath Location relative to the share where the file is to be deleted (e.g. "dir1/dir2/foo.txt").
 * @return none
 */
function deleteFileFromShare($shareName, $relativePath){

	if(!isShareAccessible($shareName, true)){
		//We could use 401, but we don't even want to let a hacker know which shares exist
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	if(!isPathLegal($relativePath)){
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	$directoriesConfig = getGlobalConfig("directories");
	$sharesPath = $directoriesConfig["SHARES_PATH"];

	$file = $sharesPath . '/' . $shareName . '/' . $relativePath;

	if(!file_exists($file)){
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	$status = @unlink($file);

	if (!$status) {
		throw new Exception('FILE_DELETE_FAILED', 500);
	}
}


/**
 * Reads the specified file from the specified share.
 * @param $shareName Share to which the file is to be read.  The current session must have read permission for this share. Examples: "public", "eric".
 * @param $relativePath Location relative to the share where the file is to be read (e.g. "dir1/dir2/foo.txt").
 * @return none
 */
function readFileFromShare($shareName, $relativePath, $transcodingType){

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'shareName', $shareName);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'relativePath', $relativePath);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __FUNCTION__, 'transcodingType', $transcodingType);

	if(!isShareAccessible($shareName, false)){
		//We could use 401, but we don't even want to let a hacker know which shares exist
		//die("404 File not found!");
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	if(!isPathLegal($relativePath)){
		//die("404 File not found!");
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	//Look up path to share
	$directoriesConfig = getGlobalConfig("directories");
	$sharesPath = $directoriesConfig["SHARES_PATH"];
	//$sharesPath = "C:/Shares";

	//Form full file path
	$fullPath = $sharesPath . '/' . $shareName . '/' . $relativePath;
	//$fullPath = "C:/shares/public/dir1/test.html";
	//echo "Path: " . $fullPath."<br>";

	readFileFromPath($fullPath, $transcodingType);
}


/**
 * Downloads the file from the specified absolute path using mod x-send, if it is installed,
 * else uses slightly slower PHP-native download (the old version of this function)
 * @return none
 */
function readFileFromPath($file, $transcodingType=null){

	$offcie2007_mime_types = array(
	    "docx" => "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
	    "docm" => "application/vnd.ms-word.document.macroEnabled.12",
	    "dotx" => "application/vnd.openxmlformats-officedocument.wordprocessingml.template",
	    "dotm" => "application/vnd.ms-word.template.macroEnabled.12",
	    "xlsx" => "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
	    "xlsm" => "application/vnd.ms-excel.sheet.macroEnabled.12",
	    "xltx" => "application/vnd.openxmlformats-officedocument.spreadsheetml.template",
	    "xltm" => "application/vnd.ms-excel.template.macroEnabled.12",
	    "xlsb" => "application/vnd.ms-excel.sheet.binary.macroEnabled.12",
	    "xlam" => "application/vnd.ms-excel.addin.macroEnabled.12",
	    "pptx" => "application/vnd.openxmlformats-officedocument.presentationml.presentation",
	    "pptm" => "application/vnd.ms-powerpoint.presentation.macroEnabled.12",
	    "ppsx" => "application/vnd.openxmlformats-officedocument.presentationml.slideshow",
	    "ppsm" => "application/vnd.ms-powerpoint.slideshow.macroEnabled.12",
	    "potx" => "application/vnd.openxmlformats-officedocument.presentationml.template",
	    "potm" => "application/vnd.ms-powerpoint.template.macroEnabled.12",
	    "ppam" => "application/vnd.ms-powerpoint.addin.macroEnabled.12",
	    "sldx" => "application/vnd.openxmlformats-officedocument.presentationml.slide",
	    "sldm" => "application/vnd.ms-powerpoint.slide.macroEnabled.12",
	    "one" => "application/onenote",
	    "onetoc2" => "application/onenote",
	    "onetmp" => "application/onenote",
	    "onepkg" => "application/onenote",
	    "thmx" => "application/vnd.ms-officetheme",
        "mp3" =>    "audio/mpeg"
	);
	
	$modules = apache_get_modules();
	if (!in_array('mod_xsendfile', $modules)) {
		//mod x-send is not installed
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'file', $file);
		return readFileFromPathOld($file, $transcodingType);
	}
	if (!empty($transcodingType)) {
		$transcodedFilePath = getTranscodedFilePath($file, $transcodingType);
		if ($transcodedFilePath == 400){
			throw new Exception('INVALID_PARAMETER_VALUE', 400);
		}
		if (empty($transcodedFilePath)){
			throw new Exception('FILE_NOT_FOUND', 404);
		}
		$file = $transcodedFilePath;
	}
	if (!file_exists($file)) {
		throw new Exception('FILE_NOT_FOUND', 404);
	}

	$path_info = pathinfo($file);
	if(array_key_exists($path_info['extension'], $offcie2007_mime_types)){
		$ctype = $offcie2007_mime_types[$path_info['extension']];
	}
	if(!isset($ctype )){
		$ctype = mime_content_type($file);
	}
	
	$filename = end(explode('/', $file));

	header ('X-Sendfile: ' . $file);
	header ('Content-Type: ' . $ctype);
	header ('Content-Disposition: attachment; filename="' . $filename . '"');
}


/**
 * Reads the file from the specified absolute path.  Accepts header options Content-Range and Content-Length.  Sets the mime-type.
 * @param $file the absolute path to the file to be read
 * @return none
 */
function readFileFromPathOld($file, $transcodingType=null){

	if (!file_exists($file)) {
		throw new Exception('NOT_FOUND: File does not exist! ['.$file.']', 404);
	}

	if (isset($transcodingType)) {
		$transcodedFilePath = getTranscodedFilePath($file, $transcodingType);
		if (!isset($transcodedFilePath)){
			throw new Exception('NOT_FOUND: Transcoded file not found! ['.$file.']', 404);
		}
		$file = $transcodedFilePath;
		if (!file_exists($file)) {
			throw new Exception('NOT_FOUND: Transcoded file does not exist! ['.$file.']', 404);
		}
	}

	$filesize = filesize($file);
	$filename = basename($file);
	$file_ext = strtolower(substr(strrchr($filename,"."),1));

	$ctype = mime_content_type($file);

	/*
	 switch($file_ext) {
		case "avi" : $ctype = "video/x-msvideo"; break;
		case "exe" : $ctype = "application/octet-stream"; break;
		case "flv" : $ctype = "video/x-flv"; break;
		case "html": $ctype = "text/html"; break;
		case "jpg" : $ctype = "image/jpeg"; break;
		case "mp3" : $ctype = "audio/mp3"; break;
		case "mpg" : $ctype = "video/mpeg"; break;
		case "zip" : $ctype = "application/zip"; break;
		default    : $ctype = "application/force-download";
	} */

	if (isset($_SERVER['HTTP_RANGE'])) {
		$httpRange = $_SERVER['HTTP_RANGE'];
		if (!preg_match('^bytes=\d*-\d*(,\d*-\d*)*$', $httpRange)) {
			//have to set header here as we have to set the Content-Range for HTTP 416 error code
        	header('HTTP/1.1 416 Requested Range Not Satisfiable');
        	header('Content-Range: bytes */' . $filesize); // Required for 416
        	return false;
    	}

	    list($start, $end) = explode(',', substr($httpRange, 6));
	    if ($start > $end) {
	            header('HTTP/1.1 416 Requested Range Not Satisfiable');
	            header('Content-Range: bytes */' . $filesize); // Required for 416.
	            return false;
    	}
		$contentLength = $end - $start + 1;
	}
	else {
		$start = 0;
		$end = $filesize - 1;
		$contentLength = $filesize;
	}
	$contentRange = $start."-".$end."/".$filesize;

	if ($start > $end) {
		throw new Exception('NOT_SATISFIABLE: Range start is greater than range end!', 416);
	}

	if ($start > $filesize - 1) {
		throw new Exception('NOT_SATISFIABLE: Range start is greater than file length! ['.$filesize.']', 416);
	}

	header("Cache-Control:");
	header("Cache-Control: public");
	header("Content-Type: $ctype");
	header("Accept-Ranges: bytes");
	header("Content-Range: bytes ".$contentRange);
	header("Content-Length: ".$contentLength);

	set_time_limit(0);
	$offset = $start;
	$maxChunkSize = 64 * 1024;
	$bytesRemaining = $contentLength;
	//set script execution time to unlimited so script will continue to run for huge file downloads
	set_time_limit(0);
	while( $bytesRemaining > 0 ) {
		$chunkSize = ($bytesRemaining > $maxChunkSize)? $maxChunkSize : $bytesRemaining;
		$fileChunk = file_get_contents ( $file, NULL, NULL, $offset, $chunkSize );
		if (!$fileChunk && ($offset == $start)) {
			throw new Exception('INTERNAL_ERROR: Failed to get file contents! ['.$file.']', 500);
			set_time_limit(ini_get('max_execution_time'));
		}
		echo($fileChunk);
		$offset += $chunkSize;
		$bytesRemaining -= $chunkSize;
		flush();
	}
	ob_flush();
	set_time_limit(ini_get('max_execution_time'));
	return true;
}


/**
 * Writes the uploaded file to the specified absolute path.
 * @param $file the absolute path to the file to be written
 * @param $overwrite boolean indicating whether an existing file should be overwritten.
 * @return none (any errors are explicitely output)
 */
function writeFileFromPath($path, $overwrite){

	if (is_array($_FILES['file']['name'])) {
		$FILES = $_FILES;
	} else {
		$FILES['file']['name'][0]     = $_FILES['file']['name'];
		$FILES['file']['type'][0]     = $_FILES['file']['type'];
		$FILES['file']['tmp_name'][0] = $_FILES['file']['tmp_name'];
		$FILES['file']['error'][0]    = $_FILES['file']['error'];
		$FILES['file']['size'][0]     = $_FILES['file']['size'];
	}

	for ($i=0; $i < count($FILES['file']['name']); $i++) {

		if (empty($FILES['file']['name'][$i])) continue;

		$name    = $FILES['file']['name'][$i];
		$type    = $FILES['file']['type'][$i];
		$tmpFile = $FILES['file']['tmp_name'][$i];
		$error   = $FILES['file']['error'][$i];
		$size    = $FILES['file']['size'][$i];

		if ($error > 0) {
			throw new Exception('FILE_UPLOAD_ERROR', 400);
		}

		if ($size > 1024*1024*1024) {
			throw new Exception('FILE_TOO_LARGE', 403);
		}

		if (!isPathLegal($name)) {
			throw new Exception('FILE_NOT_FOUND', 404);
		}

		if (file_exists($path) && is_dir($path)) {
			$file = $path . '/' . $name;
		} else {
			$file = $path;
		}

		if (file_exists($file) && !$overwrite) {
			throw new Exception('FILE_EXISTS', 403);
		}

		if (isset($_SERVER['HTTP_RANGE'])) {
			list($start, $end) = explode('-', $_SERVER['HTTP_RANGE']);

			if (!file_exists($file)) {

				if ($start > 0) {
					throw new Exception('INVALID_RANGE', 416);
				}

				if (!$fout = fopen($file, 'w')) {
					throw new Exception('FILE_WRITE_FAILED', 500);
				}

			} else {

				if ($start > filesize($file)) {
					throw new Exception('INVALID_RANGE', 416);
				}

				### OPEN OUTPUT FILE FOR READ AND WRITE
				if (!$fout = fopen($file, 'r+')) {
					throw new Exception('FILE_READ_WRITE_FAILED', 500);
				}

				### TRUNCATE OUTPUT FILE FROM START POINT
				if (!ftruncate($fout, $start)) {
					throw new Exception('FILE_TRUNCATE_FAILED', 500);
				}

				### SEEK TO START POINT OF OUTPUT FILE
				if (fseek($fout, $start) == -1) {
					throw new Exception('FILE_SEEK_FAILED', 500);
				}
			}

			if (!$fin = fopen($tmpFile, 'r')) {
				throw new Exception('FILE_READ_FAILED', 500);
			}

			$size = 0;
			while (!feof($fin)) {
				$line = fread($fin, 8192);
				$size += fwrite($fout, $line);
			}
			fclose($fout);
			fclose($fin);

		} else {

			if (file_exists($file) && is_file($file) && !unlink($file)) {
				throw new Exception('DELETE_FILE_FAILED', 500);
			}

			if (!$status = move_uploaded_file($tmpFile, $file)) {
				throw new Exception('MOVE_FILE_FAILED', 500);
			}
		}
	}
	return true;
}
?>