<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbaccess.inc');

class UsersDB extends DBAccess {

	static $queries = array (
		'SELECT_USER'     => "SELECT user_id,local_username as username,fullname,is_admin,is_password FROM Users WHERE user_id=:user_id",
		'SELECT_USERS'    => "SELECT user_id,local_username as username,fullname,is_admin,is_password FROM Users",
		'SELECT_USER_ID'  => "SELECT user_id FROM Users WHERE local_username = :local_username",
		'SELECT_USERNAME' => "SELECT local_username FROM Users WHERE user_id = :user_id",
		'SELECT_USER_BY_EMAIL'        => "SELECT u.user_id, u.local_username, u.fullname, u.is_admin, u.is_password FROM DeviceUsers du, Users u WHERE du.user_id = u.user_id AND du.email = :email",
		'SELECT_USERS_WITH_EMAIL'     => "SELECT distinct u.user_id, u.local_username, u.fullname, u.is_admin, u.is_password, du.email FROM AlbumAccess aa LEFT JOIN DeviceUsers du ON du.user_id = aa.user_id LEFT JOIN Users u ON u.user_id  = du.user_id LEFT JOIN Albums a ON a.album_id = aa.abum_id WHERE a.owner = :user_id AND du.email LIKE :email",
		'SELECT_ALL_USERS_WITH_EMAIL' => "SELECT u.user_id, u.local_username, u.fullname, u.is_admin, u.is_password, du.email FROM DeviceUsers du, Users u WHERE du.user_id = u.user_id AND du.email LIKE :email",
		'SELECT_IS_ADMIN' => "SELECT COUNT(*) AS COUNT FROM Users WHERE userid = :user_id and is_admin=1",
		'SELECT_IS_VALID' => "SELECT COUNT(*) AS COUNT FROM Users WHERE local_username = :username",
		'INSERT_USER'     => "INSERT INTO Users (local_username,fullname,is_admin,is_password,created_date) VALUES (:local_username,:fullname,:is_admin,:is_password,:created_date)",
		'DELETE_USER'     => "DELETE FROM Users WHERE user_id = :user_id",
		'DELETE_USERNAME' => "DELETE FROM Users WHERE local_username = :username",
	);

	/**
	 * UsersDB
	 *
	 * Provides access to the User database table and extends DBAccess
	 *
	 */
	function __construct() {
	}


	/**
	 * getUser
	 *
	 * Returns information of specified user
	 *
	 * @param integer $userId of the specified user
	 * @return array $users information of the users
	 */
	public function getUser($userId=null) {
		if (empty($userId)) {
			$users = $this->executeQuery(self::$queries['SELECT_USERS'],'SELECT_USER');
			return $users;
		} else {
			$bindVarNVTArray = array(array(':user_id', $userId, PDO::PARAM_INT));
			$users = $this->executeQuery(self::$queries['SELECT_USER'], 'SELECT_USER', $bindVarNVTArray);
			if (isset($users) && sizeof($users) > 0) {
				$users = $users[0];
			} else {
				$users = null;
			}
		}
		if (!$users) {
			return false;
		}
		return $users;
	}

