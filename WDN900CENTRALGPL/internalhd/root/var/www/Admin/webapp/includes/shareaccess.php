<?php
require_once('usersdb.inc');
require_once('shareaccessdb.php');
require_once('shareaccessshell.php');

/**
 * Control class for controlling CrUD of Samba Share ACLs
 *
 * @author sapsford_j
 *
 */

class ShareAccess {

	static $usersDb;
	static $shareAccessDb;
	static $shareAccessShell;
	static $logObj;


	/**
	 * Constructor - creates static service class instances if they are not already instantiated
	 */
	public function __construct() {
		if (!isset(self::$usersDb)) {
			self::$usersDb = new UsersDB();
		}
		if (!isset(self::$shareAccessDb)) {
			self::$shareAccessDb = new ShareAccessDB();
		}
		if (!isset(self::$shareAccessShell)) {
			self::$shareAccessShell = new ShareAccessShell();
		}
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
	}


	/**
	 * Get the list of user_ids and access levels that have access to a share.
	 *
	 * Returns an array of assoc. arrays for a given share. Each array contains user_id and access_level for user.
	 * Format: [[user_id, access_level]]
	 *
	 * access_level is one of: "RO". "RW"
	 *
	 * @param string $shareName name of share
	 * @param string $userId id of user (optional)
	 */
	public function getAclForShare($shareName, $userId) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userId=$userId)");
		$acl = self::$shareAccessDb->getAclForShare($shareName, $userId);
		if (!empty($acl)) {
			for($i = 0; $i < sizeof($acl); $i++ ) {
				$acl[$i]['username'] = self::$usersDb->getUsername($acl[$i]['user_id']);
			}
		}
		return $acl;
	}


	/*
	 * Returns an array of assoc. arrays for all of the shares a given user has access to.
	 * Each array contains share_name and access_level for each share.
	 * Format: [[share_name, access_level]]
	 *
	 * access_level is one of: "RO". "RW"
	 *
	 * @param $userId - ID of user
	 */
	public function getSharesForUser($userId) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId)");
		$shares = self::$shareAccessDb->getSharesForUser($userId);
		return $shares;
	}


	public function addAclForShare($shareName, $userId, $accessLevel) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userId=$userId, accessLevel=$accessLevel)");
		$acl = self::$shareAccessDb->getAclForShare($shareName);
		if (isset($acl) && sizeof($acl) > 0) {
			foreach($acl as $userAccess) {
				if ($userAccess['user_id'] == $userId) {
					//ACL already exists
					return 403;
				}
			}
		}
		$userName = self::$usersDb->getLocalUsername($userId);
		if (!isset($userName)) {
			//invalid user id
			return 400;
		}
		$success = self::$shareAccessShell->addAclForShare($shareName, $userName, $accessLevel);
		if ($success) {
			$success = self::$shareAccessDb->addAclForShare($shareName, $userId, $accessLevel);
		}
		if (isset($success)) {
			return 200;
		}
		return 500;
	}

	public function updateAclForShare($shareName, $userId, $accessLevel) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userId=$userId, accessLevel=$accessLevel)");
		$acl = self::$shareAccessDb->getAclForShare($shareName);
		$exists = false;
		foreach($acl as $userAccess) {
			if ($userAccess['user_id'] == $userId) {
				//ACL exists
				$exists = true;
				break;
			}
		}
		if (!$exists) {
			//no ACL for this user
			return 404;
		}
		$userName = self::$usersDb->getLocalUsername($userId);
		if (!isset($userName)) {
			//invalid user id
			return 400;
		}
		$success = self::$shareAccessShell->updateAclForShare($shareName, $userName, $accessLevel);
		if ($success) {
			$success = self::$shareAccessDb->updateAclForShare($shareName, $userId, $accessLevel);
		}
		if ($success) {
			return 200;
		}
		return 500;
	}


	public function deleteUserAccessForShare($shareName, $userId, $checkShareExists = true) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userId=$userId, checkShareExists=$checkShareExists)");
		
		if ($checkShareExists) {
			$acl = self::$shareAccessDb->getAclForShare($shareName);
			if (!isset($acl) || sizeof($acl) == 0) {
				//ACL doesn't exist
				return 500;
			}
		}
		$userName = self::$usersDb->getLocalUsername($userId);
		if (!isset($userName)) {
			//invalid user id
			return 400;
		}
		$success = self::$shareAccessShell->deleteAclForShare($shareName, $userName);
		if ($success) {
			$success = self::$shareAccessDb->deleteUserAccessForShare($shareName, $userId);
		}
		if ($success) {
			return 200;
		}
		return 500;
	}


	public function deleteAllAccessForShare($shareName) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
		$acl = self::$shareAccessDb->getAclForShare($shareName);
		if (!isset($acl) || sizeof($acl) == 0) {
			//ACL doesn't exist
			return 500;
		}
		foreach($acl as $userAccess) {
			$status = $this->deleteUserAccessForShare($shareName, $userAccess['user_id'], false);
			if ($status != 200) {
				break;
			}
		}
		if ($status == 200) {
			$success = self::$shareAccessDb->deleteAllAccessForShare($shareName);
			if ($success) {
				return 200;
			}
		}
		return 500;
	}


	public function deleteAllAccessForUser($userId) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId)");
		$userName = self::$usersDb->getLocalUsername($userId);
		if (!isset($userName)) {
			//invalid user id
			return 400;
		}
		$success = self::$shareAccessShell->deleteAllAclsForUser($userName);
		if ($success) {
			$success = self::$shareAccessDb->deleteAllAccessForUser($userId);
		}
		if ($success) {
			return 200;
		}
		return 500;
	}
}

/*
// TEST CODE
$sa = new ShareAccess();
$shares = $sa->getSharesForUser(52);
$sa->addAclForShare("burt1", 1, "RW");
$sa->addAclForShare("burt2", 1, "RW");
$sa->addAclForShare("ralph", 2, "RW");
$sa->addAclForShare("huey", 3, "RW");
$sa->addAclForShare("burt2", 2, "RO");
$sa->addAclForShare("burt2", 3, "RW");
$sa->addAclForShare("huey", 1, "RO");
$sa->addAclForShare("huey", 2, "RW");
$acl = $sa->getAclForShare("larry",52);
var_dump($acl);
$acl = $sa->getAclForShare("burt2");
var_dump($acl);
$acl = $sa->getAclForShare("huey");
var_dump($acl);
$sa->updateAclForShare("burt2", 3, "RO");
$acl = $sa->getAclForShare("burt2");
var_dump($acl);
$sa->updateAclForShare("huey", 1, "RW");
$acl = $sa->getAclForShare("huey");
var_dump($acl);
$sa->deleteUserAccessForShare("burt2",2);
$acl = $sa->getAclForShare("burt2");
var_dump($acl);
$sa->deleteAllAccessForShare("burt2");
$acl = $sa->getAclForShare("burt2");
var_dump($acl);
$sa->deleteAllAccessForUser(2);
*/
?>