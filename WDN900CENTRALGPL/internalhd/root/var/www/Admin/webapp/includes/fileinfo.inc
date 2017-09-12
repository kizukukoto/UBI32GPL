<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');

function generateFileInfoList($dbPath, $requestPath, $startTime, $columns, $recursive, $fileRowOffset, $fileRowCount, $output, $sharePath, $includeHidden){

	$requestPath = sqlite_escape_string($requestPath);

	if ($recursive !== 'true') {
		$sql = "SELECT * FROM Folders WHERE path = '$requestPath'";
		//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
		$mediaDb = openMediaDb($dbPath);
		$dbaccess = new DBAccess();
		$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));
		closeMediaDb($mediaDb);
		$folder_id = $rows[0]['folder_id'];
	}

	$sql = "SELECT ".$columns." FROM Files WHERE 1 = 1 ";

	if (!empty($requestPath)) {
		if ($recursive !== 'true') {
			$sql .= " AND (folder_id = '$folder_id' OR path = '$requestPath')";
		} else {
			$sql .= " AND (path LIKE '".$requestPath."/%' OR path = '$requestPath')";
		}
		
	} 

	if (!empty($startTime)) {
		$sql .= " AND last_updated_time >= '$startTime'";
	} else {
		$sql .= " AND is_deleted = 0";
	}

	
	if(!$includeHidden){
		$sql .= " AND is_hidden = 0 ";
	}
	
	if (!empty($fileRowOffset) && !empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowOffset, $fileRowCount";
	} else if (empty($fileRowOffset) && !empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowCount";
	}else if (!empty($fileRowOffset) && empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowOffset, 1000";
	}

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);

	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$stmt = $dbaccess->executeQueryWithDbCursor($mediaDb, $sql);
	if($stmt){
		$output->pushArray('files');
		while ($row = $stmt->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT)) {
			$output->pushArrayElement();

			$item = mapItem($row, $sharePath);
			foreach($item as $elmKey => $elmVal){
				if($elmVal['TYPE'] === 'NUMBER'){
					$output->numberElement($elmKey, $elmVal['VALUE']);
				}else{
					$output->element($elmKey, $elmVal['VALUE']);
				}
			}

			$output->popArrayElement();
		}
		$output->popArray();
	}
	closeMediaDb($mediaDb);
}


function generateDirInfoList($dbPath, $requestPath, $startTime, $columns, $recursive, $singleDir, $dirRowOffset, $dirRowCount, $output, $sharePath, $includeHidden){

	$requestPath = sqlite_escape_string($requestPath);

	if ($recursive !== 'true') {
		$sql = "SELECT * FROM Folders WHERE path = '$requestPath'";
		$mediaDb = openMediaDb($dbPath);
		$dbaccess = new DBAccess();
		$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		closeMediaDb($mediaDb);
		$folder_id = $rows[0]['folder_id'];
	}

	$sql = " SELECT ".$columns." FROM Folders";
	$sql .= " WHERE 1 = 1 ";
	
	if ($singleDir) {
		$sql .= " AND (path = '$requestPath' )";
	} else if (!empty($requestPath)) {
		if ($recursive !== 'true') {
			$sql .= " AND (path = '$requestPath' OR parent_id = '$folder_id')";
		} else {
			$sql .= " AND (path LIKE '".$requestPath."/%' OR path = '$requestPath')";
		}
	} 
	
	if (!empty($startTime)) {
		$sql .= " AND last_updated_time >= '$startTime'";
	} else {
		$sql .= " AND is_deleted = 0";
	}
	

	if(!$includeHidden){
		$sql .= " AND name not like '.%' ";
	}

	if (!empty($dirRowOffset) && !empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowOffset, $dirRowCount";
	} else if (empty($dirRowOffset) && !empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowCount";
	}else if (!empty($dirRowOffset) && empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowOffset, 1000";
	}

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$stmt = $dbaccess->executeQueryWithDbCursor($mediaDb, $sql);
	if($stmt){
		$output->pushArray('dirs');
		while ($row = $stmt->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT)) {
			$output->pushArrayElement();

			$item = mapItem($row, $sharePath);
			foreach($item as $elmKey => $elmVal){
				if($elmVal['TYPE'] === 'NUMBER'){
					$output->numberElement($elmKey, $elmVal['VALUE']);
				}else{
					$output->element($elmKey, $elmVal['VALUE']);
				}
			}

			$output->popArrayElement();
		}
		$output->popArray();
	}
	closeMediaDb($mediaDb);
}

