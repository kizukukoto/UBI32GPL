<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class AlbumInfo extends RestComponent {

	function __construct() {
		require_once('album.inc');
		require_once('fileinfo.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	function get($urlPath, $queryParams=null, $output_format='xml') {

		$albumId = $urlPath[0];
		$userId  = getSessionUserId();

		if (!isset($albumId)) {
			$this->generateErrorOutput(400, 'album_info', 'ALBUM_ID_MISSING', $output_format);
			return;
		}

		if (!isAlbumValid($albumId)) {
			$this->generateErrorOutput(404, 'album_info', 'ALBUM_NOT_FOUND', $output_format);
			return;
		}

		if (!isAlbumAccessible($albumId) && !isAlbumOwner($userId, $albumId) && !isAdmin($userId)) {
			$this->generateErrorOutput(401,'album_info','USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}

		try {
			//$albumInfo = getAlbumInfo($albumId);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumInfo', print_r($albumInfo,true));
			//$albumInfo = isset($albumInfo[0]) ? $albumInfo[0] : false;
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumInfo', print_r($albumInfo,true));


			$AlbumItemsDB = new AlbumItemsDB();
			$albumItems = $AlbumItemsDB->getAlbumItems($albumId);

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumItems', print_r($albumItems,true));

			$volInfo   = getMediaVolumesInfo();
			$dbPath    = isset($volInfo[0]['DbPath']) ? $volInfo[0]['DbPath'] : '';
			$sharePath = isset($volInfo[0]['Path'])   ? $volInfo[0]['Path']   : '';

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'volInfo', print_r($volInfo,true));
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'dbPath', $dbPath);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'basePath', $basePath);

			$startTime = null;
			$columns   = null;
			$offset    = null;
			$count     = null;

			$output = new OutputWriter(strtoupper($output_format));
			$output->pushElement('album_info');

			$abumInfo = array();
			foreach ($albumItems as $albumItem) {

				//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumItems', print_r($albumItem,true));

				$fileId = $albumItem['file_id'];

				$fileInfo = generateFileInfo($dbPath, $fileId, $startTime, $columns, $offset, $count, $output, $sharePath);

				//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'fileInfo', print_r($fileInfo,true));

				/*
				if (isset($fileInfo[0])) {
					$fileInfo[0]['path'] = $basePath.$fileInfo[0]['path'];
					$fileInfo[0]['path'] = str_replace('//', '/', $fileInfo[0]['path']);
					$abumInfo[] = $fileInfo[0];
				}
				*/

			}

			$output->popElement();
			$output->close();

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'abumInfo', print_r($abumInfo,true));


			//$this->generateCollectionOutput(201, 'album_info', 'files', $albumInfo, $output_format);
		} catch (Exception $e) {
			$this->generateErrorOutput(500, 'album_info', 'ALBUM_INFO_FAILED', $output_format);
		}
	}
}
?>