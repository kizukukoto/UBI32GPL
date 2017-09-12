<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.

require_once('dbaccess.inc');
require_once('shareaccess.php');

/**
 * Class to enable CRUD actions on the UserShares table.
 *
 * The UserShares table contains a row for each Samba share that each user has access to.
 *
 * @author sapsford_j
 *
 */

class UserSharesDB extends DBAccess {

	private static $queries = array (
		'GET_SHARES'		=> "SELECT * FROM UserShares",
		'GET_ACTIVE_SHARES' => "SELECT share_name AS share FROM UserShares ",
		'GET_ALL_SHARE_NAMES' => "SELECT share_name, usb_handle FROM UserShares ",
		'GET_NAMED_SHARE'   => "SELECT * FROM UserShares WHERE share_name = :share_name",
		'INSERT_USER_SHARE' => "INSERT INTO UserShares ( share_name, description, public_access, media_serving, remote_access, created_date)
								 VALUES ( :share_name, :description, :public_access, :media_serving, :remote_access, DATETIME('NOW') )",
		'INSERT_USER_SHARE_DYNAMIC' => "INSERT INTO UserShares ( share_name, description, public_access, media_serving, remote_access, dynamic_volume, capacity, read_only, usb_handle, file_system_type, created_date)
								 VALUES ( :share_name, :description, :public_access, :media_serving, :remote_access, :dynamic_volume, :capacity, :read_only, :usb_handle, :file_system_type, DATETIME('NOW') )",
		'DELETE_USER_SHARE' => "DELETE FROM UserShares WHERE share_name=:share_name ",
		'DELETE_ALL_SHARES' => "DELETE FROM UserShares",
		'DELETE_DYNAMIC_VOLUME_SHARES' => "DELETE FROM UserShares WHERE dynamic_volume='true'"
	);

	public function __construct() {
		require_once("dbaccess.inc");
	}

	/**
	 * getShares
	 *
	 * Returns an assoc. array of properties for all shares with keys:
	 * user_id, share_name, share_path, description,  public_access, media_serving, created_date.
	 *
	 * @param $user_id - id of user
	 */
	public function getShares() {
		$shareList = $this->executeQueryWithDb(getDb(), self::$queries['GET_SHARES']);
		return $shareList;
	}

	/**
	 *
	 * Returns share name whth remote_access set to TRUE
	 */
	public function getActiveShares(){
		$shareList = $this->executeQueryWithDb(getDb(), self::$queries['GET_ACTIVE_SHARES']);
		return $shareList;
	}

	/**
	 *
	 * Returns share name whth remote_access set to TRUE
	 */
	public function getAllShareNames(){
		$shareList = $this->executeQueryWithDb(getDb(), self::$queries['GET_ALL_SHARE_NAMES']);
		return $shareList;
	}
	
	
	/**
	 * getShareForName
	 *
	 * Returns an assoc. array of properties for a named share belonging to a single user with keys:
	 * user_id, share_name, share_path, description,  public_access, media_serving, created_date.
	 *
	 * @param $user_id - id of user
	 */
	public function getShareForName($shareName) {
		$bindVarNVTArray = array(array(':share_name', $shareName, PDO::PARAM_STR));
		return $this->executeQuery(self::$queries['GET_NAMED_SHARE'], 'GET_NAMED_SHARE', $bindVarNVTArray);
	}