function getFileInfoList($dbPath, $requestPath, $startTime, $columns, $recursive, $fileRowOffset, $fileRowCount){

	$requestPath = sqlite_escape_string($requestPath);

	if ($recursive !== true) {
		$sql = "SELECT * FROM Folders WHERE path = '$requestPath'";
		$mediaDb = openMediaDb($dbPath);
		$dbaccess = new DBAccess();
		$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		closeMediaDb($mediaDb);
		$folder_id = $rows[0]['folder_id'];
	}

	$sql = "SELECT ".$columns." FROM Files";

	if (!empty($requestPath)) {
		if ($recursive !== 'true') {
			$sql .= " WHERE folder_id = '$folder_id'";
		} else {
			$sql .= " WHERE path LIKE '".$requestPath."%'";
		}
		if (!empty($startTime)) {
			$sql .= " AND last_updated_time >= ".$startTime;
		}
	} else {
		if (!empty($startTime)) {
			$sql = $sql." WHERE last_updated_time >= '$startTime'";
		}
	}

	if (!empty($fileRowOffset) && !empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowOffset, $fileRowCount";
	} else if (empty($fileRowOffset) && !empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowCount";
	} else if (!empty($fileRowOffset) && empty($fileRowCount)) {
		$sql = $sql." LIMIT $fileRowOffset, 1000";
	}

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);

	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
	closeMediaDb($mediaDb);

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));

	return $rows;
}


function getDirInfoList($dbPath, $requestPath, $startTime, $columns, $recursive, $dirRowOffset, $dirRowCount){

	$requestPath = sqlite_escape_string($requestPath);

	if ($recursive !== true) {
		$sql = "SELECT * FROM Folders WHERE path = '$requestPath'";
		$mediaDb = openMediaDb($dbPath);
		$dbaccess = new DBAccess();
		$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		closeMediaDb($mediaDb);
		$folder_id = $rows[0]['folder_id'];
	}

	$sql = "SELECT ".$columns." FROM Folders";

	if (!empty($requestPath)) {
		if ($recursive !== 'true') {
			$sql .= " WHERE path = '$requestPath' OR parent_id = '$folder_id'";
		} else {
			$sql = $sql." WHERE path LIKE '".$requestPath."%'";
		}
		if (!empty($startTime)) {
			$sql = $sql." AND last_updated_time >= '$startTime'";
		}
	} else {
		if (!empty($startTime)) {
			$sql = $sql." WHERE last_updated_time >= '$startTime'";
		}
	}

	if (!empty($dirRowOffset) && !empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowOffset, $dirRowCount";
	} else if (empty($dirRowOffset) && !empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowCount";
	} else if (!empty($dirRowOffset) && empty($dirRowCount)) {
		$sql = $sql." LIMIT $dirRowOffset, 1000";
	}

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);

	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
	closeMediaDb($mediaDb);

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));

	return $rows;
}


function mapItem($fileInfo, $sharePath){
	if(isset($fileInfo)){
		$item = array();
		// Set is deleted
		if(isset($fileInfo['is_deleted'])){
			$deleted = 'false';
			if($fileInfo['is_deleted'] == 1)
				$deleted = 'true';
			$item['deleted'] = array('TYPE'=>'STRING', 'VALUE'=>$deleted);
		}

		// Set path
		if(isset($fileInfo['path'])){
			$path = str_replace($sharePath, "", $fileInfo['path']);
			$fileInfoArr = explode('/',$fileInfo['path']);
			
			if(count($fileInfoArr) > 0){
				unset($fileInfoArr[count($fileInfoArr)-1]);
			}
			$newPath = implode('/',$fileInfoArr);
			//$path = str_replace('/'.$fileInfo['name'], "", $path);
			//$item['path'] = array('TYPE'=>'STRING', 'VALUE'=>$path);
			$item['path'] = array('TYPE'=>'STRING', 'VALUE'=>$newPath);
		}

		// Set name
		if(isset($fileInfo['name'])){
			$item['name'] = array('TYPE'=>'STRING', 'VALUE'=>$fileInfo['name']);
		}

		// Set size
		if(isset($fileInfo['size'])){
			$item['size'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fileInfo['size']);
		}

		// Set modified
		if(isset($fileInfo['modified_time'])){
			$item['modified'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fileInfo['modified_time']);
		}

		// Set last_updated
		if(isset($fileInfo['last_updated_time'])){
			$item['last_updated_time'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fileInfo['last_updated_time']);
		}

		return $item;
	}
	return null;
}

