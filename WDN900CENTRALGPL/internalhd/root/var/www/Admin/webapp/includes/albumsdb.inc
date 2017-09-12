<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');

class AlbumsDB extends DBAccess {

	function __construct(){}

	/**
	 *
	 * @param integer $userId
	 * @param integer $albumId
	 */
	function getAlbum($userId, $albumId) {
		$sql = "SELECT  Albums.album_id as album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
			FROM Albums
			LEFT JOIN AlbumShares ON Albums.album_id = AlbumShares.album_id
			WHERE (AlbumShares.user_id = :userId OR Albums.owner = :userId)
			  AND Albums.album_id = :albumId";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = array(
			array(':userId', $userId, PDO::PARAM_INT),
			array(':albumId', $albumId, PDO::PARAM_INT)
		);
		$album = $this->executeQuery($sql, 'GET_ALBUM', $bindVarNVTArray);
		return $album;
	}


	/**
	 * Returns albums user has access
	 *
	 * @param integer $userId
	 */
	function getAlbums($userId) {
		$sql = "SELECT  Albums.album_id as album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
				FROM Albums
				LEFT JOIN AlbumShares ON Albums.album_id = AlbumShares.album_id
				WHERE AlbumShares.user_id = :userId OR Albums.owner = :userId";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = array(array(':userId', $userId, PDO::PARAM_INT));
		$albums = $this->executeQuery($sql, 'GET_ALBUMS', $bindVarNVTArray);
		return $albums;
	}


	/**
	 * Returns all albums
	 */
	function getAllAlbums() {
		$sql = "SELECT  album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
				FROM Albums";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = null;
		$albums = $this->executeQuery($sql, 'GET_ALBUMS', $bindVarNVTArray);
		return $albums;
	}


	/**
	 * Returns one albums
	 */
	function getOneAlbum($albumId) {
		$sql = "SELECT  album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
				FROM Albums
				WHERE album_id = :albumId";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = array(array(':albumId', $albumId, PDO::PARAM_INT));
		$albums = $this->executeQuery($sql, 'GET_ALBUMS', $bindVarNVTArray);
		return $albums;
	}


	/**
	 * Returns info about an album
	 */
	function getAlbumInfo($albumId) {
		$sql = "SELECT  album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
				FROM Albums
				WHERE album_id = :albumId";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = array(array(':albumId', $albumId, PDO::PARAM_INT));
		$albums = $this->executeQuery($sql, 'GET_ALBUMS', $bindVarNVTArray);
		return $albums;
	}


	/**
	 *
	 * @param $userName
	 * @param $name
	 * @param $description
	 * @param $bkgdImage
	 * @param $bkgdColor
	 * @param $expireDate
	 * @param $creationDate
	 * @param $size
	 * @param $readonly
	 * @param $previewImage
	 */
	function createAlbum($userId, $name, $description, $bkgdImage, $bkgdColor, $expiredDate, $previewImage) {
		$sql = "INSERT
				INTO Albums (name,
							 description,
							 owner,
							 background_image,
							 background_color,
							 expired_date,
							 created_date,
							 preview_image)
				VALUES (:name,
						:description,
						:owner,
						:background_image,
						:background_color,
						:expired_date,
						DATETIME('NOW'),
						:preview_image)";
		$bindVarNVTArray = array(
			array(':name', $name, PDO::PARAM_STR),
			array(':description', $description, PDO::PARAM_STR),
			array(':owner', $userId, PDO::PARAM_INT),
			array(':background_image', $bkgdImage, PDO::PARAM_STR),
			array(':background_color', $bkgdColor, PDO::PARAM_STR),
			array(':expired_date', $expiredDate, PDO::PARAM_STR),
			array(':preview_image', $previewImage, PDO::PARAM_STR)
		);
		return $this->executeInsert($sql, 'INSERT_ALBUM', $bindVarNVTArray);
	}


	/**
	 *
	 * @param $albumId
	 * @param $name
	 * @param $description
	 * @param $bkgdImage
	 * @param $bkgdColor
	 * @param $expiredDate
	 * @param $previewImage
	 */
	function updateAlbum($albumId, $name, $description, $bkgdImage, $bkgdColor, $expiredDate, $previewImage) {
		$sql = "UPDATE Albums SET";
		$colAdded = false;
		$colArray = $this->addColumnToUpdateSQL('name', getSafeDatabaseText($name), $sql, $colAdded);
		$colArray = $this->addColumnToUpdateSQL('description', getSafeDatabaseText($description), $colArray['sql'], $colArray['isColAdded']);
		$colArray = $this->addColumnToUpdateSQL('background_image', getSafeDatabaseText($bkgdImage), $colArray['sql'], $colArray['isColAdded']);
		$colArray = $this->addColumnToUpdateSQL('background_color', getSafeDatabaseText($bkgdColor), $colArray['sql'], $colArray['isColAdded']);
		$colArray = $this->addColumnToUpdateSQL('expired_date', getSafeDatabaseText($expiredDate), $colArray['sql'], $colArray['isColAdded']);
		$colArray = $this->addColumnToUpdateSQL('preview_image', getSafeDatabaseText($previewImage), $colArray['sql'], $colArray['isColAdded']);
		$albumId = getSafeDatabaseText($albumId);
		$sql = $colArray['sql']. " WHERE album_id = $albumId";
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		return $this->executeUpdate($sql);
	}


	/**
	 *
	 * @param $albumId
	 */
	function deleteAlbum($albumId) {
		$sql = "DELETE FROM Albums
				WHERE album_id = :album_id";
		$bindVarNVTArray = array(
			array(':album_id', 1 => $albumId, 2 => PDO::PARAM_INT)
		);
		return $this->executeDelete($sql, 'DELETE_ALBUM', $bindVarNVTArray);
	}


	/**
	 *
	 * @param $albumId
	 */
	function isAlbumAccessible($albumId) {
		$sessionUserId = getSessionUserId();
		$sql = "SELECT count(*)
				FROM AlbumShares
				WHERE album_id=:album_id AND user_id=:user_id";
		$bindVarNVTArray = array(
			array(':album_id', $albumId, PDO::PARAM_INT),
			array(':user_id', $sessionUserId, PDO::PARAM_INT),
		);
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_ALBUM_SHARE', $bindVarNVTArray);
		if(isset($arrayResult[0])){
			$shareExists = $arrayResult[0] > 0;
			return $shareExists;
		}
		return false;
	}


	/**
	 *
	 * @param $albumId
	 */
	function isAlbumValid($albumId) {
		$sql = "SELECT  album_id,
						name,
						description,
						owner,
						background_image,
						background_color,
						expired_date,
						created_date,
						preview_image
				FROM Albums
				WHERE album_id = :albumId";

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);

		$bindVarNVTArray = array(array(':albumId', $albumId, PDO::PARAM_INT));
		$albums = $this->executeQuery($sql, 'GET_ALBUMS', $bindVarNVTArray);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albums', print_r($albums,true));

		if (!isset($albums[0])) {
			return false;
		}
		return true;
	}


	/**
	 *
	 * @param $userId
	 * @param $albumId
	 */
	function isAlbumOwner($userId, $albumId) {
		$sql = "SELECT count(*)
				FROM Albums
				WHERE album_id=:album_id AND owner=:user_id";
		$bindVarNVTArray = array(
			array(':album_id', $albumId, PDO::PARAM_INT),
			array(':user_id', $userId, PDO::PARAM_INT),
		);
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'IS_ALBUM_OWNER', $bindVarNVTArray);
		if(isset($arrayResult[0])){
			$owner = $arrayResult[0] > 0;
			return $owner;
		}
		return false;
	}
}
?>