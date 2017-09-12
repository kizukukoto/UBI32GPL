<?php
require_once('logMessages.php');
require_once('outputwriter.inc');
require_once('restcomponent.inc');
require_once('security.inc');
require_once('shareaccess.php');
require_once('usersdb.inc');
require_once('usershares.php');

class UserShareControl extends RestComponent {

	static $userShares;
	static $shareAccess;
	
	static $medias = array('any','pictures_only','music_only','videos_only','all','photos','music','videos','none');

	var $logObj;
	var $shareDetails = null;

	/**
	 * Constructor, creates UserShares and ShareAccess instances
	 */
	function __construct() {
		$this->logObj = new LogMessages();
		if (!isset(self::$userShares)) {
			self::$userShares = new UserShares();
		}
		if (!isset(self::$shareAccess)) {
			self::$shareAccess = new ShareAccess();
		}
	}

	/**
	 * Function to get share info for a specific share (if share name is provided), or
	 * all shares (if no share name is provided)
	 * 
	 * @param string $shareName name of share, or null to get info for all shares
	 */
	private function getShares($shareName=null) {
		$shares = self::$userShares->getShares($shareName);
		return $shares;
	}

	private function createShare($shareName, $queryParams) {
		if (strcasecmp($shareName, 'public') == 0) {
			return 403;
		}
		$description  = isset($queryParams['description'])   ? $queryParams['description']   : '';
		$mediaServing = isset($queryParams['media_serving']) ? $queryParams['media_serving'] : '';
		$publicAccess = isset($queryParams['public_access']) ? $queryParams['public_access'] : '';
		$remoteAccess = isset($queryParams['remote_access']) ? $queryParams['remote_access'] : '';
		if (!empty($mediaServing) && 
			!in_array($mediaServing, self::$medias) ) {
				return 400;
		}
		if (!empty($publicAccess)) {
			if ($publicAccess != 'true' && $publicAccess != 'false') {
				if (is_numeric($publicAccess)) {
					if ( intval($publicAccess) == 1) {
						$publicAccess = 'true';
					} else if ( intval($publicAccess) == 0) {
						$publicAccess = 'false';
					}
				} else {
					return 400;
				}
			}
			if ($publicAccess != 'true' && $publicAccess != 'false') {
				return 400;
			}
		}
		if (!empty($remoteAccess)) {
			if ($remoteAccess != 'true' && $remoteAccess != 'false') {
				if (is_numeric($remoteAccess)) {
					if ( intval($remoteAccess) == 1) {
						$remoteAccess = 'true';
					} else if ( intval($remoteAccess) == 0) {
						$remoteAccess = 'false';
					}
				} else {
					return 400;
				}
			}
			if ($remoteAccess != 'true' && $remoteAccess != 'false') {
				return 400;
			}
		}
		$status = self::$userShares->createShare($shareName, $description, $publicAccess, $mediaServing, $remoteAccess);
		if ($status === false) {
			return 500;
		}
		return 200;
	}

