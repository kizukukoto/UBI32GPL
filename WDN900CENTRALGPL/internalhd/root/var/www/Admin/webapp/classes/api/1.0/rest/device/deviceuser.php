<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class DeviceUser extends RestComponent {

	function __construct() {
		require_once('deviceusersdb.inc');
		require_once('device.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
		require_once('userlist.php');
	}

	private function _checkValidUser($userId) {
		$userdb = new UsersDB();
		$user = $userdb->getUser($userId);
		
		return $user;		
	}
	
	private function addDeviceUser($urlPath, $queryParams) {
		$email  = isset($queryParams['email']) ? $queryParams['email'] : null;
		$userId = isset($queryParams['user_id']) ? $queryParams['user_id'] : getSessionUserId();
		$sender = isset($queryParams['sender']) ? $queryParams['sender'] : null;
			
		$deviceId = getDeviceId();
		$restart = false;
		
		if (!$this->_checkValidUser($userId)) {
			throw new Exception('USER_ID_NOT_FOUND', 404);
		}
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceId', $deviceId);
		if (empty($deviceId)) {
			//if remote access is enabled and device is not registered, register the device
			$remoteAccess = getRemoteAccess();	
			
			if ( strcasecmp($remoteAccess, "TRUE") == 0) {
				//register the device
				$registeredDevice = DeviceControl::getInstance()->registerDevice(); 
				if ($registeredDevice == false) {
					//can't start services if device is not registered
					return 403;
				}
				$restart = true;
			}
		}
		
		
		$ret = null;
		if (!empty($email)) {
			//check if device user with this e-mail address already exists for this user
			$deviceUsersDb = new DeviceUsersDB();
			$webuser = $deviceUsersDb->getWebUserByMail($email);
			if (!empty($webuser)) {
				throw new Exception('USER_NAME_EXISTS', 403);
			}
			//$deviceUser = $deviceUsersDb->getDeviceUserForUserIdWithEmail($userId, $email);
			//if (isset($deviceUser) && $deviceUser != false) {
			//	//DU with this email address already exists for this user
			//	return false;
			//}
			$status = addEmailAccessToUser($userId, $email, $sender);
			if ($status !== false) {
				//check status of orion services and start them if necessary depending on remote access setting
				updateRemoteServices();	
		
			}
			$ret = $status;
		} else {
			//'null' user - we need to get a device activation code for this device user
			$dacFlag = isset($queryParams['dac']) ? $queryParams['dac'] : 1;
			$DAC = addDACAccessToUser($userId, $dacFlag);
			if ($DAC) {
				//check status of orion services and start them if necessary depending on remote access setting
				updateRemoteServices();
			}
			$ret = $DAC;
		}
		if ($restart) {
			//reload apache config because we got new certificate
			$ulObj = new Userlist();
			$ulObj->reloadApacheConfig(STINGRAY_DEVICE_TYPE);
		}
		return $ret;
	}

	private function modifyDeviceUser($urlPath, $queryParams) {
		$deviceUserId = $urlPath[0];
		$deviceType   = isset($queryParams['type'])  ? trim($queryParams['type'])  : null;
		$deviceName   = isset($queryParams['name'])  ? trim($queryParams['name'])  : null;
		$deviceEmail  = isset($queryParams['email']) ? trim($queryParams['email']) : null;
		$isActive     = isset($queryParams['is_active']) && $queryParams['is_active'] == 'false' ? false : true;
		$typeName     = isset($queryParams['type_name'])   ? trim($queryParams['type_name'])   : null;
		$application  = isset($queryParams['application']) ? trim($queryParams['application']) : null;
		$resendEmail  = !empty($queryParams['resend_email']) && $queryParams['resend_email'] == 'true' ? 'true' : 'false';
		$sender   = isset($queryParams['sender'])  ? trim($queryParams['sender'])  : null;
		
		if ($resendEmail == 'true') {
			$result = resendEmail($deviceUserId, $sender);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'result', $result);
			return $result;
		}

		if (empty($deviceUserId) || empty($deviceType) || empty($deviceName)) {
			return 400;
		}

		if (!is_numeric($deviceUserId) || $deviceUserId < 0) {
			return 401;
		}

 		$result = updateDeviceUser($deviceUserId, $deviceType, $deviceName, $deviceEmail, $isActive, $typeName, $application);
 		//The only time an update is done is when the deviceUser is first registered.
 		//When this happens, we need to do a callback to notify the system
 		if ($result) {
 			$this->putDeviceUserNotify($deviceUserId);
 			if($deviceUserArr = getDeviceUsers($deviceUserId) && isset($deviceUserArr['auth'])) {
 				updateApacheHtpassword('UPDATE', $deviceUserId, $deviceUserArr['auth']);
				// reload apache configuration
				//Comment out apache reload because of ITR No. 49175 Abstract: Consecutive addition of device user with email fails
				//$ulObj = new Userlist();
				//$ulObj->reloadApacheConfig(STINGRAY_DEVICE_TYPE);
 			}
 		}
 		return $result;
	}

	/**
	 * If PUT_DEVICE_USER_NOTIFICATION_SCRIPT is defined in the device section of globalconfig.ini, this routine
	 * will call the defined script with the parameters of 'update <deviceUserId>'.
	 */
	private function putDeviceUserNotify($deviceUserId) {
		$deviceConfig = getGlobalConfig('device');
		if (isset($deviceConfig) && sizeof($deviceConfig) > 0) {
			if (isset($deviceConfig['PUT_DEVICE_USER_NOTIFICATION_SCRIPT']) &&
			  sizeof($deviceConfig['PUT_DEVICE_USER_NOTIFICATION_SCRIPT']) > 0 ) {
				$putDeviceUserNotificationScript = $deviceConfig['PUT_DEVICE_USER_NOTIFICATION_SCRIPT'];
				system($putDeviceUserNotificationScript.' update '.$deviceUserId, $response);
			}
		}
	}

	private function removeDeviceUser($urlPath, $queryParams) {
		$deviceUserId = $urlPath[0];
		$deviceUserAuthCode = $queryParams['device_user_auth_code'];
		
		if (isset($deviceUserId)) {
			$status = deleteDeviceUser($deviceUserId, $deviceUserAuthCode);
			if ($status) {
				//check and stop services if they are running, if there are no device users
				updateRemoteServices();
				updateApacheHtpassword('DELETE', $deviceUserId);
				// reload apache configuration
				//Comment out apache reload because of ITR No. 49175 Abstract: Consecutive addition of device user with email fails
				//$ulObj = new Userlist();
				//$ulObj->reloadApacheConfig(STINGRAY_DEVICE_TYPE);				
			}
		}
		return $status;
	}

	private function mapDeviceUserItem($deviceUser) {
		if (isset($deviceUser)) {
			$item = array(
				'device_user_id'        => $deviceUser['device_user_id'],
				'device_user_auth_code' => $deviceUser['auth'],
				'user_id'               => $deviceUser['user_id'],
				'device_reg_date'       => $deviceUser['created_date'],
				'type'                  => stripslashes($deviceUser['type']),
				'name'                  => stripslashes($deviceUser['name']),
				'active'                => $deviceUser['is_active'],
				'email'                 => $deviceUser['email'],
				'dac'                   => $deviceUser['dac'],
				'dac_expiration'        => $deviceUser['dac_expiration'],
				'type_name'             => $deviceUser['type_name'],
				'application'           => $deviceUser['application']
				);
			return $item;
		}
		return null;
	}

	function get($urlPath, $queryParams=null, $output_format='xml') {

		$deviceUserId = !empty($urlPath[0]) ? $urlPath[0] : null;

		if (!empty($deviceUserId)) {
			$deviceUser = getDeviceUser($deviceUserId);

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUser', $deviceUser);

			if (isset($deviceUser) && $deviceUser != false) {
				$item = $this->mapDeviceUserItem($deviceUser);
				$this->generateItemOutput(200, 'device_user', $item, $output_format);
			} else {
				$this->generateErrorOutput(404, 'device_user', 'DEVICE_USER_NOT_FOUND', $output_format);
			}
		} else {
			$userId = isset($queryParams['user_id']) ? $queryParams['user_id'] : getSessionUserId();
			if (!$this->_checkValidUser($userId)) {
				throw new Exception('USER_ID_NOT_FOUND', 404);
			}
			$deviceUserDb = new DeviceUsersDB();
			$deviceUsers = $deviceUserDb ->getDeviceUsersForUserId($userId);

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUsers', print_r($deviceUsers,true));

			$items = array();
			foreach ($deviceUsers as $deviceUser) {
				$item = $this->mapDeviceUserItem($deviceUser);
				array_push($items, $item);
			}
			$this->generateCollectionOutput(200, 'device_users', 'device_user', $items, $output_format);
		}
	}

	function post($urlPath, $queryParams=null, $output_format='xml') {
		$results = $this->addDeviceUser($urlPath, $queryParams);

		if ($results === 403) {
			$this->generateErrorOutput(403, 'device_user', 'DEVICE_NOT_REGISTERED', $output_format);
			return;
		}

		if ($results === false) {
			$this->generateErrorOutput(500, 'device_user', 'DEVICE_USER_CREATE_FAILED', $output_format);
		} else {
			$results['status'] = 'success';
			$this->generateSuccessOutput(201, 'device_user', $results, $output_format);
		}
	}

	function put($urlPath, $queryParams=null, $output_format='xml') {
		$putData = $this->getPutData();
		if($putData){
			foreach($putData as $key => $val){
				$queryParams[$key] = $val;
			}
		}
		if (!isset($urlPath[0])) {
			$this->generateErrorOutput(400, 'device_user', 'DEVICE_USER_ID_MISSING', $output_format);
			return;
		}

		$status = $this->modifyDeviceUser($urlPath, $queryParams);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', $status);

		if ($status === 400) {
			$this->generateErrorOutput(400, 'device_user', 'PARAMETER_MISSING', $output_format);
		} else if ($status === 401) {
			$this->generateErrorOutput(400, 'device_user', 'INVALID_PARAMETER', $output_format);
		} else if ($status) {
			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'device_user', $results, $output_format);
		} else {
			$this->generateErrorOutput(500, 'device_user', 'DEVICE_USER_UPDATE_FAILED', $output_format);
		}
	}

	function delete($urlPath, $queryParams=null, $output_format='xml') {
		if(!isSet($urlPath[0])){
			$this->generateErrorOutput(400, 'device_user', 'DEVICE_USER_ID_MISSING', $output_format);
		} else {
			$status = $this->removeDeviceUser($urlPath, $queryParams);
			if ($status) {
				$results = array('status' => 'success');
				$this->generateSuccessOutput(200, 'device_user', $results, $output_format);
			} else {
				$this->generateErrorOutput(404, 'device_user', 'DEVICE_USER_NOT_FOUND', $output_format);
			}
		}
	}
}

/*
//test code
$queryParams = array();
//$queryParams['email'] = 'joe1@interwebmail.com';
$queryParams['name'] = 'Joe's iPhone';
$queryParams['type'] = 'iPhone 3Gs';
$queryParams['format'] = 'xml';
$queryParams['device_user_id'] = 115;
$queryParams['device_auth_code'] = 'ikOgm2m0TWgvY2goH3LKKw==';
$queryParams['userId'] = 12345;
$path = array();
$path[0] = 'deviceuser';
$deviceObj = new DeviceUser();
//$deviceObj->put($path, $queryParams);
$deviceObj->get($path, $queryParams);
//$deviceObj->post($path, $queryParams);
$testUser = new DeviceUser();
$testUser->putDeviceUserNotify(42);
*/
?>