	/**
	 * getUserByEmail - Returns user with exact match email
	 *
	 * @param $email
	 */
	function getUserByEmail($email, $userId=null){
		$bindVarNVTArray = array(
			array(':email', $email, PDO::PARAM_STR),
		);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'SELECT_USERS_BY_EMAIL', self::$queries['SELECT_USERS_BY_EMAIL']);
		return $this->executeQuery(self::$queries['SELECT_USER_BY_EMAIL'], 'SELECT_USER_BY_EMAIL', $bindVarNVTArray);
	}

	/**
	 * getUsersWithEmail - Returns users with specified partial email
	 *
	 * @param $email
	 */
	function getUsersWithEmail($email, $userId=null){
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'userId', $userId);
		if (!empty($userId)) {
			$bindVarNVTArray = array(
				array(':email', $email, PDO::PARAM_STR),
				array(':user_id', $userId, PDO::PARAM_INT),
			);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'SELECT_USER_BY_EMAIL', self::$queries['SELECT_USER_BY_EMAIL']);
			return $this->executeQuery(self::$queries['SELECT_USER_WITH_EMAIL'], 'SELECT_USER_WITH_EMAIL', $bindVarNVTArray);
		} else {
			$bindVarNVTArray = array(
				array(':email', $email, PDO::PARAM_STR),
			);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'SELECT_USERS_BY_EMAIL', self::$queries['SELECT_USERS_BY_EMAIL']);
			return $this->executeQuery(self::$queries['SELECT_ALL_USERS_WITH_EMAIL'], 'SELECT_ALL_USERS_WITH_EMAIL', $bindVarNVTArray);
		}
	}

	/**
	 * Creates a new user in the Users table
	 * @param string $username
	 * @param string $password
	 * @param string $fullname
	 * @param string $isAdmin
	 */
	function createUser($username, $password=null, $fullname=null, $isAdmin=null) {
		$isPassword = !empty($password) ? 'true' : 'false';
		$createdDate = time();
		$bindVarNVTArray = array(
			array(':local_username', getSafeDatabaseText((string)$username), PDO::PARAM_STR),
			array(':fullname', getSafeDatabaseText((string)$fullname), PDO::PARAM_STR),
			array(':is_admin', $isAdmin, PDO::PARAM_INT),
			array(':is_password', $isPassword, PDO::PARAM_INT),
			array(':created_date', $createdDate, PDO::PARAM_INT),
		);
		$userId = $this->executeInsert(self::$queries['INSERT_USER'], 'INSERT_USER', $bindVarNVTArray);
		if ($userId < 1) $userId = false;
		return $userId;
	}

	/**
	 * updateUser
	 *
	 * Updates a single user row in the User table
	 *
	 * @param integer $userId identifies which user is to be updated
	 * @param string $username holds the new user name
	 * @param string $fullname holds the new full name of the user
	 * @param string $oldPassword holds the old password of the user
	 * @param string $newPassword holds the new password of the user
	 * @param boolean $isAdmin the new boolean status of whether this user is an admin
	 * @return boolean $status indicates whether the update succeeded
	 */
	public function updateUser($userId, $username=null, $fullname=null, $oldPassword=null, $newPassword=null, $isAdmin=null, $changePassword=false) {
		$params = array();
		if ($username !== null) {
			$params['local_username'] = getSafeDatabaseText((string)$username);
		}
		if ($fullname !== null) {
			$params['fullname'] = getSafeDatabaseText((string)$fullname);
		}
		$params['is_admin']    = ($isAdmin == 'true' || $isAdmin == 'yes' || $isAdmin == '1') ? 'true' : 'false';
		$params['is_password'] = $changePassword ? 'true' : 'false';
		//printf("<PRE>%s.%s=[%s]</PRE>", __METHOD__, 'is_admin', $params['is_admin']);
	 	if (count($params > 0 )) {
			$sql = $this->generateUpdateSql('Users', 'user_id', $userId, $params);
			//printf("<PRE>%s.%s=[%s]</PRE>", __METHOD__, 'sql', $sql);
			$status = $this->executeUpdate($sql);
			return $status;
		}
	}

	/**
	 * deleteUser
	 *
	 * Deletes a user that is identified based on the userId
	 *
	 * @param integer $user_id
	 * @return boolean indicates whether the update succeeded
	 */
	public function deleteUser($user_id) {
		$username = $this->getUsername($user_id);
		/* DELETE USER IN OS LAYER
		$os_status = DeleteUser($uuser_idserId, $username);
		if (!$os_status) {
			return false;
		}
		*/
		$bindVarNVTArray = array( array(':user_id', $user_id, PDO::PARAM_INT));
		$status = $this->executeDelete(self::$queries['DELETE_USER'], 'DELETE_USER', $bindVarNVTArray);
		return $status;
	}

	/**
	 * Returns the userId of the user identified by the supplied Local Username
	 * @param String $localUsername identifier of the user (column in the Users table)
	 * @return integer userId of the specified User
	 */
	function getUserId($username) {
		$sql = "SELECT user_id
				FROM Users
				WHERE lower(local_username) = :local_username";
		$bindVarNVTArray = array( array(':local_username', strtolower($username), PDO::PARAM_STR));
		$rows = $this->executeQuery($sql, 'GET_USER_ID', $bindVarNVTArray);
		if (isset($rows[0]['user_id'])) {
			return $rows[0]['user_id'];
		} else {
			return null;
		}
	}

	/**
	 * Returns the local username of the user identified by the supplied userId
	 * @param integer $userId identifier of the user (primary key of Users table)
	 * @return String local username of the specified User
	 */
	public function getUsername($userId) {
		$sql = "SELECT local_username
				FROM Users
				WHERE user_id = :user_id";
		$bindVarNVTArray = array( array(':user_id', $userId, PDO::PARAM_INT));
		$rows = $this->executeQuery($sql, 'GET_USERNAME', $bindVarNVTArray);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'rows', print_r($rows,true));
		if (isset($rows[0]['local_username'])) {
			return $rows[0]['local_username'];
		} else {
			return null;
		}
	}

	/**
	 * Returns the local username of the user identified by the supplied userId
	 * @param integer $userId identifier of the user (primary key of Users table)
	 * @return String local username of the specified User
	 */
	function getLocalUsername($userId) {
		$sql = "SELECT local_username
				FROM Users
				WHERE user_id = :user_id";
		$bindVarNVTArray = array( array(':user_id', $userId, PDO::PARAM_INT));
		$rows = $this->executeQuery($sql, 'GET_LOCAL_USERNAME', $bindVarNVTArray);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'rows', print_r($rows,true));
		$username = null;
		foreach ($rows as $row) {
			$username = $row['local_username'];
			break;
		}
		return $username;
	}

	/**
	 * Updates the isAdmin status of the user identified by localUserName
	 * @param String $username identifies which user is to be updated
	 * @param boolean $is_admin the new boolean status of whether this user is an admin
	 */
	function updateLocalUser($username, $isAdmin) {
		$sql = "UPDATE Users SET
					is_admin=:is_admin
					WHERE local_username=:local_username ";
		$bindVarNVTArray = array(
			array(':local_username', $username, PDO::PARAM_STR),
			array(':is_admin', $isAdmin, PDO::PARAM_INT)
		);
		return $this->executeUpdateWithPreparedStatements($sql, 'UPDATE_USER', $bindVarNVTArray);
	}

	/**
	 * deleteUserId
	 *
	 * Deletes a user that is identified based on the userId.
	 *
	 * @param integer $userId
	 * @return boolean indicates whether the delete succeeded
	 */
	function deleteUserId($user_id) {
		$sql = "DELETE FROM Users WHERE user_id = :user_id";
		$bindVarNVTArray = array(array(':user_id', $user_id, PDO::PARAM_INT));
		$status = $this->executeDelete($sql, 'DELETE_USER_ID', $bindVarNVTArray);
		return $status;
	}

	/**
	 * deleteUsername
	 *
	 * Deletes a user that is identified based on the username.
	 *
	 * @param String $localUserName
	 * @return boolean indicates whether the delete succeeded
	 */
	public function deleteUsername($username) {
		$sql = "DELETE FROM Users WHERE local_username = :local_username ";
		$bindVarNVTArray = array( array(':local_username', $username, PDO::PARAM_STR));
		$status = $this->executeDelete($sql, 'DELETE_USERNAME', $bindVarNVTArray);
		return $status;
	}

	/**
	 * Deletes a user that is identified based on the localUserName (e.g. delete 'eric').
	 * @param String $localUserName
	 * @return boolean indicates whether the update succeeded
	 */
	function deleteLocalUser($username) {
		$sql = "DELETE FROM Users WHERE local_username = :local_username";
		$bindVarNVTArray = array( array(':local_username', $username, PDO::PARAM_STR));
		return $this->executeDelete($sql, 'DELETE_USER', $bindVarNVTArray);
	}

	/**
	 * deleteUserName
	 *
	 * Deletes a user that is identified based on the username.
	 *
	 * @param String $username
	 * @return boolean indicates whether the update succeeded
	 */
	public function delete_Username($username) {
		$userId = $this->getUserId($username);
		/* DELETE USER IN OS LAYER
		$os_status = DeleteUser($userId, $username);
		if (!$os_status) {
			return false;
		}
		*/
		$bindVarNVTArray = array( array(':username', $username, PDO::PARAM_STR));
		$status = $this->executeDelete(self::$queries['DELETE_USERNAME'], 'DELETE_USERNAME', $bindVarNVTArray);
		return $status;
	}

	/**
	 * Returns the admin status of the user identified by the supplied userId
	 * @param integer $userId identifier of the user (primary key of Users table)
	 * @return boolean admin status of the specified User
	 */
	function isAdmin($userId) {
		$sql = "SELECT COUNT(*) as count
		FROM Users
		WHERE user_id = :user_id
		 AND is_admin = 'true'";
		$bindVarNVTArray = array(array(':user_id', $userId, PDO::PARAM_INT));
		$rows = $this->executeQuery($sql, 'IS_ADMIN', $bindVarNVTArray);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', print_r($sql,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'rows', print_r($rows,true));
		$results = false;
		foreach ($rows as $row) {
			$count = $row['count'];
			if ($count > 0) {
				 $results = true;
			}
			break;
		}
		return $results;
	}

	/**
	 * Determines whether the specified Local Username exists in the Users table
	 * @param String $localUsername identifies the user being checked for validity
	 * @return boolean indicates if such a user exists.
	 */
	function isValid($username) {
		$sql = "SELECT COUNT(*) AS count
				FROM Users
				WHERE local_username = :local_username";
		$bindVarNVTArray = array( array(':local_username', $username, PDO::PARAM_STR));
		$rows = $this->executeQuery($sql, 'VALID_USER', $bindVarNVTArray);
		$retVal = false;
		foreach ($rows as $row) {
			$count = $row['count'];
			if($count == 1){
				 $retVal = true;
			}
			break;
		}
		return $retVal;
	}
}

/** TEST CODE **/
/*
$udb = new UsersDB();
$users = $udb->getUser();
var_dump($users);
$user = $udb->getUser(52);
var_dump($user);
*/
?>