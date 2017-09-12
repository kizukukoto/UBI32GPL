<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class AlbumItem extends RestComponent {

	function __construct() {
		require_once('album.inc');
		require_once('albumitem.inc');
		require_once('outputwriter.inc');
		require_once('util.inc');
	}


	/**
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		if (isset($urlPath[0])) {
			$albumItemId = $urlPath[0];
			$userId = getSessionUserId();
			if (isAdmin($userId) || isAlbumItemAccessible($albumItemId)) {
				$albumItem = getAlbumItem($albumItemId);
				if (isset($albumItem[0])) {
					$this->generateItemOutput(200, 'album_item', $albumItem[0], $output_format);
				} else {
					$this->generateErrorOutput(500, 'album_item', 'ALBUM_ITEM_GET_FAILED', $output_format);
				}
			} else {
				$this->generateErrorOutput(403, 'album_item', 'USER_NOT_AUTHORIZED', $output_format);
			}
		} else if (isset($queryParams['album_id'])) {
			$albumId = $queryParams['album_id'];
			$albumItems = getAlbumItems($albumId);
			$this->generateCollectionOutput(200, 'album_items', 'album_item', $albumItems, $output_format);
		} else {
			$this->generateErrorOutput(400, 'album_item', 'ALBUM_ITEM_ID_MISSING', $output_format);
		}
	}


	/**
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {
		$albumId   = isset($queryParams['album_id'])   ? $queryParams['album_id']   : null;
		$fileId    = isset($queryParams['file_id'])    ? $queryParams['file_id']    : null;
		$itemOrder = isset($queryParams['item_order']) ? $queryParams['item_order'] : null;

		if (empty($albumId)) {
			$this->generateErrorOutput(400, 'album_item', 'PARAMETER_MISSING', $output_format);
			return;
		}
		if (empty($fileId)) {
			$this->generateErrorOutput(400, 'album_item', 'PARAMETER_MISSING', $output_format);
			return;
		}
		$userId = getSessionUserId();

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'userId', $userId);

		if (!isAdmin($userId) && !isAlbumOwner($userId, $albumId) && !isAlbumAccessible($albumId)){
			$this->generateErrorOutput(403, 'album_item', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		$albumItemId = createAlbumItem($albumId, $fileId, $itemOrder);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumItemId', $albumItemId);

		if ($albumItemId == -1) {
			$this->generateErrorOutput(500, 'album_item', 'ALBUM_ITEM_CREATE_FAILED', $output_format);
			return;
		}

		$results = array('status' => 'success', 'album_item_id' => $albumItemId);
		$this->generateSuccessOutput(201, 'album_item', $results, $output_format);
	}


	/**
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {
		$albumItemId = $urlPath[0];
		if (!isset($albumItemId) ){
				$this->generateErrorOutput(400, 'album_item', 'ALBUM_ITEM_ID_MISSING', $output_format);
		} else {
			$itemOrder = $queryParams['item_order'];
			$userId = getSessionUserId();
			if (isAdmin($userId) || isAlbumItemAccessible($albumItemId)) {
				$status = updateAlbumItem($albumItemId, $itemOrder);
				if ($status == true) {
					$results = array('status' => 'success', 'album_item_id' => $albumItemId);
					$this->generateSuccessOutput(200, 'album_item', $results, $output_format);
				} else {
					$this->generateErrorOutput(500, 'album_item', 'ALBUM_ITEM_UPDATE_FAILED', $output_format);
				}
			 }else{
				$this->generateErrorOutput(403, 'album_item', 'USER_NOT_AUTHORIZED', $output_format);
			}
		}
	}


	/**
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function delete($urlPath, $queryParams=null, $output_format='xml') {
		$albumItemId = $urlPath[0];
		if (!isset($albumItemId)) {
			$this->generateErrorOutput(400, 'album_item', 'ALBUM_ITEM_ID_MISSING', $output_format);
		} else {
			$userId = getSessionUserId();
			if (isAdmin($userId) || isAlbumItemAccessible($albumItemId)) {
				$status = deleteAlbumItem($albumItemId);
				if ($status == true){
					$results = array('status' => 'success', 'album_item_id' => $albumItemId);
					$this->generateSuccessOutput(200, 'album_item', $results, $output_format);
				} else {
					$this->generateErrorOutput(500, 'album_item', 'ALBUM_ITEM_DELETE_FAILED', $output_format);
				}
			} else {
				$this->generateErrorOutput(403, 'album_item', 'USER_NOT_AUTHORIZED', $output_format);
			}
		}
	}
}
?>