	/**
	 * createUserShare
	 *
	 * Inserts a new row into the UserShares table for a Samba share that a user has access to
	 *
	 * @param $share_name name of share
	 * @param $description descripion of share
	 * @param $public_access whether public access is allowed (true/false)
	 * @param $media_serving type of media served by the share
	 * @param $remote_access whether remote access is allowed (true/false)
	 * @param $dynamic_volume indicates if share is from removable storage (true/false)
     * @param $capacity (only meaningful for dynamic volumes) maximum size of share in MB
	 * @param $read_only (only meaningful for dynamic volumes) indicates if share can only be read (true/false)
	 * @param $usb_handle (only meaningful for dynamic volumes) handle of USB device associated with volume
	 */
	public function createUserShare($share_name, $description,
									$public_access, $media_serving, $remote_access,
									$dynamic_volume, $capacity, $read_only, $usb_handle, $file_system_type){

		$nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false)
        {
			return false;
        }
        else 
        {
        	//if (isset($nasConfig['drive-lib']['AUTO_MOUNT_CONFIG_FILE'])) {
					$bindVarNVTArray = array(
						array(':share_name', getSafeDatabaseText((string)$share_name), PDO::PARAM_STR),
						array(':description', getSafeDatabaseText((string)$description), PDO::PARAM_STR),
						array(':public_access', getSafeDatabaseText((string)$public_access), PDO::PARAM_STR),
						array(':media_serving', getSafeDatabaseText((string)$media_serving), PDO::PARAM_STR),
						array(':remote_access', getSafeDatabaseText((string)"true"), PDO::PARAM_STR),
						array(':dynamic_volume', getSafeDatabaseText((string)$dynamic_volume), PDO::PARAM_STR),
						array(':capacity', getSafeDatabaseText((string)$capacity), PDO::PARAM_STR),
						array(':read_only', getSafeDatabaseText((string)$read_only), PDO::PARAM_STR),
						array(':usb_handle', getSafeDatabaseText((string)$usb_handle), PDO::PARAM_STR),
						array(':file_system_type', getSafeDatabaseText((string)$file_system_type), PDO::PARAM_STR)
					);
					return $this->executeInsert(self::$queries['INSERT_USER_SHARE_DYNAMIC'], 'INSERT_USER_SHARE', $bindVarNVTArray);
			//	} else {
			//		$bindVarNVTArray = array(
			//			array(':share_name', getSafeDatabaseText((string)$share_name), PDO::PARAM_STR),
			//			array(':description', getSafeDatabaseText((string)$description), PDO::PARAM_STR),
			//			array(':public_access', getSafeDatabaseText((string)$public_access), PDO::PARAM_STR),
			//			array(':media_serving', getSafeDatabaseText((string)$media_serving), PDO::PARAM_STR),
			//			array(':remote_access', getSafeDatabaseText((string)"true"), PDO::PARAM_STR)
			//		);
			//		return $this->executeInsert(self::$queries['INSERT_USER_SHARE'], 'INSERT_USER_SHARE', $bindVarNVTArray);
			//	}
        	}
		}

	/**
	 * updateUserShare
	 *
	 * Updates a single user share row in the Db with one or more of the optional parameters. The parameter
	 * $user_id and $exisitng_share_name identify the user share that is to be modified and must be supplied. If mone of the
	 * optional parameters are supplied, then no update will occur and NULL will be returned.
	 *
	 * @param $user_id id of user
	 * @param $existing_share_name name of user share that is to be modified
	 * @param $new_share_name new name for user share
	 * @param $description description of share
	 * @param $public_access public_access boolean flag
	 * @param $media_serving media_serving boolean flag
	 */

	public function updateUserShare($existing_share_name,
									$new_share_name=NULL, $description=NULL,
									$public_access=NULL, $media_serving=NULL, $remote_access=NULL) {
		if (empty($existing_share_name)) {
			return false;
		}
		$shareNameChanged = !empty($new_share_name) && strcmp($existing_share_name, $new_share_name) != 0;
		$queryParams = array();
		if ($shareNameChanged) {
			$sa = new ShareAccess();
			//delete old share name acl
			$acl = $sa->getAclForShare($existing_share_name,NULL);
			if (!empty($acl)) {
				$sa->deleteAllAccessForShare($existing_share_name);
			}
			$queryParams["share_name"] = getSafeDatabaseText((string)$new_share_name);
		}
		if ($description !== null) {
			$queryParams["description"] = getSafeDatabaseText((string)$description);
		}
		if (!empty($public_access)) {
			$queryParams["public_access"] = getSafeDatabaseText((string)$public_access);
		}
		if (!empty($media_serving)) {
			$queryParams["media_serving"] = getSafeDatabaseText((string)$media_serving);
		}
		if (!empty($remote_access)) {
			$queryParams["remote_access"] = getSafeDatabaseText((string)"true"); //should always be true?
		}
		if (sizeof($queryParams > 0 )) {
			$sql = $this->generateUpdateSql("UserShares", "share_name", $existing_share_name, $queryParams);
			$status = $this->executeUpdate($sql);
		}
		if ($status && $shareNameChanged) {
			//update share access - get acl for old share name, delete it and add for new share name
			if (!empty($acl)) {
				//re-create acl for new share name
				foreach($acl as $access) {
					$sa->addAclForShare($new_share_name, $access['user_id'], $access['access_level']);
				}
			}
		}
		return true;
	}

	/**
	 * deleteUserShare
	 *
	 * Deletes a single named Samba share
	 *
	 * @param $share_name name of Samba share
	 */

	public function deleteUserShare($share_name) {
		$bindVarNVTArray = array( array(':share_name', getSafeDatabaseText((string)$share_name), PDO::PARAM_STR));
		return $this->executeDelete(self::$queries['DELETE_USER_SHARE'], 'DELETE_USER_SHARE', $bindVarNVTArray);
	}

	/**
	 * deleteAllShares()
	 *
	 * Deletes all Samba shares for ALL users - use with caution
	 *
	 */

	public function deleteAllShares() {
		return $this->executeSql(self::$queries['DELETE_ALL_SHARES']);
	}

	/**
	 * deleteDynamicVolumeShares()
	 *
	 * Deletes all the shares associated with dynamic volumes - use with caution
	 * This is used at startup to cleanup the database after the system was powered
	 * off while a USB drive was connected.
	 *
	 */

	public function deleteDynamicVolumeShares() {
		return $this->executeSql(self::$queries['DELETE_DYNAMIC_VOLUME_SHARES']);
	}
}

