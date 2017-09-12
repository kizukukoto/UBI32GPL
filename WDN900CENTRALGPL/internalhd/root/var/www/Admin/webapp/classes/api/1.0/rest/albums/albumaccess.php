<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class AlbumAccess extends RestComponent {

	function __construct()
	{
		require_once('contents.inc');
		require_once('globalconfig.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$albumId = $urlPath[0];

		if (!isset($albumId)) {
			$this->generateErrorOutput(400, 'album_access', 'ALBUM_ID_MISSING', $output_format);
			return;
		}

		try {

			### TODO: Develop this function
			//$albumAccess = getAlbumAccess($albumId);
			//$results = array('album_access' => $albumAccess);
			//$this->generateSuccessOutput(201, 'album_access', $results, $output_format);

		} catch (Exception $e) {
			$this->generateErrorOutput(500, 'album_access', 'ALBUM_ACCESS_FAILED', $output_format);
		}
	}
}
?>