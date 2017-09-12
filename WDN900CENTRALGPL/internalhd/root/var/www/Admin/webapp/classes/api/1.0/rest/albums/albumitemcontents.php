<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class AlbumItemContents extends RestComponent {

	function __construct() {
		require_once('albumitem.inc');
		require_once('contents.inc');
		require_once('globalconfig.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Download transcoded and/or range contents for specifed album item.
	 * Security: Check if user is authorized to access specifed album item.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {

		$albumItemId = $urlPath[0];

		if (!isset($albumItemId)) {
			$this->generateErrorOutput(400, 'album_item_contents', 'ALBUM_ITEM_ID_MISSING', $output_format);
			return;
		}

		if (!isAlbumItemValid($albumItemId)) {
			$this->generateErrorOutput(404, 'album_item_contents', 'ALBUM_NOT_FOUND', $output_format);
			return;
		}

		if (!isAlbumItemAccessible($albumItemId) &&  !isAlbumOwner($userId, $albumId) && !isAdmin($userId)) {
			$this->generateErrorOutput(401,'album_item_contents','USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}

		$tnType = !empty($queryParams['tn_type'])    ? $queryParams['tn_type']    : null;
		$range  = !empty($queryParams['http_range']) ? $queryParams['http_range'] : null;
		if(!empty($range)) $_SERVER['HTTP_RANGE'] = $range;

		try {
			readFileFromAlbumItem($albumItemId, $tnType);
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'album_item_contents', $e->getMessage(), $output_format);
		}
	}
}
?>