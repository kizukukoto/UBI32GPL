<?php
/**
 * Class Volumes
 *
 * PHP version 5.2
 *
 * @category  Classes
 * @package   Orion
 * @author    WDMV Software Engineering
 * @copyright 2011 © Western Digital Technologies, Inc. - All rights reserved
 * @license   http://support.wdc.com/download/netcenter/gpl.txt GNU License
 * @link      http://www.wdc.com/
 *
 */
require_once('volumesdb.inc');
require_once('usersharesdb.php');

 /**
 * Volume Class manipulates volumes within drives.
 *
 * @version Release: @package_version@
 *
 */
class Volume {

	function __construct()
	{
	}


	/**
	 * Connect a new volume and add entry to Volumes table
	 *
	 * @param string $uuid unqiue identifier of volume
	 * @param string $label name of volume
	 * @param string $path mount point of volume
	 * @param string $drive_path unqiue identifier of drive
	 * @return boolean $status return status of operation
	 */
	public function connect($uuid, $label, $path, $drive_path)
	{
		//Create new volume entry, if one does not already exist
		$VolumesDB = new VolumesDB();
		$is_connected = 1;
		$status = $VolumesDB->insert($uuid, $label, $path, $drive_path, $is_connected);
		if (!status) return $status;

		//Create new share with same name, if one does not already exist
		$UserSharesDB = new UserSharesDB();
		$share_name    = $label;
		$description   = 'Volume Label';
		$public_access = 'true';
		$media_serving = 'any';
		$remote_access = 'true';
		$status = $UserSharesDB->createUserShare($share_name, $description, $public_access, $media_serving, $remote_access);
		if ($status == -1) return false;
		return $status;
	}


	/**
	 * Retrieve all volumes or a specified volume
	 *
	 * @param string $uuid of a specified volume
	 * @return array $volumes return array of volumes
	 */
	public function retrieve($uuid=null)
	{
		$volumesDB = new VolumesDB();

		if (!empty($uuid)) {
			$volume = $volumesDB->select($uuid);
			if (empty($volume)) {
				return false;
			}
			return $volume;
		}

		$volumes = $volumesDB->select();
		if (empty($volumes)) {
			return false;
		}
		return $volumes;
	}


	/**
	 * Disconnect volume and mark entry as disconnected
	 *
	 * @param string $uuid of a specified volume
	 * @return boolean $status return status of operation
	 */
	public function disconnect($uuid=null)
	{
		$VolumesDB = new VolumesDB();
		$is_connected = false;
		$status = $VolumesDB->update($uuid, null, null, null, $is_connected);
		return $status;
	}


	/**
	 * Modify volume label
	 *
	 * @param string $uuid of a specified volume
	 * @param string  $label holds the new volume label name
	 * @return boolean $status return status of operation
	 */
	public function modify($uuid, $label, $path=null, $drive_path=null, $is_connected=null)
	{
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'is_connected', $is_connected);
		$VolumesDB = new VolumesDB();
		$status = $VolumesDB->update($uuid, $label, $path, $drive_path, $is_connected);
		return $status;
	}


	/**
	 * Unmount a volume by removing entry from Volumes table
	 * @param string $uuid of a specified volume
	 * @return boolean $status return status of operation
	 */
	public function unmount($uuid)
	{
		$VolumesDB = new VolumesDB();
		$status = $VolumesDB->delete($uuid);
		return $status;
	}


	/**
	 * Clean all shares from volume
	 *
	 * @param string $uuid
	 * @return boolean $status return status of operation
	 */
	public function clean($uuid)
	{
		//Remove associated shares
		$UserSharesDB = new UserSharesDB();
		$shareName = $label;
		$status = $UserSharesDB->deleteUserShare($shareName);
		return $status;
	}


	/**
	 * Delete all data from volume and delete volume entry
	 *
	 * @param string $uuid
	 * @return boolean $status return status of operation
	 */
	public function delete($uuid)
	{
		//Remove associated shares
		$status = $this->clean($uuid);
		if (!status) return $status;
		//Remove volume entry
		$status = $this->umount($uuid);
		return $status;
	}
}
?>