	private function updateShare($shareName, $queryParams) {
		$newShareName = isset($queryParams['new_share_name']) ? trim($queryParams['new_share_name']) : null;
		$description  = isset($queryParams['description'])    ? trim($queryParams['description'])    : null;
		$mediaServing = isset($queryParams['media_serving'])  ? trim($queryParams['media_serving'])  : null;
		$publicAccess = isset($queryParams['public_access'])  ? trim($queryParams['public_access'])  : null;
		$remoteAccess = isset($queryParams['remote_access'])  ? trim($queryParams['remote_access'])  : null;

		$this->logObj->LogData('APITEST', get_class($this),  __FUNCTION__,  'UPDATESHARE' . ": " . $shareName);

		if (strcasecmp($shareName, 'Public') == 0 && !empty($newShareName) && strcasecmp($shareName, $newShareName) != 0 ) {
			//can't rename an existing share to be 'Public'
			return 403;
		}
		if (!empty($mediaServing) && 
			!in_array($mediaServing, self::$medias) ) {
						return 400;
		}
		if (!empty($publicAccess)) {
			if ($publicAccess != 'true' && $publicAccess != 'false') {
				if (is_numeric($publicAccess)) {
					if ( intval($publicAccess) == 1) {
						$publicAccess = 'true';
					} else if ( intval($publicAccess) == 0) {
						$publicAccess = 'false';
					}
				} else {
					return 400;
				}
			}
			if ($publicAccess != 'true' && $publicAccess != 'false') {
				return 400;
			}
		}
		if (!empty($remoteAccess)) {
			if ($remoteAccess != 'true' && $remoteAccess != 'false') {
				if (is_numeric($remoteAccess)) {
					if ( intval($remoteAccess) == 1) {
						$remoteAccess = 'true';
					} else if ( intval($remoteAccess) == 0) {
						$remoteAccess = 'false';
					}
				} else {
					return 400;
				}
			}
			if ($remoteAccess != 'true' && $remoteAccess != 'false') {
				return 400;
			}
		}
		$status = self::$userShares->updateShare($shareName, $newShareName, $description, $publicAccess, $mediaServing, $remoteAccess);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', print_r($status,true));

		if ($status === false) {
			return 500;
		}
		return 200;
	}

	/**
	 * Delete the share with the given share name and parameters
	 * 
	 * @param string $shareName
	 * @param array $queryParams
	 */
	private function deleteShare($shareName, $queryParams) {
		$status = self::$userShares->deleteUserShare($shareName);
		if ($status === false) {
			return 500;
		}
		return 200;
	}

