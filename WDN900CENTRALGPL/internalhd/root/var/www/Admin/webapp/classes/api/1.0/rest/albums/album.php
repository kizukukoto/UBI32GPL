<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Album extends RestComponent {

	function __construct() {
		require_once('album.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get a specified album or all albums
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {

		$sessionUserId = getSessionUserId();

		if (isAdmin($sessionUserId)) {
			$userId = !empty($queryParams['user_id']) ? $queryParams['user_id'] : $sessionUserId;
		} else {
			$userId = $sessionUserId;
		}

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'userId', $userId);

		if (!empty($urlPath[0])) {
			$albumId = $urlPath[0];
			$albums  = getAlbum($albumId, $userId);
		} else if (empty($userId)) {
			$albums  = getAllAlbums();
		} else {
			$albums  = getAlbums($userId);
		}

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albums', print_r($albums,true));

		if ($albums == null) {
			$this->generateErrorOutput(404, 'album', 'ALBUM_NOT_FOUND', $output_format);
		} else {
			$this->generateCollectionOutput(200, 'albums', 'album', $albums, $output_format);
		}
	}


	/**
	 * Creates a new album
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {
		$owner           = getSessionUserId();
		$name            = $queryParams['name'];
		$description     = $queryParams['description'];
		$backgroundImage = $queryParams['background_image'];
		$backgroundColor = $queryParams['background_color'];
		$expiredDate     = $queryParams['expired_date'];
		$previewImage    = $queryParams['preview_image'];

		// TODO: Check for required parameters

		$newAlbumId = createAlbum($owner, $name, $description, $backgroundImage, $backgroundColor, $expiredDate, $previewImage);

		if ($newAlbumId != -1) {
			$results = array('status' => 'success', 'album_id' => $newAlbumId);
			$this->generateSuccessOutput(201, 'album', $results, $output_format);
		} else {
			$this->generateErrorOutput(500, 'album', 'ALBUM_CREATE_FAILED', $output_format);
		}
	}

	/**
	 * Method to modify album.
	 * Security: Only album owner or admin user can modify an album.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {
		$albumId         = $urlPath[0];
		$name            = $queryParams['name'];
		$description     = $queryParams['description'];
		$backgroundImage = $queryParams['background_image'];
		$backgroundColor = $queryParams['background_color'];
		$expiredDate     = $queryParams['expired_date'];
		$previewImage    = $queryParams['preview_image'];
		$userId = getSessionUserId();

		if (isAdmin($userId) || isAlbumOwner($userId, $albumId)) {
			$status = updateAlbum($albumId, $name, $description, $backgroundImage, $backgroundColor, $expiredDate, $previewImage);
			if ($status == true) {
				$results = array('status' => 'success', 'album_id' => $albumId);
				$this->generateSuccessOutput(200, 'album', $results, $output_format);
			} else {
				$this->generateErrorOutput(500, 'album', 'ALBUM_UPDATE_FAILED', $output_format);
			}
		 } else {
			$this->generateErrorOutput(403, 'album', 'USER_NOT_AUTHORIZED', $output_format);
		}
	}


	/**
	 * Method to delete album.
	 * Security : Only album owner or admin user can delete an album.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function delete($urlPath, $queryParams=null, $output_format='xml') {
		$albumId = $urlPath[0];
		$userId = getSessionUserId();

		if (isAdmin($userId) || isAlbumOwner($userId, $albumId)) {
			$status = deleteAlbum($albumId);
			if ($status == true) {
				$results = array('status' => 'success', 'album_id' => $albumId);
				$this->generateSuccessOutput(200, 'album', $results, $output_format);
			} else {
				$this->generateErrorOutput(500, 'album', 'ALBUM_DELETE_FAILED', $output_format);
			}
		} else {
			$this->generateErrorOutput(403, 'album', 'USER_NOT_AUTHORIZED', $output_format);
		}
	}
}
?>