function mapFileItem($fileInfo, $sharePath){
	if(isset($fileInfo)){
		$deleted = 'false';
		if($fileInfo['is_deleted'] == 1)
			$deleted = 'true';
		$path = str_replace($sharePath, "", $fileInfo['path']);
		$item = array(
			'path'  => $path,
			'name'  => $fileInfo['name'],
			'size'=> $fileInfo['size'],
			'modified' => $fileInfo['modified_time'],
			'last_updated' => $fileInfo['last_updated_time'],
			'deleted' => $deleted
			);
		return $item;
	}
	return null;
}


function mapDirItem($dirInfo, $sharePath){
	if(isset($dirInfo)){
		$deleted = 'false';
		if($dirInfo['is_deleted'] == 1)
			$deleted = 'true';
		$path = str_replace($sharePath, "", $dirInfo['path']);
		$item = array(
			'path'  => $path,
			'name'  => $dirInfo['name'],
			'modified' => $dirInfo['modified_time'],
			'last_updated' => $fileInfo['last_updated_time'],
			'deleted' => $deleted
			);
		return $item;
	}
	return null;
}


function getDirSummary($dbPath, $path=null, $recursive=null, $startTime=null, $category=null, $includeHidden=false){

	$path = sqlite_escape_string($path);

	if ($recursive !== 'true') {
		$sql = "SELECT * FROM Folders WHERE path = '$path'";
		//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
		$mediaDb = openMediaDb($dbPath);
		$dbaccess = new DBAccess();
		$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
		closeMediaDb($mediaDb);
		//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));
		$folder_id = $rows[0]['folder_id'];
		//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'folder_id', $folder_id);
	}

	$sql = "SELECT COUNT(path) as file_count, SUM(size) as size FROM Files WHERE 1 = 1 ";

	if (!empty($path)) {
		if ($recursive !== 'true') {
			$sql .= " AND folder_id = '$folder_id'";
		} else {
			$sql .= " AND path LIKE '".$path."/%'";
		}
		
	}

	if (!empty($startTime)) {
		$sql .= " AND last_updated_time >= '$startTime'";
	} else {
		$sql .= " AND is_deleted = 0 ";
	}

	if (!empty($category)) {
		$sql .= " AND category = ".$category;
	}
	
	if (!$includeHidden) {
		$sql .= " AND is_hidden = 0";
	}

	
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);

	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
	closeMediaDb($mediaDb);

	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));

	$results['file_count'] = $rows[0]['file_count'];
	$results['size']       = $rows[0]['size'];
	return $results;
}


function getDirSummaryByFileType($dbPath, $path=null, $recursive=true, $startTime=null, $mediaType=null){

	$path = sqlite_escape_string($path);

	$sql  = "SELECT COUNT(*) as file_count, SUM(size) as size FROM Files fi, Folders fo";
	$sql .= " WHERE fi.folder_id = fo.folder_id";
	if (!empty($path)) {
		if ($recursive) {
			$sql .= " AND fo.name LIKE '".$path."/%'";
		} else {
			$sql .= " AND fo.name = '".$path."'";
		}
		if ($startTime) {
			$sql .= " AND fi.modified_date >= ".$startTime;
		}
		if ($mediaType) {
			switch($mediaType) {
				case 'photos':
				case 'image':
				$sql .= " AND (fi.name LIKE '%.gif' OR fi.name LIKE '%.jpg' OR fi.name LIKE '%.png')";
				break;

				case 'music':
				case 'audio':
				$sql .= " AND (fi.name LIKE '%.mp3' OR fi.name LIKE '%.wav')";
				break;

				case 'videos':
				case 'video':
				$sql .= " AND (fi.name LIKE '%.mov' OR fi.name LIKE '%.mmv' OR fi.name LIKE '%.mp4')";
				break;

				case 'docs':
				$sql .= " AND (fi.name LIKE '%.doc OR fi.name LIKE '%.doc' OR fi.name LIKE '%.doc')";
				break;

				case 'apps':
				case 'application':
				$sql .= " AND (fi.name LIKE '%.exe OR fi.name LIKE '%.msi' OR fi.name LIKE '%.sh')";
				break;

				case 'text':
				$sql .= " AND fi.name LIKE '%.txt'";
				break;

				default:
				break;
			}
		}
	}
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
	closeMediaDb($mediaDb);
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));
	$results['file_count'] = !empty($rows[0]['file_count']) ? $rows[0]['file_count'] : 0;
	$results['size']       = !empty($rows[0]['size'])       ? $rows[0]['size']       : 0;
	return $results;
}


