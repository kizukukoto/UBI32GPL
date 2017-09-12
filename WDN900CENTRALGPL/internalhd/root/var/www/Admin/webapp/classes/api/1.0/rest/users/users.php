<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Users extends RestComponent {

	public function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('shareaccess.php');
		require_once('usersdb.inc');
		require_once('usersshell.php');
		require_once('util.inc');
	}

	
	private function _filterOutput($users) {
		if (key_exists(0, $users)) {
			//list of users
			foreach ($users as &$user) {
				$user = $this->_filterOutput($user);
			}
		} else if (isset($users['is_password']) && isset($users['username'])) {
			//single user
			$isPassword = isPasswordRequiredForLocalUser($users['username']);
			$users['is_password'] = $isPassword? 'true':'false';
		}
		
		return $users;
	}
	/**
	 * Get user information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $outputFormat
	 */
	public function get($urlPath, $queryParams=null, $outputFormat='xml') {
		if (isset($urlPath[0])) {
			if (is_numeric($urlPath[0])) {
				$userId = trim($urlPath[0]);
			} else {
				$username = trim($urlPath[0]);
			}
		} else {
			$userId   = isset($queryParams['user_id'])  ? trim($queryParams['user_id'])  : '';
			$username = isset($queryParams['username']) ? trim($queryParams['username']) : '';
		}
		$email = isset($queryParams['email']) ? trim($queryParams['email']) : '';

		$UsersDB  = new UsersDB();
		if (isAdmin(getSessionUserId()) || isLanRequest()) {

			if (!empty($email)) {

				$sessionUserId = getSessionUserId();
				if (!empty($userId) && $userId != $sessionUserId && !isAdmin(getSessionUserId())) {
					$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
					return;
				}

				if (strpos($email, '%') !== false) {
					if (!empty($user_id)) {
						### GET USERS WITH WHOM ADMIN HAS SHARED ALBUMS
						$userId = getSessionUserId();
						$users = $UsersDB->getUsersWithEmail($email, $userId);
					} else {
						### GET ALL USERS WITH PARTIAL EMAIL
						$userId = getSessionUserId();
						$users = $UsersDB->getUsersWithEmail($email);
					}
				} else {
					### GET USER WITH EXACT EMAIL MATCH
					$users = $UsersDB->getUserByEmail($email);
				}

				$this->generateCollectionOutput(200, 'users', 'user', $this->_filterOutput($users), $outputFormat);
				return;

			} else if (!empty($userId)) {
				$user = $UsersDB->getUser($userId);
				if (empty($user)) {
					$this->generateErrorOutput(404, 'users', 'USER_ID_NOT_FOUND', $outputFormat);
					return;
				}
			} else if (!empty($username)) {
				$userId = $UsersDB->getUserId($username);
				if (empty($userId)) {
					$this->generateErrorOutput(404, 'users', 'USER_NAME_NOT_FOUND', $outputFormat);
					return;
				}
				$user = $UsersDB->getUser($userId);
			} else {
				$users = $UsersDB->getUser();
				$this->generateCollectionOutput(200, 'users', 'user', $this->_filterOutput($users), $outputFormat);
				return;
			}

		} else {

			if (!empty($email)) {

				if (!empty($userId)) {
					$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
					return;
				}

				if (strpos($email, '%') !== false) {
					### GET USERS WITH WHOM THIS USER HAS SHARED ALBUMS
					$userId = getSessionUserId();
					$users = $UsersDB->getUsersWithEmail($email, $userId);
				} else {
					### GET USER WITH EXACT EMAIL MATCH
					$users = $UsersDB->getUserByEmail($email);
				}
				$this->generateCollectionOutput(200, 'users', 'user', $this->_filterOutput($users), $outputFormat);
				return;

			} else if (!empty($userId) || !empty($username)) {
				$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
				return;
			}
			$userId = getSessionUserId();
			$user = $UsersDB->getUser($userId);
		}
		if (!$user) {
			$this->generateErrorOutput(404, 'users', 'USER_GET_FAILED', $outputFormat);
			return;
		}
		$results = array('user' => $this->_filterOutput($user));
		$this->generateCollectionOutput(200, 'users', 'user', $results, $outputFormat);
	}


	/**
	 * Create a new user.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $outputFormat
	 */
	public function post($urlPath, $queryParams=null, $outputFormat='xml') {
		$username = isset($queryParams['username']) ? trim($queryParams['username']) : '';
		$password = isset($queryParams['password']) ? trim($queryParams['password']) : '';
		$fullname = isset($queryParams['fullname']) ? trim($queryParams['fullname']) : '';
		$isAdmin  = isset($queryParams['is_admin']) ? trim($queryParams['is_admin']) : '';
		$isAdmin  = $isAdmin == 'true' || $isAdmin == '1' ? 'true' : 'false';
		$UsersDB  = new UsersDB();

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'getSessionUserId', getSessionUserId());
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'isAdmin', isAdmin(getSessionUserId()));

		if (!isAdmin(getSessionUserId()) && !empty($username)) {
			$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}

		if (strlen($password) > 255) {
			$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
			return;
		}

		if (!empty($username)) {
			if (!preg_match('/^[a-zA-Z0-9_]+$/i', $username)  || strlen($username) > 32) {
				$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
				return;
			}
			$userId = $UsersDB->getUserId($username);
			if ($userId) {
				$this->generateErrorOutput(403, 'users', 'USER_NAME_EXISTS', $outputFormat);
				return;
			}
		}

		$userId = $UsersDB->createUser($username, $password, $fullname, $isAdmin);

		if (!empty($userId) && !empty($username)) {
			$usersShell = new UsersShell();
			$status = $usersShell->createUser($userId, $username, $password, $fullname);
			if (!$status) {
				$status = $UsersDB->deleteUser($userId);
				if (!$status) {
					$this->generateErrorOutput(500, 'users', 'INTERNAL ERROR: Failed to delete just created user', $outputFormat);
					return;
				}
				$this->generateErrorOutput(500, 'users', 'INTERNAL ERROR: Failed to create OS user', $outputFormat);
				return;
			}
		}

		if (!$userId) {
			$this->generateErrorOutput(500, 'users', 'USER_CREATE_FAILED', $outputFormat);
			return;
		}

		$results = array('status' => 'success', 'user_id' => $userId);
		$this->generateSuccessOutput(201, 'users', $results, $outputFormat);
	}


	/**
	 * Update user information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $outputFormat
	 */
	public function put($urlPath, $queryParams=null, $outputFormat='xml') {
		$UsersDB  = new UsersDB();

		if (isset($urlPath[0])) {
			$userId   = trim($urlPath[0]);
			$username = $UsersDB->getUsername($userId);
			if (!empty($username)) {
				if (!empty($queryParams['user_id']))  {
					$this->generateErrorOutput(400, 'users', 'CONFLICTING_PARAMETER', $outputFormat);
					return;
				}
			} else {
				$uname = trim($urlPath[0]);
				$uid   = $UsersDB->getUserId($uname);
				if (!empty($uid)) {
					$userId   = $uid;
					$username = $uname;
				}
			}
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'userId', $userId);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'username', $username);
		} else {
			if (isset($queryParams['user_id']) && !empty($queryParams['user_id'])) {
				$userId   = trim($queryParams['user_id']);
				if (isset($queryParams['username']) && !empty($queryParams['username'])) {
					$username = isset($queryParams['username']) ? trim($queryParams['username']) : '';
				} else {
					$username = $UsersDB->getUsername($userId);
				}
			} else if (isset($queryParams['username']) && !empty($queryParams['username'])) {
				$username = trim($queryParams['username']);
				if (isset($queryParams['user_id']) && !empty($queryParams['user_id'])) {
					$userId = isset($queryParams['user_id']) ? trim($queryParams['user_id']) : '';
				} else {
					$userId   = $UsersDB->getUserId($username);
				}
			}
		}

		if (empty($userId) && empty($username))  {
			$this->generateErrorOutput(400, 'users', 'MISSING_PARAMETER', $outputFormat);
			return;
		}
		if (empty($userId) || empty($username))  {
			$this->generateErrorOutput(404, 'users', 'USER_NOT_FOUND', $outputFormat);
			return;
		}

		$newUsername = isset($queryParams['username'])     ? trim($queryParams['username'])     : null;
		$fullname    = isset($queryParams['fullname'])     ? trim($queryParams['fullname'])     : null;
		$oldPassword = isset($queryParams['old_password']) ? trim($queryParams['old_password']) : null;
		$newPassword = isset($queryParams['new_password']) ? trim($queryParams['new_password']) : null;
		$isAdmin     = isset($queryParams['is_admin'])     ? trim($queryParams['is_admin'])     : null;

		// set boolean for signaling password change action 
		$changePassword = isset($queryParams['new_password']) ?  true : false;

		//printf("<PRE>%s.%s=[%s]</PRE>", __METHOD__, 'userId', $userId);

		### CHECK IF USERNAME IS NEW AND ALREADY EXISTS
		if (!empty($newUsername)) {
			$otherUserId = $UsersDB->getUserId($newUsername);
			if (!empty($otherUserId) && $otherUserId != $userId) {
				$this->generateErrorOutput(400, 'users', 'USER_EXISTS', $outputFormat);
				return;
			}
			if (strlen($newUsername) < 1 || strlen($newUsername) > 32) {
				$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
				return;
			}
			if (!preg_match('/^[a-zA-Z0-9_]+$/i', $newUsername)) {
				$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
				return;
			}
		}

		if (strlen($newPassword) > 255) {
			$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
			return;
		}

		if (isAdmin(getSessionUserId()) || $userId == getSessionUserId()) {
			if (empty($userId)) {
				if (empty($username)) {
					$userId = getSessionUserId();
				} else {
					$userId = $UsersDB->getUserId($username);
					if (empty($userId)) {
						$this->generateErrorOutput(404, 'users', 'USER_ID_NOT_FOUND', $outputFormat);
						return;
					}
				}
			}
		} else {
			if (!empty($userId) || !empty($username)) {
				$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
				return;
			}
			if (!empty($is_admin)) {
				$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
				return;
			}
			if (!empty($chg_pass)) {
				$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
				return;
			}
			$userId = getSessionUserId();
		}

		$passusername = $username;
		if (!empty($newUsername) && $newUsername != $username) {
			$passusername = $newUsername;
		}
		$status = $UsersDB->updateUser($userId, $passusername, $fullname, $oldPassword, $newPassword, $isAdmin, $changePassword);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', $status);

		if ($status) {
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'userId', $userId);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'username', $username);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'newUsername',  $newUsername);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'passusername', $passusername);
			$usersShell = new UsersShell();
			$status2 = $usersShell->updateUser($userId, $passusername, $fullname, $newPassword, $isAdmin, $changePassword);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status2', $status2);
		}

		if (!$status) {
			$this->generateErrorOutput(500, 'users', 'USER_UPDATE_FAILED', $outputFormat);
			return;
		}
		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'users', $results, $outputFormat);
	}


	/**
	 * Delete user.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $outputFormat
	 */
	public function delete($urlPath, $queryParams=null, $outputFormat='xml') {

		$UsersDB = new UsersDB();

		if (!empty($urlPath[0])) {
			$userId = isset($urlPath[0]) ? trim($urlPath[0]) : null;
			$user   = $UsersDB->getUser($userId);
			if (empty($user)) {
				$username = $userId;
				$userId   = $UsersDB->getUserId($username);
			}
		} else {
			$userId = isset($queryParams['user_id']) ? trim($queryParams['user_id']) : null;
			if (!empty($userId)) {
				$user = $UsersDB->getUser($userId);
				if (empty($user)) {
					$username = $userId;
					$userId   = $UsersDB->getUserId($username);
				}
			} else {
				$username = isset($queryParams['username']) ? trim($queryParams['username']) : null;
				if (!empty($username)) {
					$userId   = $UsersDB->getUserId($username);
				}
			}
		}

		if (empty($userId) && empty($username)) {
			$this->generateErrorOutput(400, 'users', 'USER_ID_MISSING', $outputFormat);
			return;
		}

		if ($username == 'admin') {
			$this->generateErrorOutput(400, 'users', 'INVALID_PARAMETER', $outputFormat);
			return;
		}

		if (!isAdmin(getSessionUserId())) {
			$this->generateErrorOutput(401, 'users', 'USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}

		// Delete any associated share access
		$ShareAccess = new ShareAccess();
		$status = $ShareAccess->deleteAllAccessForUser($userId);
		if ($status != 200) {
			$this->generateErrorOutput(500, 'users', 'DELETE_USER_ACCESS_FAILED', $outputFormat);
			return;
		}

		// ITR 35514 - cannot add Remote Access user
		//             (because user is not being deleted from the Central Server)
		// Delete all associated devices users
		$deviceUsersDb = new deviceUsersDb();
		$deviceUsers = $deviceUsersDb->getDeviceUsersForUserId($userId);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUsers', print_r($deviceUsers,true));
		foreach ($deviceUsers as $deviceUser) {
			$deviceUserId       = $deviceUser['device_user_id'];
			$deviceUserAuthCode = $deviceUser['auth'];
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUserId', $deviceUserId);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUserAuthCode', $deviceUserAuthCode);
			$status = deleteDeviceUser($deviceUserId, $deviceUserAuthCode);
			if (!$status) {
				$this->generateErrorOutput(500, 'users', 'DELETE_DEVICE_USERS_FAILED', $outputFormat);
				return;
			}
		}

		// Delete user from OS low-level
		$usersShell = new usersShell();
		$status = $usersShell->deleteUser($userId, $username);
		if (!$status) {
			$this->generateErrorOutput(500, 'users', 'DELETE_OS_USER_FAILED', $outputFormat);
			return;
		}

		// Delete user from database Users table
		$status = $UsersDB->deleteUser($userId);
		if (!$status) {
			$this->generateErrorOutput(500, 'users', 'USER_DELETE_FAILED', $outputFormat);
			return;
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'users', $results, $outputFormat);
	}
}