/** TEST CODE **/
/*
require_once("usersdb.inc");
$usersdb = new UsersDB();
$usersdb->createUser("Burt", "pw", "Burt Burtenson", 0, 0);
$usersdb->createUser("Ralph", "pw", "Ralph Ralphenson", 0, 0);
$usersdb->createUser("Huey", "pw", "Huey Lewis", 0, 0);
$userIdBurt = $usersdb->getUserId("Burt");
$userIdRalph = $usersdb->getUserId("Ralph");
$userIdHuey = $usersdb->getUserId("Huey");
$usersharesdb = new UserSharesDB();
$usersharesdb->deleteAllShares();  //WARNING - DO NOT RUN THESE TESTS AGAINST A LIVE DB AS ALL SHARES WILL BE DELETED
$usersharesdb->createUserShare(1, "burt", "Burt's Share",
									"disabled", "all", "enabled");
$usersharesdb->createUserShare(1, "burt2", "Burt's other Share",
									"disabled", "all", "enabled");
$usersharesdb->createUserShare(2, "ralph", "Ralph's Share",
									"disabled", "all", "enabled");
$usersharesdb->createUserShare(3, "huey", "Huey's Share",
									"disabled", "all", "enabled");
$listShares = $usersharesdb->getShares();
var_dump($listShares);
$burtsShares = $usersharesdb->getSharesForUserId(1);
var_dump($burtsShares);
$hueysShare = $usersharesdb->getShareForUserId(3,"huey");
var_dump($hueysShare);
$usersharesdb->updateUserShare(1, "burt", "burt_updated", "An Updated Share",
									"enabled", "photos", "disabled");
$usersharesdb->deleteUserShare(2,"ralph");
$usersharesdb->deleteAllSharesForUser(1);
$usersharesdb->deleteAllShares();
*/
?>