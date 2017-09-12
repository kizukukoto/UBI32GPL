<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('albumsdb.inc');
require_once('albumitemsdb.inc');

function isAlbumAccessible($albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->isAlbumAccessible($albumId);
}

function isAlbumValid($albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->isAlbumValid($albumId);
}

function isAlbumOwner($userId, $albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->isAlbumOwner($userId, $albumId);
}

function createAlbum($owner, $name, $description, $backgroundImage, $backgroundColor, $expiredDate, $previewImage){
	$albumsDb = new AlbumsDB();
	$albumId = $albumsDb->createAlbum($owner, $name, $description, $backgroundImage, $backgroundColor, $expiredDate, $previewImage);
	return $albumId;
}

function deleteAlbum($albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->deleteAlbum($albumId);
}

function updateAlbum($albumId, $name, $description, $bkgdImage, $bkgdColor, $expiredDate, $previewImage){
	$albumsDb = new AlbumsDB();
	return $albumsDb->updateAlbum($albumId, $name, $description, $bkgdImage, $bkgdColor, $expiredDate, $previewImage);
}

function getAlbum($userId, $albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->getAlbum($userId, $albumId);
}

function getAlbums($userId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->getAlbums($userId);
}

function getAllAlbums(){
	$albumsDb = new AlbumsDB();
	return $albumsDb->getAllAlbums();
}

function getOneAlbum($albumId){
	$albumsDb = new AlbumsDB();
	return $albumsDb->getOneAlbum($albumId);
}

/*
function getAlbumInfo($albumId){

	$AlbumItemsDB = new AlbumItemsDB();

	$albumItems = $AlbumItemsDB->getAlbumItems($albumId);

	printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumItems', print_r($albumItems,true));

	$volInfo   = getMediaVolumesInfo();
	$dbPath    = isset($volInfo[0]['DbPath']) ? $volInfo[0]['DbPath'] : '';
	$lastPurgeTime = getLastPurgeTime($dbPath);

	$abumInfo = array();
	foreach ($albumItems as $albumItem) {


		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement('metadb_info');
		$output->element('generated_time', time());
		$output->element('last_purge_time',$lastPurgeTime);
		$fileList = generateFileInfoList($dbPath, $path, $startTime, $fileColumns, $recursive, $fileRowOffset, $fileRowCount, $output, $sharePath);
		$output->popElement();
		$output->close();

		$abumInfo[] = $albumItem;
	}

	return $abumInfo;
}
*/

?>