<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');

class VolumesDB extends DBAccess {

	static $queries = array (
		'SELECT_VOLUME'  => "SELECT uuid,label,base_path,dev_path,drive_path,fs_type,is_connected FROM Volumes WHERE uuid = :uuid",
		'SELECT_VOLUMES' => "SELECT uuid,label,base_path,dev_path,drive_path,fs_type,is_connected FROM Volumes",
		'INSERT_VOLUME'  => "INSERT INTO Volumes (uuid,label,base_path,dev_path,drive_path,fs_type,is_connected,mount_time,modified_date) VALUES (:uuid,:label,:base_path,:dev_path,:drive_path,:fs_type,:is_connected,:mount_time,:modified_date)",
		'DELETE_LABEL'   => "DELETE FROM Volumes WHERE label = :label",
		'DELETE_VOLUME'  => "DELETE FROM Volumes WHERE uuid = :uuid",
		'DELETE_ALL'     => "DELETE FROM Volumes",
	);

	/**
	 * VolumesDB
	 *
	 * Provides access to the Volumes database table and extends DBAccess
	 *
	 */
	function __construct()
	{
	}


	/**
	 * Select and return information of all or a specific volume
	 *
	 * @param integer $uuid of a specified volume
	 * @return array $volumes information of the volumes
	 */
	public function getVolume($uuid=null)
	{
		if (empty($uuid)) {
			$volumes = $this->executeQuery(self::$queries['SELECT_VOLUMES'],'SELECT_VOLUMES');
			return $volumes;
		} else {
			$bindVarNVTArray = array(array(':uuid', $uuid, PDO::PARAM_STR));
			$volumes = $this->executeQuery(self::$queries['SELECT_VOLUME'], 'SELECT_VOLUME', $bindVarNVTArray);
			if (isset($volumes) && sizeof($volumes) > 0) {
				$volumes = $volumes[0];
			} else {
				$volumes = null;
			}
		}
		if (!$volumes) {
			return false;
		}
		return $volumes;
	}


	/**
	 * Creates a new volume in the Volumes table
	 *
	 * @param string  $uuid holds the unique unit id
	 * @param string  $label holds the volume label
	 * @param string  $basePath holds the volume moint point
	 * @param string  $devPath holds the device path
	 * @param string  $drivePath holds the drive path
	 * @param string  $fsType holds the file system type
	 * @param integer $isConnected indicates if volume is connected
	 * @return boolean $status indicates success of operation
	 */
	function createVolume($uuid, $label, $basePath, $devPath, $drivePath, $fsType, $isConnected)
	{
		$isConnected = ($isConnected === 'true' || $isConnected === '1' || $isConnected === 1) ? 'true' : 'false';
		$date         = date("Y-m-d H:i:s");
		$mounteTime   = $date;
		$modifiedDate = $date;
		$bindVarNVTArray = array(
			array(':uuid', getSafeDatabaseText((string)$uuid), PDO::PARAM_STR),
			array(':label', getSafeDatabaseText((string)$label), PDO::PARAM_STR),
			array(':base_path', getSafeDatabaseText((string)$basePath), PDO::PARAM_STR),
			array(':dev_path', getSafeDatabaseText((string)$devPath), PDO::PARAM_STR),
			array(':drive_path', getSafeDatabaseText((string)$drivePath), PDO::PARAM_STR),
			array(':fs_type', getSafeDatabaseText((string)$fsType), PDO::PARAM_STR),
			array(':is_connected', $isConnected, PDO::PARAM_INT),
			array(':mount_time', $mountTime, PDO::PARAM_STR),
			array(':modified_date', $modifiedDate, PDO::PARAM_STR),
		);
		$status = $this->executeInsert(self::$queries['INSERT_VOLUME'], 'INSERT_VOLUME', $bindVarNVTArray);
		if ($status < 1) $status = false;
		return $status;
	}


	/**
	 * Update a volume with specified uuid
	 *
	 * @param string  $uuid holds the unique unit id
	 * @param string  $label holds the volume label
	 * @param string  $basePath holds the volume moint point
	 * @param string  $devPath holds the device path
	 * @param string  $drivePath holds the drive path
	 * @param string  $fsType holds the file system type
	 * @param integer $isConnected indicates if volume is connected
	 * @return boolean $status indicates success of operation
	 */
	public function updateVolume($uuid, $label=null, $basePath=null, $devPath=null, $drivePath=null, $fsType=null, $isConnected=null)
	{
		$params = array();
		if ($label !== null) {
			$params['label'] = getSafeDatabaseText((string)$label);
		}
		if ($basePath !== null) {
			$params['base_path'] = getSafeDatabaseText((string)$basePath);
		}
		if ($devPath !== null) {
			$params['dev_path'] = getSafeDatabaseText((string)$devPath);
		}
		if ($drivePath !== null) {
			$params['drive_path'] = getSafeDatabaseText((string)$drivePath);
		}
		if ($fsType !== null) {
			$params['fs_type'] = getSafeDatabaseText((string)$fsType);
		}
		if ($isConnected !== null) {
			$params['is_connected'] = ($isConnected === 'true' || $isConnected === '1' || $isConnected === 1) ? 'true' : 'false';
		}
		$date = date("Y-m-d H:i:s");
		$params['mount_time'] = $date ;
		$params['modified_date'] = $date;
		if (count($params > 0 )) {
			$sql    = $this->generateUpdateSql('Volumes', 'uuid', $uuid, $params);
			$status = $this->executeUpdate($sql);
			return $status;
		} else {
			return false;
		}
	}


	/**
	 * Deletes a volume with specified uuid
	 *
	 * @param integer $uuid
	 * @return boolean $status indicates whether the update succeeded
	 */
	public function deleteVolume($uuid)
	{
		if (!empty($uuid)) {
			$bindVarNVTArray = array(array(':uuid', $uuid, PDO::PARAM_STR));
			$status = $this->executeDelete(self::$queries['DELETE_VOLUME'], 'DELETE_VOLUME', $bindVarNVTArray);
		} else {
			$status = $this->executeDelete(self::$queries['DELETE_ALL'], 'DELETE_ALL');
		}
		return $status;
	}
}
?>