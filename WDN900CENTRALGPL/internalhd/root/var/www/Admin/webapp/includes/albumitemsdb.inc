<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');
require_once('security.inc');

class AlbumItemsDB extends DBAccess {


	function __construct(){}


	/**
	 *
	 * @param $albumItemId
	 */
	function getAlbumItem($albumItemId){
		$sql = "SELECT  album_item_id, album_id, item_order,
						Files.file_id as file_id, folder_id, name, modified_date, type, size, readonly,
						tn96s1_status, tn96s2_status, i480s1_status, i1024s1_status,
						width, height, quality, created_date, latitude, longitude
						title, artist, album, year, genre, cover_art_path, format, duration
				FROM AlbumItems, Files
				WHERE album_item_id = :album_item_id AND AlbumItems.file_id = Files.file_id ";
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		$bindVarNVTArray = array(
			array(':album_item_id', $albumItemId, PDO::PARAM_INT)
		);
		$albumItem = $this->executeQuery($sql, 'GET_ALBUM_ITEM', $bindVarNVTArray);
		return $albumItem;
	}


	/**
	 *
	 * @param $albumId Album Id
	 */
	function getAlbumItems($albumId){
		$sql = "SELECT  album_item_id, album_id, item_order,
						Files.file_id file_id, folder_id, name, modified_date, type, size, readonly,
						tn96s1_status, tn96s2_status, i480s1_status, i1024s1_status,
						width, height, quality, created_date, latitude, longitude
						title, artist, album, year, genre, cover_art_path, format, duration
				FROM AlbumItems, Files
				WHERE AlbumItems.file_id = Files.file_id
				  AND album_id = :album_id";
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		$bindVarNVTArray = array( array(':album_id', $albumId, PDO::PARAM_INT));
		$albumItems = $this->executeQuery($sql, 'GET_ALBUM_ITEMS', $bindVarNVTArray);
		return $albumItems;
	}


	/**
	 *
	 * @param $albumId
	 * @param $fileId
	 * @param $itemOrder
	 */
	function createAlbumItem($albumId, $fileId, $itemOrder=null){
		$results = -1;

		if (!empty($itemOrder)) {

			$sql = "INSERT
					INTO AlbumItems (file_id, album_id, item_order)
					SELECT :file_id, :album_id, :item_order
					WHERE 0 < (SELECT COUNT(*)
								FROM AlbumItems
								WHERE album_id = :album_id
								AND (
								(item_order >= :item_order)
								or
								(item_order = :item_order - 1)
							))
					OR 1 = (SELECT COUNT(*) + :item_order FROM AlbumItems WHERE album_id = :album_id)
					";

			$bindVarNVTArray = array(
				array(':file_id', $fileId, PDO::PARAM_INT),
				array(':album_id', $albumId, PDO::PARAM_INT),
				array(':item_order', $itemOrder, PDO::PARAM_INT)
			);

			$results = $this->executeInsert($sql, 'INSERT_ALBUM_ITEM', $bindVarNVTArray);

			if ($results > 0) {
				$rowId = $results;
				$this->shiftAlbumItemsUp(null, $rowid);
			}

		} else {

			$sql = "INSERT
					INTO AlbumItems (file_id, album_id, item_order)
					SELECT :file_id, :album_id, max(item_order) + 1
					FROM AlbumItems
					WHERE album_id = :album_id
					";
			$bindVarNVTArray = array(
				array(':file_id', $fileId, PDO::PARAM_INT),
				array(':album_id', $albumId, PDO::PARAM_INT)
			);

			$results = $this->executeInsert($sql, 'INSERT_ALBUM_ITEM', $bindVarNVTArray);
		}

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'results', $results);