	/**
	 * Extract the shareName from the URL or qury params and call the 
	 * function passed in $ShareFunction to process the request
	 * 
	 * @param array $urlPath URI elements
	 * @param array $queryParams q=URL query parameters 
	 * @param string $outputFormat 
	 * @param string $shareFunction
	 */
	private function processUserShare($urlPath, $queryParams, $outputFormat, $shareFunction) {
		if (!empty($urlPath[0])) {
			$shareName = trim($urlPath[0]);
		} else if (isset($queryParams["share_name"])) {
			$shareName = trim($queryParams["share_name"]);
		} else {
			$this->generateErrorOutput(400, 'shares', 'SHARE_NAME_MISSING', $outputFormat);
			return;
		}
		if (strlen($shareName) < 1 || strlen($shareName) > 32) {
			$this->generateErrorOutput(400, 'shares', 'INVALID_SHARE_NAME', $outputFormat);
			return;
		}
		if (!preg_match('/^[a-zA-Z0-9_]+$/i', $shareName)) {
			$this->generateErrorOutput(400, 'shares', 'INVALID_SHARE_NAME', $outputFormat);
			return;
		}
		if ($shareFunction == 'createShare') {
			// CHECK IF SHARE ALREADY EXISTS
			$share = self::$userShares->getShares($shareName);
			if (!empty($share)) {
				$this->generateErrorOutput(403, 'share_access', 'SHARE_ALREADY_EXISTS', $outputFormat);
				return;
			}
		}
		if ($shareFunction == 'updateShare' || $shareFunction == 'deleteShare') {
			// CHECK IF SHARE DOES EXIST
			$share = self::$userShares->getShares($shareName);
			if (empty($share)) {
				$this->generateErrorOutput(404, 'share_access', 'SHARE_NOT_FOUND', $outputFormat);
				return;
			}
		}

		//call function passed-in to perform desired operation
		$result = $this->$shareFunction($shareName, $queryParams);
		if ($result != 200) {
			switch($result) {
			case 400:
				$msg = "INVALID_PARAMETER";
				break;
			case 403:
				$msg = "OPERATION_FORBIDDEN";
				break;
			case 404:
				$msg = "SHARE_NOT_FOUND";
				break;
			case 500:
				$msg = "SHARE_FUNCTION_FAILED";
				break;
			default:
				$msg = "UNKNOWN_ERROR";
				break;
			}
			$this->generateErrorOutput($result, 'shares', $msg, $outputFormat);
		} else {
			$msg = 'success';
			$this->generateSuccessOutput($result, 'shares', array('status' => 'success'), $outputFormat);
		}
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result . ": " . $msg);
	}

	/**
	 * Handle HTTP GET - get share info.
	 * 
	 * @param array $urlPath URI elements
	 * @param array $queryParams q=URL query parameters 
	 * @param string $outputFormat 
	 */
	public function get($urlPath, $queryParams=null, $outputFormat='xml') {
		$usersDb = new UsersDB();
		$userId  = getSessionUserId();
		$isAdmin = $usersDb->isAdmin($userId);
		$includeWdSyncFolder = isset($queryParams['include_wd_sync_folder'])  ? trim($queryParams['include_wd_sync_folder'])  : false;
		$includeWdSyncFolder = ($includeWdSyncFolder === 'true' || $includeWdSyncFolder === '1'  || $includeWdSyncFolder === 1) ? true : false;
		
		/* share.type Temp. comment out by Andrew	
		$shareType = isset($queryParams['share_type']) ? trim($queryParams['share_type']) : 'all';
		*/
		
		if (isset($queryParams["show_all"]) ) {
			$showAll =  ($queryParams["show_all"] === 'true') ? true : false;
		} else {
			$showAll = true;
		}
		//$this->logObj->LogParameters(get_class($this), __FUNCTION__, $urlPath[5]);
		if (isset($urlPath[0]) && !(strcmp( $urlPath[0], "") == 0) ){
				$shareName = trim($urlPath[0]);
		} else {
			if (isset($queryParams["share_name"]) ) {
				$shareName = $queryParams["share_name"];
			}
		}
		if (isset($shareName)) {
			$shareDetails = $this->getShares($shareName);
		} else {
			$shareDetails = $this->getShares();
		}
		$found = false;
		if (isset($shareDetails)) {
			foreach($shareDetails as $share){
				$authorized = false;
				if (empty($share['share_name'])) {
					continue;
				}
				$shareAccessList = self::$shareAccess->getAclForShare($share['share_name'], ($isAdmin && $showAll) ? NULL : $userId );
				if ( ($isAdmin && $showAll) || (strcasecmp($share['public_access'],'true') == 0)) {
					$authorized = true;
				}
				else {
					//get acl for share, if user is not in acl, then they must be admin to see it
					foreach ($shareAccessList as $shareAccess) {
						if ($shareAccess['user_id'] == $userId) {
							$authorized = true;
							break;
						}
					}
				}
				if (!$authorized) {
					if (isset($shareName)) {
						//single share requested, but user is anuthorized to access it
						$this->generateErrorOutput(401,'shares','USER_NOT_AUTHORIZED', $outputFormat);
						return;
					}
					else {
						//all shares requested, filter out the shares that the user does not have access to
						continue;
					}
				}
				if (!$found) {
					$found = true;
					$output = new OutputWriter(strtoupper($outputFormat));
					$output->pushElement("shares");
					$output->pushArray('share');
				}
				
				/* share.type Temp. comment out by Andrew	
				// determinate output by shareType (all / static / dynamic)
				if ($shareType == 'all' // case of all
					|| (($share['dynamic_volume'] == 'false' || empty($share['dynamic_volume']) ) && $shareType == 'static') // case of static 
					|| ($share['dynamic_volume'] == 'true' && $shareType == 'dynamic')) { // case of dynamic
				*/
					$output->pushArrayElement('share');
					$output->element('share_name', $share['share_name']);
					$output->element('sharename', $share['share_name']);
					$output->element('description', $share['description']);
					$output->element('sharedesc', $share['description']);
					if($includeWdSyncFolder){
						$syncFolderPath = getSharePath().'/'.$share['share_name'].'/'.'WD Sync';
						if(is_dir($syncFolderPath)) {
							$output->element('wd_sync_folder', 'true');
						}else{
							$output->element('wd_sync_folder', 'false');
						}
					}
					
					$output->element('size', $share['size']);
					//convert legacy DB values for boolean values to string equivalents
					$output->element('remote_access', $share['remote_access']);
					$output->element('public_access', $share['public_access']);
					$output->element('media_serving', $share['media_serving']);
	                $output->element('dynamic_volume', $share['dynamic_volume']);
	                if (strcasecmp($share['dynamic_volume'],'true') == 0) {
	                    $output->element('capacity', $share['capacity']);
	                    $output->element('read_only', $share['read_only']);
	                    $output->element('usb_handle', $share['usb_handle']);
	                    $output->element('file_system_type', $share['file_system_type']);
	                }
					if(!empty($shareAccessList)) {
						$output->pushArray('share_access');
						foreach ($shareAccessList as $access) {
							$output->pushArrayElement('share_access');
							$output->element('user_id', $access['user_id']);
							$output->element('username', $access['username']);
							$output->element('access', $access['access_level']);
							$output->popArrayElement();
						}
						$output->popArray();
					}
					$output->popArrayElement();
				/* share.type Temp. comment out by Andrew	
				} // EOF shareType			
				*/
			}
			
			if ($found) {
				$output->popArray();
				$output->popElement();
				$output->close();
			}
		}
		if (!$found) {
			$this->generateErrorOutput(404, 'shares', 'SHARE_NOT_FOUND', $outputFormat);
			return;
		}
	}

	/**
	 * Handle HTTP POST - create a share
	 * 
	 * @param unknown_type $urlPath
	 * @param unknown_type $queryParams
	 * @param unknown_type $outputFormat
	 */
	public function post($urlPath, $queryParams=null, $outputFormat='xml') {
		$usersDb = new UsersDB();
		$userId  = getSessionUserId();
		$isAdmin = $usersDb->isAdmin($userId);
		if ($isAdmin) {
			return $this->processUserShare($urlPath, $queryParams, $outputFormat, 'createShare');
		} else {
			$this->generateErrorOutput(401,'shares', 'USER_NOT_AUTHORIZED', $outputFormat);
			return FALSE;
		}
	}

	/**
	 * Handle HTTP PUT - update a share 
	 * 
	 * @param array $urlPath URI elements
	 * @param array $queryParams q=URL query parameters 
	 * @param string $outputFormat 
	 */
	public function put($urlPath, $queryParams=null, $outputFormat='xml') {
		if (!empty($urlPath[0])) {
			$shareName = trim($urlPath[0]);
		} else if (isset($queryParams['share_name'])) {
			$shareName = trim($queryParams['share_name']);
		} else {
			$this->generateErrorOutput(400, 'shares', 'SHARE_NAME_MISSING', $outputFormat);
			return;
		}
		$newShareName = isset($queryParams['new_share_name']) ? $queryParams['new_share_name'] : '';
		if (strcasecmp($shareName, 'Public') == 0 && !empty($newShareName) && strcasecmp($shareName, $newShareName) != 0 ) {
			$this->generateErrorOutput(403, 'shares', 'RENAME_PUBLIC_FORBIDDEN', $outputFormat);
			return;
		}
		$usersDb = new UsersDB();
		$userId  = getSessionUserId();
		$isAdmin = $usersDb->isAdmin($userId);
		if (!$isAdmin) {
			$this->generateErrorOutput(401, 'shares', 'USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}
		return $this->processUserShare($urlPath, $queryParams, $outputFormat, 'updateShare');
	}

	/**
	 * Handle HTTP Delete - delete a share
	 * 
	 * @param array $urlPath URI elements
	 * @param array $queryParams q=URL query parameters 
	 * @param string $outputFormat 
	 */
	public function delete($urlPath, $queryParams=null, $outputFormat='xml') {
		if (!empty($urlPath[0])) {
			$shareName = trim($urlPath[0]);
		} else if (isset($queryParams['share_name'])) {
			$shareName = trim($queryParams['share_name']);
		} else {
			$this->generateErrorOutput(400, 'shares', 'SHARE_NAME_MISSING', $outputFormat);
			return;
		}

		if (strcasecmp($shareName, 'Public') == 0) {
			$this->generateErrorOutput(403, 'shares', 'DELETE_PUBLIC_FORBIDDEN', $outputFormat);
			return;
		}

		$usersDb = new UsersDB();
		$userId  = getSessionUserId();
		$isAdmin = $usersDb->isAdmin($userId);
		if (!$isAdmin) {
			$this->generateErrorOutput(401, 'shares', 'USER_NOT_AUTHORIZED', $outputFormat);
			return;
		}
		return $this->processUserShare($urlPath, $queryParams, $outputFormat, 'deleteShare');
	}
}

/** TEST CODE **/
/*
// add root user for cli php testing
//require_once('usershares.php');
$udb = new UsersDB();
//$udb->createUser("root", "welc0me", "toor", 1, 1);
//authenticateLocalUser("root", "welc0me");
$udb = new UsersDB();
$udb->deleteLocalUser("larry");
$udb->deleteLocalUser("sue");
$udb->deleteLocalUser("barney");
$udb->createUser("larry", $password="nub", $fullname="larry larryson", $is_admin=1, $chg_pass=1);
$udb->createUser("sue", $password="nub", $fullname="sue susans", $is_admin=1, $chg_pass=1);
$udb->createUser("barney", $password="nub", $fullname="barney barneyson", $is_admin=1, $chg_pass=1);
$userId1 = $udb->getUserId("larry");
$userId2 = $udb->getUserId("sue");
$userId3 = $udb->getUserId("barney");
$usc = new UserShareControl();
//
// Create Shares
//
$queryParams = array();
$queryParams['share_desc'] = "Larry's share";
$queryParams['public_access'] = "disabled";
$queryParams['media_serving'] = "all";
$queryParams['remote_access'] = "enabled";
$queryParams['share_name'] = "larry";
$path = array();
//$path[0] = "larry";
$usc->post($path, $queryParams);
$queryParams['share_desc'] = "Sue's share";
$queryParams['public_access'] = "disabled";
$queryParams['media_serving'] = "all";
$queryParams['remote_access'] = "enabled";
$queryParams['share_name'] = "sue";
$path = array();
//$path[0] = "sues";
$usc->post($path, $queryParams);
$queryParams['share_desc'] = "Barney's share";
$queryParams['public_access'] = "disabled";
$queryParams['media_serving'] = "all";
$queryParams['remote_access'] = "enabled";
$queryParams['share_name'] = "barney";
$path = array();
$path[0] = "Barney";
$usc->post($path, $queryParams);
//
//Add to acl
//
require_once('usershareaccess.php');
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
$queryParams['user_id'] = $userId2;
$queryParams['access'] = 'RO';
$usa->post($path, $queryParams);
//
// Get Shares
//
$queryParams = array();
$path = array();
$path[0] = "larry";
$usc->get($path, $queryParams);
$path = array();
$queryParams = array();
$usc->get($path, $queryParams);
//
// Update share
//
$queryParams = array();
$queryParams['share_desc'] = "Larry's updated share";
$queryParams['new_share_name'] = "larry_changed";
$queryParams['public_access'] = "enabled";
$queryParams['media_serving'] = "videos";
$queryParams['remote_access'] = "disabled";
$path = array();
$path[0] = "larry";
$usc->put($path, $queryParams);
//
// Delete share
//
$queryParams = array();
$path = array();
$path[0] = "larry_changed";
$usc->delete($path, $queryParams);
/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>