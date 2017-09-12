<?php
// Copyright (c) 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('logMessages.php');
require_once('outputwriter.inc');
require_once('restcomponent.inc');
require_once('security.inc');
require_once('shareaccess.php');
require_once('usersdb.inc');
require_once('usershares.php');

class UserShareAccess extends RestComponent {

	static $shareAccess;
	var $logObj;

	public function __construct() {
		$this->logObj = new LogMessages();
		if (!isset(self::$shareAccess)) {
			self::$shareAccess = new ShareAccess();
		}
	}

	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @string array $ouputFormat
	 */
	public function get($urlPath, $queryParams=null, $outputFormat='xml') {
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $urlParts[5]);
		// Get specific ACL:  /share_access_configuration/{share Name}
		if (isset($urlPath[0]) && !(strcmp ($urlPath[0], '') == 0)) {
			$shareName = $urlPath[0];
		} else {
			$shareName = $queryParams['share_name'];
		}
		$userId = $queryParams['user_id'];
		if (isset($shareName) || isset($userId)) {
			$acl = self::$shareAccess->getAclForShare($shareName, $userId);
			if (isset($acl) && sizeof($acl) > 0) {
				$output = new OutputWriter(strtoupper($outputFormat));
				$output->pushElement('share_access_list');
				$output->element('share_name', $shareName);
				$output->pushArray("share_access");
				foreach ($acl as $access) {
					$output->pushArrayElement();
					$output->element('user_id', $access['user_id']);
					$output->element('username', $access['username']);
					$output->element('access', $access['access_level']);
					$output->popArrayElement();
				}
				$output->popArray();
 				$output->popElement();
 				$output->close();
			} else {
				$this->generateErrorOutput(404,'share_access', 'NO_SHARE_ACCESS', $outputFormat);
			}
		} else {
			$this->generateErrorOutput(400, 'share_access', 'PARAMETER_MISSING', $outputFormat);
		}
	}

	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function post($urlPath, $queryParams=null, $outputFormat='xml') {
		if (!empty($urlPath[0]) ){
			$shareName = isset($urlPath[0]) ? trim($urlPath[0]) : null;
		} else {
			$shareName = isset($queryParams['share_name']) ? trim($queryParams['share_name']) : null;
		}

		// CHECK IF SHARE EXISTS
		$userShares = new UserShares();
		$share = $userShares->getShares($shareName);
		if (empty($share)) {
			$this->generateErrorOutput(404, 'share_access', 'SHARE_NOT_FOUND', $outputFormat);
			return;
		}

		$userId = isset($queryParams['user_id']) ? trim($queryParams['user_id']) : null;

		// CHECK IF USER EXISTS
		$UsersDB = new UsersDB();
		$user = $UsersDB->getUser($userId);

		//printf("<PRE>%s.%s=[%s]</PRE>", __METHOD__, 'user', print_r($user,true));

		if (empty($user)) {
			$this->generateErrorOutput(404, 'share_access', 'USER_NOT_FOUND', $outputFormat);
			return;
		}

		$access = isset($queryParams['access']) ? trim($queryParams['access']) : null;

		// CHECK IF PARAMETER MISSING
		if (empty($shareName) || empty($userId) || empty($access)) {
			$this->generateErrorOutput(400, 'share_access', 'PARAMETER_MISSING', $outputFormat);
			return;
		}

		$result = self::$shareAccess->addAclForShare($shareName, $userId, $access);

		if ($result != 200) {
			switch ($result) {
				case 400: $msg = 'INVALID_PARAMETER'; break;
				case 401: $msg = 'USER_NOT_AUTHORIZED'; break;
				case 403: $msg = 'SHARE_ACCESS_EXISTS'; break;
				case 404: $msg = 'SHARE_NOT_FOUND'; break;
				case 500: $msg = 'CREATE_SHARE_ACCESS_FAILED'; break;
			}
			$this->generateErrorOutput($result, 'share_access', $msg, $outputFormat);
		} else {
			$this->generateSuccessOutput($result, 'share_access', array('status' => 'success'), $outputFormat);
		}
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	function put($urlPath, $queryParams=null, $outputFormat='xml') {
		if (!empty($urlPath[0]) ){
			$shareName = $urlPath[0];
		} else {
			$shareName = $queryParams['share_name'];
		}
		$userId = $queryParams['user_id'];
		$accessLevel = $queryParams['access'];
		if (isset($shareName) && isset($userId) && isset($accessLevel)) {
			$result = self::$shareAccess->updateAclForShare($shareName, $userId, $accessLevel );
		} else {
			$result = 400;
		}
		if ($result != 200) {
			switch ($result) {
				case 400: $msg = 'INVALID_PARAMETER'; break;
				case 401: $msg = 'USER_NOT_AUTHORIZED'; break;
				case 404: $msg = 'SHARE_NOT_FOUND'; break;
				case 500: $msg = 'UPDATE_SHARE_ACCESS_FAILED'; break;
			}
			$this->generateErrorOutput($result,'share_access', $msg, $outputFormat);
		} else {
			$this->generateSuccessOutput($result,'share_access', array('status' => 'success'), $outputFormat);
		}
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function delete($urlPath, $queryParams=null, $outputFormat='xml') {
		if (isset($urlPath[0]) && !(strcmp ( $urlPath[0], "") == 0) ){
			$shareName = $urlPath[0];
			$userId = $queryParams['user_id'];

			// CHECK IF SHARE EXISTS
			$userShares = new UserShares();
			$share = $userShares->getShares($shareName);
			if (empty($share)) {
				$this->generateErrorOutput(404, 'share_access', 'SHARE_NOT_FOUND', $outputFormat);
				return;
			}

			// CHECK IF USER HAS ACCESS TO SHARE
			$acl = self::$shareAccess->getAclForShare($shareName, $userId);
			if (empty($acl)) {
				$this->generateErrorOutput(404, 'share_access', 'USER_ACCESS_NOT_FOUND', $outputFormat);
				return;
			}

			if (isset($shareName) && isset($userId)) {
				$result = self::$shareAccess->deleteUserAccessForShare($shareName, $userId);
			} else {
				$result = 400;
			}
		} else {
			$result = 400;
		}
		if ($result != 200) {
			switch ($result) {
				case 400 : $msg = 'INVALID_PARAMETER'; break;
				case 401 : $msg = 'USER_NOT_AUTHORIZED'; break;
				case 404 : $msg = 'SHARE_NOT_FOUND'; break;
				case 500 : $msg = 'DELETE_SHARE_ACCESS_FAILED'; break;
			}
			$this->generateErrorOutput($result,'share_access', $msg, $outputFormat);
		} else {
			$this->generateSuccessOutput($result,'share_access', array('status' => 'success'), $outputFormat);
		}
	}
}


// TEST CODE
/*
require_once('usershares.php');
$udb = new UsersDB();
$udb->createUser("larry", $password="nub", $fullname="larry larryson", $is_admin=1, $chg_pass=1);
$udb->createUser("sue", $password="nub", $fullname="sue susans", $is_admin=1, $chg_pass=1);
$udb->createUser("barney", $password="nub", $fullname="barney barneyson", $is_admin=1, $chg_pass=1);
$userId1 = $udb->getUserId("larry");
$userId2 = $udb->getUserId("sue");
$userId3 = $udb->getUserId("barney");
$us = new UserShares();
$us->deleteAllUserShares();
$us->createShare($userId, "larry", "Burt's share", "disabled", "all", "enabled");
$us->createShare($userId2, "sue", "Burt's Other share", "disabled", "all", "enabled");
$us->createShare($userId3, "barney", "Ralph's share", "disabled", "all", "enabled");
//
// Add access to shares
//
$usa = new UserShareAccess();
$path = array();
$path[0] = "larry";
$queryParams = array();
$queryParams['user_id'] = $userId1;
$queryParams['access'] = 'RW';
$usa->post($path, $queryParams);
$queryParams = array();
$queryParams['user_id'] = $userId2;
$queryParams['access'] = 'RO';
$usa->post($path, $queryParams);
$path = array();
$path[0] = "sue";
$queryParams = array();
$queryParams['user_id'] = $userId2;
$queryParams['access'] = 'RW';
$usa->post($path, $queryParams);
$path = array();
$path[0] = "larry";
$queryParams = array();
$queryParams['user_id'] = $userId2;
$queryParams['access'] = 'RO';
$usa->put($path, $queryParams);
$queryParams = array();
$queryParams['user_id'] = $userId2;
$usa->delete($path, $queryParams);
//
// Get share ACLs
//
$path = array();
$path[0] = "larry";
$queryParams = array();
//$queryParams["user_id"] = 52;
//$usa->get($path, $queryParams);
$usa->get($path, NULL, 'json' );
$path = array();
$path[0] = "sue";
$usa->get($path);
$path = array();
$path[0] = "barney";
$usa->get($path);
*/
?>