<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');
require_once('userlist.php');
require_once('security.inc');

class DeviceUsersDB extends DBAccess {

	static $queries = array (
		'VALID_DEVICE_USER'  => "SELECT COUNT(*) AS COUNT FROM DeviceUsers  WHERE device_user_id = :device_user_id and auth = :auth",
		'COUNT_DEVICE_USERS' => "SELECT COUNT(*) AS COUNT FROM DeviceUsers",
		'WAN_ACCESS_ENABLED' => "SELECT enable_wan_access FROM DeviceUsers WHERE device_user_id = :device_user_id",
	    'GET_WEB_USER_BY_EMAIL' => "SELECT * FROM DeviceUsers WHERE email = :email and type like 'webuser'",
		'GET_ALL_DEVICE_USERS' => "SELECT * FROM DeviceUsers",
		'GET_DEVICE_USERS_FOR_USER_ID' => "SELECT * FROM DeviceUsers WHERE user_id = :user_id",
		'GET_DEVICE_USER' => "SELECT * FROM DeviceUsers WHERE device_user_id = :device_user_id",
		'INSERT_DEVICE_USER' => "INSERT INTO DeviceUsers (user_id, device_user_id, auth, email, type, name, is_active, created_date, enable_wan_access, dac, dac_expiration)
								 VALUES (:user_id, :device_user_id, :auth, :email, :type, :name, :is_active, :now, :enable_wan_access, :dac, :dac_expiration)",
		'DELETE_DEVICE_USER'      => "DELETE FROM DeviceUsers WHERE device_user_id = :device_user_id",
		'DELETE_DEVICE_USERS'     => "DELETE FROM DeviceUsers WHERE user_id = :user_id",
		'DELETE_ALL_DEVICE_USERS' => "DELETE FROM DeviceUsers"
	);

	function DeviceUsersDB(){
		require_once('dbaccess.inc');
	}

	/**
	 *
	 * @param $device_user_id
	 * @param $auth
	 */
	function isValid($device_user_id, $auth){

		$bindVarNVTArray = array(array(':device_user_id', getSafeDatabaseText($device_user_id), PDO::PARAM_INT),
								 array(':auth', getSafeDatabaseText($auth), PDO::PARAM_STR));
		$rows = $this->executeQuery(self::$queries['VALID_DEVICE_USER'], 'VALID_DEVICE_USER', $bindVarNVTArray);
		$retVal = false;
		foreach ($rows as $row) {
			$count = $row['COUNT'];
			if($count == 1){
				$retVal = true;
			}
			break;
		}
		return $retVal;
	}

	/**
	 * 
	 * @return Ambigous <number, unknown>
	 */
	function getNumberOfDeviceUsers(){
		$count = 0;
		$rows = $this->executeQuery(self::$queries['COUNT_DEVICE_USERS'], 'COUNT_DEVICE_USERS');
		foreach ($rows as $row) {
			$count = $row['COUNT'];
			break;
		}
		return $count;
	}

	/**
	 *
	 * @param $device_user_id
	 * @param $auth
	 */
	function isWanAccessEnabled($device_user_id){
		$bindVarNVTArray = array( array(':device_user_id', $device_user_id, PDO::PARAM_INT));
		$rows = $this->executeQuery(self::$queries['WAN_ACCESS_ENABLED'], 'WAN_ACCESS_ENABLED', $bindVarNVTArray);
		$retVal = false;
		foreach ($rows as $row) {
			$retVal = $row['enable_wan_access'];
			break;
		}
		return $retVal;
	}

	/**
	 *
	 * @param $user_id
	 */
	function getDeviceUsersForUserId($user_id){
		if (isAdmin($user_id)) {
			$bindVarNVTArray = null;
			return $this->executeQuery(self::$queries['GET_ALL_DEVICE_USERS'], 'GET_ALL_DEVICE_USERS', $bindVarNVTArray);
		} else {
			$bindVarNVTArray = array( array(':user_id', $user_id, PDO::PARAM_INT));
			return $this->executeQuery(self::$queries['GET_DEVICE_USERS_FOR_USER_ID'], 'GET_DEVICE_USERS_FOR_USER_ID', $bindVarNVTArray);
		}
	}