function getDirSummaryByMimeType($dbPath, $path=null, $recursive=true, $startTime=null, $mediaType=null){

	$path = sqlite_escape_string($path);

	$sql = "SELECT COUNT(path) as file_count, SUM(size) as size FROM Files";
	if (!empty($path)) {
		if ($recursive) {
			//$sql .= " WHERE path LIKE '%".$path."%'";
			$sql .= " WHERE path LIKE '".$path."/%'";
		} else {
			$sql .= " WHERE path='".$path."'";
		}
		if ($startTime) {
			$sql .= " AND last_updated_time >= ".$startTime;
		}
		if ($mediaType) {
			//$sql .= " AND media_type = ". $mediaType;
			switch($mediaType) {
				case 'photos':
				case 'image':
				$sql .= " AND mime_type LIKE 'image%'";
				break;

				case 'music':
				case 'audio':
				$sql .= " AND mime_type LIKE 'audio%'";
				break;

				case 'videos':
				case 'video':
				$sql .= " AND mime_type LIKE 'video%'";
				break;

				case 'docs':
				$sql .= " AND (mime_type LIKE 'text%' OR mime_type LIKE 'application%')";
				break;

				case 'apps':
				case 'application':
				$sql .= " AND mime_type LIKE 'application%'";
				break;

				case 'message':
				$sql .= " AND mime_type LIKE 'message%'";
				break;

				case 'text':
				$sql .= " AND mime_type LIKE 'text%'";
				break;

				case 'x-world':
				$sql .= " AND mime_type LIKE 'x-world%'";
				break;

				default:
				break;
			}
		}
	}
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$rows = $dbaccess->executeQueryWithDb($mediaDb, $sql);
	closeMediaDb($mediaDb);
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'rows', print_r($rows,true));
	$results['file_count'] = $rows[0]['file_count'];
	$results['size']       = $rows[0]['size'];
	return $results;
}


function getLastPurgeTime($dbPath){
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'dbPath', $dbPath);
	$xmlFile   = substr($dbPath, 0, -3) . '-volume.xml';
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'xmlFile', $xmlFile);
	if (!file_exists($xmlFile)) {
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'xmlFile NOT FOUND!', $xmlFile);
		return 0;
	}
	$xml = @simplexml_load_file($xmlFile);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'xml', print_r($xml,true));
	###<MediaCrawler><Version>1.0</Version><Volume><last_updated_time>1301986798</last_updated_time><last_purge_time>1301986798</last_purge_time></Volume></MediaCrawler>
	$lastPurgeTime = isset($xml->Volume->last_purge_time) ? $xml->Volume->last_purge_time : null;
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'lastPurgeTime', $lastPurgeTime);
	return $lastPurgeTime;
}


function getRangeItems($sharePath, $infoList, $rowOffset, $rowCount, $isFiletype){
	$items = array();
	$rowIndex = 0;
	$resultStartIndex = $rowOffset-1;
	$resultRowCount = 0;
	foreach($infoList as $info) {
		if($rowIndex >= $resultStartIndex){
			if($isFiletype){
				$item = mapItem($info, $sharePath);
			}else{
				$item = mapItem($info, $sharePath);
			}
			array_push($items, $item);
			$resultRowCount++;
			if($rowCount == $resultRowCount)
				break;
		}
		$rowIndex++;
	}
	return $items;
}

function generateFileInfo($dbPath, $fileId, $startTime=null, $columns=null, $offset=null, $count=null, $output, $sharePath) {
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'dbPath', $dbPath);
	if (!empty($columns)) {
		$sql = "SELECT ".$columns." FROM Files";
	} else {
		$sql = "SELECT * FROM Files";
	}
	$sql .= " WHERE file_id = '$fileId'";
	if (!empty($startTime)) {
		$sql .= " AND last_updated_time >= '$startTime'";
	}
	if (!empty($offset) && !empty($count)) {
		$sql = $sql." LIMIT $offset, $count";
	} else if (empty($offset) && !empty($count)) {
		$sql = $sql." LIMIT $count";
	} else if (!empty($offset) && empty($count)) {
		$sql = $sql." LIMIT $offset, 1000";
	}
	//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
	$mediaDb = openMediaDb($dbPath);
	$dbaccess = new DBAccess();
	$stmt = $dbaccess->executeQueryWithDbCursor($mediaDb, $sql);
	if($stmt){
		$output->pushArray('files');
		while ($row = $stmt->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT)) {
			$output->pushArrayElement();
			$item = mapItem($row, $sharePath);
			//printf("<PRE>%s.%s=[%s]<PRE>\n", __FUNCTION__, 'sql', $sql);
			foreach ($item as $elmKey => $elmVal) {
				if ($elmVal['TYPE'] === 'NUMBER') {
					$output->numberElement($elmKey, $elmVal['VALUE']);
				} else {
					$output->element($elmKey, $elmVal['VALUE']);
				}
			}
			$output->popArrayElement();
		}
		$output->popArray();
	}
	closeMediaDb($mediaDb);
}
?>