<?php
require_once("dbaccess.inc");

class IndexerDB extends DBAccess {
	
	static $fileFieldsQuery = "SELECT file_id, folder_id, name, modified_date, created_date, width, height FROM Files";
	
	function IndexerDB(){
		require_once("dbaccess.inc");
	}
	
	function getFileIndexFields() {
		
		$bindVarNVTArray = array();
		$rows = $this->executeQuery(self::$fileFieldsQuery, 'FILE_INDEXER_QUERY', $bindVarNVTArray);
		return $rows;	
	}

}

?>