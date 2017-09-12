<?php
	// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
	require_once('albumsdb.inc');
	require_once('albumitemsdb.inc');

	function createAlbumItem($albumId, $fileId, $itemOrder=null){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->createAlbumItem($albumId, $fileId, $itemOrder);
	}

	function updateAlbumItem($albumItemId, $itemOrder){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->updateAlbumItem($albumItemId, $itemOrder);
	}

	function deleteAlbumItem($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->deleteAlbumItem($albumItemId);
	}

	function getAlbumItem($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		$albumItem = $albumItemsDb->getAlbumItem($albumItemId);
		return $albumItem;
	}

	function getAlbumItems($albumId){
		$albumItemsDb = new AlbumItemsDB();
		$albumItems = $albumItemsDb->getAlbumItems($albumId);
		return $albumItems;
	}
	function getAlbumId($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->getAlbumId($albumItemId);
	}

	function getFilePath($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->getFilePath($albumItemId);
	}

	function isAlbumItemAccessible($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->isAlbumItemAccessible($albumItemId);
	}

	function isAlbumItemValid($albumItemId){
		$albumItemsDb = new AlbumItemsDB();
		return $albumItemsDb->isAlbumItemValid($albumItemId);
	}
?>