	/**
	 *
	 * @param $device_user_id
	 */
	function getDeviceUser($device_user_id){
		$bindVarNVTArray = array( array(':device_user_id', $device_user_id, PDO::PARAM_INT));
		return $this->executeQueryAndFetchOneRow(self::$queries['GET_DEVICE_USER'], 'GET_DEVICE_USER', $bindVarNVTArray);
	}
	
	

	/**
	 *
	 * @param $device_user_id
	 */
	function getWebUserByMail($email){
		$bindVarNVTArray = array( array(':email', $email, PDO::PARAM_STR));
		return $this->executeQueryAndFetchOneRow(self::$queries['GET_WEB_USER_BY_EMAIL'], 'GET_WEB_USER_BY_EMAIL', $bindVarNVTArray);
	}

	/**
	 *
	 * @param $user_id
	 * @param $device_user_id
	 * @param $auth
	 * @param $email
	 */
	function createDeviceUser($user_id, $device_user_id, $auth, $email, $name, $type,
								$active, $enableWanAccess, $dac=null, $dacExpiration=null){
		$cutTimeInSec = time();
		$bindVarNVTArray = array(
			array(':user_id', (int)$user_id, PDO::PARAM_INT),
			array(':device_user_id', (int)$device_user_id, PDO::PARAM_INT),
			array(':auth', getSafeDatabaseText((string)$auth), PDO::PARAM_STR),
			array(':email', getSafeDatabaseText((string)$email), PDO::PARAM_STR),
			array(':name', getSafeDatabaseText($name), PDO::PARAM_STR),
			array(':type', getSafeDatabaseText($type), PDO::PARAM_STR),
			array(':is_active', $active, PDO::PARAM_BOOL),
			array(':enable_wan_access', $enableWanAccess, PDO::PARAM_BOOL),
			array(':now', $cutTimeInSec, PDO::PARAM_INT),
		);
		if(isset($dac)){
			array_push($bindVarNVTArray, array(':dac', $dac, PDO::PARAM_STR));
		}else{
			array_push($bindVarNVTArray, array(':dac', null, PDO::PARAM_NULL));
		}
		if(isset($dacExpiration)){
			array_push($bindVarNVTArray, array(':dac_expiration', $dacExpiration, PDO::PARAM_INT));
		}else{
			array_push($bindVarNVTArray, array(':dac_expiration', null, PDO::PARAM_NULL));
		}
		//
		// Create corresponding apache user - call 'userlist' directly for now
		//

		updateApacheHtpassword('CREATE', $device_user_id, $auth);
		return $this->executeInsert(self::$queries['INSERT_DEVICE_USER'], 'INSERT_DEVICE_USER', $bindVarNVTArray);
	}

	// Update device user
	function updateDeviceUser($deviceUserId, $type, $name, $email, $isActive, $typeName, $application) {
		$email = getSafeDatabaseText($email);
		$type = getSafeDatabaseText($type);
		$name = getSafeDatabaseText($name);
		$active = getSafeDatabaseText($isActive);
		$typeName = getSafeDatabaseText($typeName);
		$application = getSafeDatabaseText($application);
		$sql = $this->generateUpdateSql('DeviceUsers', 'device_user_id', (int)$deviceUserId,
			array('type'=>$type, 'name'=>$name, 'email'=>$email, 'is_active'=>$isActive, 'type_name'=>$typeName, 'application'=>$application) );
		return $this->executeUpdate($sql);
	}

	function deleteDeviceUser($device_user_id) {
		$bindVarNVTArray = array(array(':device_user_id', $device_user_id, PDO::PARAM_INT));
		//
		// Delete corresponding apache user
		//
		$ulObj = new Userlist();
		$ulObj->deleteApacheUser('delete_dev_user', $device_user_id);
		return $this->executeDelete(self::$queries['DELETE_DEVICE_USER'], 'DELETE_DEVICE_USER', $bindVarNVTArray);
	}

	function deleteDeviceUsers($userId) {
		$bindVarNVTArray = array(array(':user_id', $userId, PDO::PARAM_INT));
		return $this->executeDelete(self::$queries['DELETE_DEVICE_USERS'], 'DELETE_DEVICE_USERS', $bindVarNVTArray);
	}

	function deleteAllDeviceUser() {
		return $this->executeUpdate(self::$queries['DELETE_ALL_DEVICE_USERS']);
	}
}
?>