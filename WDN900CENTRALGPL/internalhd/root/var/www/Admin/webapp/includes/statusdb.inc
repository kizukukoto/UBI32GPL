<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once("dbaccess.inc");

class StatusDB extends DBAccess{

	
	function StatusDB(){
	}
	
	/**
	 * 
	 */
	function getStatus(){
		// SQL Statement with prepared stement bind variables. 
		$sql = 'SELECT mdate, desc, etype, mc_version from Status ' ;

		$deviceType = getDeviceType();
		
		if ($deviceType == STINGRAY_DEVICE_TYPE) {
			$pdo = openMioDB();
			$rows = $this->executeQueryWithDb($pdo, $sql);
			$status = $rows[0];
		} else {
			$status = $this->executeQueryAndFetchOneRow($sql, 'GET_STATUS', NULL, true);
		}
		
		return $status;
	}	
}
?>