		return $results;
	}


	/**
	 *
	 * @param $fileId
	 * @param $filePath
	 */
	function deleteAlbumItem($albumItemId){
		$this->shiftAlbumItemsDown(null, $albumItemId);
		$sql = "DELETE
				FROM AlbumItems
				WHERE album_item_id = :album_item_id ";
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$ret = $this->executeDelete($sql, 'DELETE_ALBUM_ITEM', $bindVarNVTArray);
		return $ret;
	}


	/**
	 *
	 * @param $albumItemId
	 * @param $itemOrder
	 */
	function updateAlbumItem($albumItemId, $itemOrder){
		$items = $this->albumItemRangeTo($albumItemId,1);
		$albumId = $items[0]['album_id'];
		$curItemOrder = $items[0]['item_order'];
		$sql = "UPDATE AlbumItems SET ";
		$colAdded = false;
		$colArray = $this->addColumnToUpdateSQL('item_order', getSafeDatabaseText($itemOrder), $sql, $colAdded);
		$albumItemId = getSafeDatabaseText($albumItemId);
		$sql = $colArray['sql']. " WHERE  album_item_id = $albumItemId
									AND 0 < ( SELECT COUNT(*)
												FROM AlbumItems
												WHERE album_id = $albumId
												AND (
												  (item_order >= $itemOrder)
												))
									";
		$ret = $this->executeUpdate($sql);
		if (!$ret) {
			return false;
		}
		if ($itemOrder < $curItemOrder) {
			$this->shiftAlbumItemsUp($curItemOrder, $albumItemId);
		} else if ($itemOrder > $curItemOrder) {
			$this->shiftAlbumItemsDown($curItemOrder, $albumItemId);
		}
		return $ret;
	}


	/**
	 * Given the albumItemId, the parent albumId will be returned.
	 * @param $albumItemId primary key of the AlbumItems table
	 * @return integer albumId which is the primary key of the Albums table.
	 */
	function getAlbumId($albumItemId){
		//SELECT album_id FROM AlbumItems WHERE album_item_id=42
		$sql = "SELECT album_id
				FROM AlbumItems
				WHERE album_item_id=:album_item_id ";
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$retVal = false;
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_ALBUM_ID', $bindVarNVTArray);
		if(isset($arrayResult[0])){
			$retVal = $arrayResult[0];
		}
		return $retVal;
	}


	/**
	 * Returns boolean indicating whether the user for the current session has read permission for the specified AlbumItem.
	 * Albums are accessible to the owner of the album as well as anybody with which the album has been shared.
	 * @param $albumItemId primary key of the AlbumItems table which indicates a particular item to be accessed.
	 * @return unknown_type
	 */
	function isAlbumItemAccessible($albumItemId){
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'albumItemId', $albumItemId);
		//Get the owner and the id of the album to which this albumItem belongs
		// SELECT owner, albumId FROM Albums, AlbumItems WHERE AlbumItems.album_id=Albums.album_id AND album_item_id=42;
		// SELECT count(*) from AlbumShares where album_id=5 and user_id="eric"
		$sql = "SELECT owner, AlbumItems.album_id
				FROM Albums, AlbumItems
				WHERE AlbumItems.album_id=Albums.album_id AND album_item_id=:album_item_id ";
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_ALBUM_INFO', $bindVarNVTArray);
		if(isset($arrayResult[0])){
			$owner = $arrayResult[0];
			$albumId = $arrayResult[1];
			//The owner is definitely allowed to access content within the album
			$sessionUserId = getSessionUserId();
			if($owner == $sessionUserId || isAdmin($sessionUserId))
				return true;
			$sql = "SELECT count(*)
					FROM AlbumShares
					WHERE album_id=:album_id AND user_id=:user_id";
			$bindVarNVTArray = array(
				array(':album_id', $albumId, PDO::PARAM_INT),
				array(':user_id', $sessionUserId, PDO::PARAM_INT),
			);
			$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_ALBUM_SHARE', $bindVarNVTArray);
			if(isset($arrayResult[0])){
				$shareExists = $arrayResult[0]>0;
				//echo "AlbumId: ".$albumId." UserId: ".$sessionUserId." Exists: ".$shareExists;
				return $shareExists;
			}
		}
		return false;
	}


	/**
	 * Check if specified albumItemId exists.
	 * @param integer $albumItemId primary key of the AlbumItems table
	 */
	function isAlbumItemValid($albumItemId){
		$sql = "SELECT album_id
				FROM AlbumItems
				WHERE album_item_id = :album_item_id ";
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_ALBUM_ID', $bindVarNVTArray);
		$status = isset($arrayResult[0]) ? true : false;
		return $status;
	}


	/**
	 * Given the albumItemId, the parent albumId will be returned.
	 * @param $albumItemId primary key of the AlbumItems table
	 * @return integer albumId which is the primary key of the Albums table.
	 */
	function getFilePath_OBSOLETE($albumItemId){
		$sql = "SELECT Folders.name||Files.name
				FROM AlbumItems, Files, Folders
				WHERE album_item_id=:album_item_id And AlbumItems.file_id = Files.file_id
				And Folders.folder_id=Files.folder_id";
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$arrayResult = $this->executeQueryAndFetchOneRow($sql, 'GET_FILE_PATH', $bindVarNVTArray);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'arrayResult', print_r($arrayResult,true));
		if(isset($arrayResult[0])){
			$retVal = $arrayResult[0];
		} else {
			$retVal = false;
		}
		return $retVal;
	}


	/**
	 * Get file path of album item.
	 * @param integer $albumItemId
	 * @return string $filePath
	 */
	function getFilePath($albumItemId){
		$fileId = $this->getFileId($albumItemId);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'fileId', $fileId);
		if (empty($fileId)) return false;
		$filePath = $this->getFileIdPath($fileId);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'filePath', $filePath);
		return $filePath;
	}


	/**
	 * Get file id of album item.
	 * @param integer $albumItemId
	 * @return integer $fileId
	 */
	function getFileId($albumItemId){
		$sql = "SELECT *
				FROM AlbumItems
				WHERE album_item_id=:album_item_id";
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		$bindVarNVTArray = array( array(':album_item_id', $albumItemId, PDO::PARAM_INT));
		$row = $this->executeQueryAndFetchOneRow($sql, 'GET_FILE_ID', $bindVarNVTArray);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'row', print_r($row,true));
		if (isset($row['file_id'])) {
			$fileId = $row['file_id'];
		} else {
			$fileId = false;
		}
		return $fileId;
	}

	/**
	 * Get file path of file id.
	 * @return integer $fileId
	 * @return string $filePath
	 */
	function getFileIdPath($fileId){
		$volInfo = getMediaVolumesInfo();
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'volInfo', print_r($volInfo,true));
		$volPath = $volInfo[0]['Path'];
		$dbPath  = $volInfo[0]['DbPath'];
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'dbPath', print_r($dbPath,true));
		$mediaDb = openMediaDb($dbPath);
		//$sql = "SELECT ".$columns." FROM Files WHERE file_id = ".$fileId;
		$sql = "SELECT * FROM Files WHERE file_id = ".$fileId;
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		//$dbaccess = new DBAccess();
		//$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		$rows = $this->executeQueryWithDb($mediaDb, $sql);
		closeMediaDb($mediaDb);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'rows', print_r($rows,true));
		if(isset($rows[0]['path'])){
			$path = $rows[0]['path'];
			$file = $rows[0]['name'];
			//$filePath = $volPath.$path.'/'.$file;
			$filePath = $volPath.$path;
		} else {
			$filePath = false;
		}
		return $filePath;
	}


	function shiftAlbumItemsUp($itemOrderTo, $fromAlbumId){
		if (isset($itemOrderTo)) {
			$sql = "UPDATE AlbumItems
					SET item_order = item_order + 1
					WHERE album_item_id != :album_item_id
					AND item_order >= (SELECT item_order
										FROM  AlbumItems
										WHERE album_item_id = :album_item_id)
					AND item_order < :item_order
					";
			$bindVarNVTArray = array(
				array(':album_item_id', $fromAlbumId, PDO::PARAM_INT),
				array(':item_order', $itemOrderTo, PDO::PARAM_INT)
			);
			return $this->executeUpdateWithPreparedStatements($sql, 'SHIFT_ALBUM_ITEMS_UP_RANGE', $bindVarNVTArray);
		} else {
			$sql = "UPDATE AlbumItems
					SET item_order = item_order + 1
					WHERE album_item_id != :album_item_id
					AND item_order >= (SELECT item_order
										FROM  AlbumItems
										WHERE album_item_id = :album_item_id)
					";
			$bindVarNVTArray = array(
				array(':album_item_id', $fromAlbumId, PDO::PARAM_INT)
			);
			return $this->executeUpdateWithPreparedStatements($sql, 'SHIFT_ALBUM_ITEMS_UP', $bindVarNVTArray);
		}
	}


	function shiftAlbumItemsDown($itemOrderFrom, $toAlbumId){
		if (isset($itemOrderFrom)) {
			$sql = "UPDATE AlbumItems
						SET item_order = item_order - 1
						WHERE album_item_id != :album_item_id
						AND item_order <= (SELECT item_order
											FROM  AlbumItems
											WHERE album_item_id = :album_item_id)
						AND item_order > :item_order
					";
			$bindVarNVTArray = array(
				array(':album_item_id', $toAlbumId, PDO::PARAM_INT),
				array(':item_order', $itemOrderFrom, PDO::PARAM_INT)
			);
			return $this->executeUpdateWithPreparedStatements($sql, 'SHIFT_ALBUM_ITEMS_DOWN_RANGE', $bindVarNVTArray);
		} else {
			$sql = "UPDATE AlbumItems
						SET item_order = item_order - 1
						WHERE album_item_id != :album_item_id
						AND item_order >= (SELECT item_order
											FROM  AlbumItems
											WHERE album_item_id = :album_item_id)
					";
			$bindVarNVTArray = array(
				array(':album_item_id', $toAlbumId, PDO::PARAM_INT),
			);
			return $this->executeUpdateWithPreparedStatements($sql, 'SHIFT_ALBUM_ITEMS_DOWN', $bindVarNVTArray);
		}
	}


	/**
	 * Get list of albums up to (includes) specified album Item
	 * @param $albumItemId
	 * @param $limit number of record to be returned, if not specified, then all results are returned
	 */
	function albumItemRangeTo($albumItemId,  $limit){
		if (!isset($limit)) {
			$limit = -1;
		}
		$sql = "SELECT album_item_id, file_id, album_id, item_order
				  FROM (SELECT * FROM AlbumItems WHERE album_id = (SELECT album_id FROM AlbumItems WHERE album_item_id = :album_item_id)) otr
				  WHERE exists (SELECT 1
				  FROM AlbumItems inr
				 WHERE inr.album_id = otr.album_id
				   AND inr.item_order >= otr.item_order
				   AND inr.album_item_id = :album_item_id)
				 ORDER BY item_order DESC
				 LIMIT :limit
				";
		$bindVarNVTArray = array(
			array(':album_item_id', $albumItemId, PDO::PARAM_INT),
			array(':limit', $limit, PDO::PARAM_INT),
		);
		$ret = $this->executeQuery($sql, 'ALBUM_ITEM_RANGE_TO', $bindVarNVTArray);
		return $ret;
	}
}
?>