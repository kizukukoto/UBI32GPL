<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class AlbumItemInfo extends RestComponent {

	function __construct()
	{
		require_once('contents.inc');
		require_once('globalconfig.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$albumItemId = $urlPath[0];

		if (!isset($albumItemId)) {
			$this->generateErrorOutput(400, 'album_item_info', 'ALBUM_ITEM_ID_MISSING', $output_format);
			return;
		}

		try {

			### TODO: Develop this function
			//$albumItemInfo = getAlbumItemInfo($albumItemId);
			//$results = array('album_item_info' => $albumItemInfo);
			//$this->generateSuccessOutput(201, 'album_item_info', $results, $output_format);

		} catch (Exception $e) {
			$this->generateErrorOutput(500, 'album_item_info', 'ALBUM_ITEM_INFO_FAILED', $output_format);
		}
	}
}
?>