/*
// TEST CODE
// add root user for cli php testing
require_once('usershares.php');
$udb = new UsersDB();
$udb->createUser("root", "welc0me", "toor", 1, 1);
authenticateLocalUser("root", "welc0me");

//
// Create Users
//
$users = new Users();
unset ($queryParams);
$queryParams = array();
$queryParams['username'] = "larry1";
$queryParams['password'] = "Larry1Password";
$queryParams['fullname'] = "larry1larry";
$queryParams['is_admin'] = 0;
$queryParams['owner'] = "admin";
$queryParams['pw'] = "";
$path = array();
//$path[0] = "larry";
$users->post($path, $queryParams);
unset ($queryParams);
$queryParams = array();
$queryParams['username'] = "larry2";
$queryParams['password'] = "Larry2Password";
$queryParams['fullname'] = "larry2larry";
$queryParams['is_admin'] = 0;
$queryParams['owner'] = "admin";
$queryParams['pw'] = "";
$path = array();
//$path[0] = "larry";
$users->post($path, $queryParams);
$users = $udb->getUser();
var_dump($users);
//
// Delete Users - test locking
//
$pid = pcntl_fork();
if ($pid == -1) {
    die('could not fork');
} else if ($pid) {
    // we are the parent
    unset ($queryParams);
    $queryParams = array();
    $queryParams['username'] = "larry1";
    $queryParams['owner'] = "admin";
    $queryParams['pw'] = "";
    $path = array();
    $users->delete($path, $queryParams);
    pcntl_wait($status); //Protect against Zombie children
} else {
    // we are the child
    unset ($queryParams);
    $queryParams1 = array();
    $queryParams1['username'] = "larry2";
    $queryParams1['owner'] = "admin";
    $queryParams1['pw'] = "";
    $path1 = array();
    $users->delete($path1, $queryParams1);
}
*/
?>