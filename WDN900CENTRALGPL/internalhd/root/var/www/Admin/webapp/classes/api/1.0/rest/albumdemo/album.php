<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

class Albums {

	//format of params : ?albumid=<albumID, _all>&media=<mediatype, _none>,
	//mediattype = 'images','video','audio','files', or a combination, e.g.: 'images+video'

	function __construct()
	{
		require_once('albumitemsdb.inc');
		require_once('albumsdb.inc');
		require_once('mediaRSS.inc');
		require_once('util.inc');
	}

	function createMediaRSSResult($albumId, $media)
	{
		$albumConfig = $GLOBALS['albumConfig'];
		if (strcmp($albumId, $albumConfig['ALLCONTENT']) == 0 && strcmp(strtolower($media), $albumConfig['NOCONTENT']) == 0) {
			$rss = genAlbumsMediaRSS(getSessionUserId());
			return $rss;
		} else {
			if (strlen($albumId ) > 0) {
				$rss = genAlbumItemsMediaRss(getSessionUserId(), $albumId, $media);
				return $rss;
			}
		}
		return null;
	}

	function addToAlbum($urlPath, $queryParams)
	{
		$albumsDB = $GLOBALS['_mrssalbumsdb'];
		$albumConfig = $GLOBALS['albumConfig'];
		$albumItemsDB = $GLOBALS['_mrssalbumitemsdb'];
		if (isset($albumsDB)) {
			$targetAlbum = end($urlPath);
			$sourceFileUrl = $queryParams['file_url'];
			if (isset($sourceFileUrl)) {
				$sourceFilePath = '/' . substr($sourceFileUrl, strlen($albumConfig['URLPREFIX']));
				if (strpos($_SERVER['OS'], 'Windows') !== false) {
					//we are on windows
					$sourceFilePath = 'c:' . $sourceFilePath;
				}
				//http://localhost/home
				echo ('Source file path: ' + $sourceFilePath);
				if (file_exists($sourceFilePath)) {
					//get album id
					$albumData = $albumsDB->getAlbum(getSessionUserId(), $targetAlbum);
					if (!empty($albumData)) {
						$albumId = $albumData[0]['album_id'];
						//get file info
						$finfo = getFileInfo($filepath);
						$itemorder = $GLOBALS['_albumsitemorder'];
						if (!isset($itemorder)) {
							$itemorder = 99000;
						}
						$GLOBALS['_albumsitemorder'] = ++$itemorder;
						$albumItemsDB->createAlbumItem(
						$albumId,
						$finfo['file_path'],
						$finfo['file_id'],
						$finfo['file_size'],
						$finfo['file_created'],
						$finfo['file_modified'],
						$finfo['file_readonly'],
						$finfo['file_owner'] ,
						$finfo['file_isdir'],
						$finfo['title'],
						$finfo['description'],
						$finfo['author'],
						$finfo['mime_type'],
						$finfo['file_path'],
						$finfo['duration'],
						$itemOrder );
						echo('<RESULT>****** ADDED ITEM TO ALBUM: ' . $targetAlbum . ' *******</RESULT>');
					} else {
						header('HTTP/1.0 404 Not Found');
						echo('Album: ' . $targetAlbum . ' not found');
					}
				} else {
					header('HTTP/1.0 404 Not Found');
					echo('File: ' . $sourceFilePath . 'does not exist');
				}
			}
		}
	}


	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		switch($output_format) {
			case 'xml'	:
			case 'rss2'	:
			case 'rss'	:
				$result =  $this->getXmlOutput($urlPath, $queryParams);
				break;
			case 'json' :
				$result =  $this->getJsonOutput($urlPath, $queryParams);
				break;
			case 'text' :
				$result =  $this->getTextOutput($urlPath, $queryParams);
				break;
		}
		if (($result  === NULL) || (sizeof($result) == 0)) {
			header('HTTP/1.0 404 Not Found');
		}
		echo($result);
	}

	function post($urlPath, $queryParams=null, $output_format='xml')
	{
		addToAlbum($urlPath, $queryParams);
	}

	function put($urlPath, $queryParams=null, $output_format='xml')
	{
		echo('Preferences.put function called!!!');
	}

	function delete($urlPath, $queryParams=null, $output_format='xml')
	{
		echo('Preferences.delete function called!!!');
	}

	function getXmlOutput($urlPath, $queryParams)
	{
		$js_albumid = $queryParams['album_id'];
		$js_media = $queryParams['media'];
		if ($js_albumid !== NULL && $js_media !== null) {
			if ( containsAnyCode($js_albumid) ||
				 containsAnyCode($js_media)) {
					return NULL;
			}
			$mediaRss = $this->createMediaRSSResult($js_albumid, $js_media);
			return ($mediaRss);
		}
		return NULL;
	}

	function getJsonOutput($urlPath, $queryParams)
	{
		echo('mediaRSS.getJsonOutput function called!!!');
	}

	function getTextOutput($urlPath, $queryParams)
	{
		echo('mediaRSS.getTextOutput function called!!!');
	}
}
?>