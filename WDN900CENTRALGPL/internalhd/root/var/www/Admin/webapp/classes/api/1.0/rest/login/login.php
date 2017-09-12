<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Login extends RestComponent{

	function __construct()
	{
		require_once('util.inc');
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('deviceusersdb.inc');
		require_once('security.inc');
	}


	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$deviceUserId = $queryParams['device_user_id'];
		$deviceUserAuthCode = $queryParams['device_user_auth_code'];
		$deviceUsersDb = new DeviceUsersDB();
		$isValid = $deviceUsersDb->isValid($deviceUserId,$deviceUserAuthCode);

		if ($isValid ) {
			$wanAccessEnabled = $deviceUsersDb->isWanAccessEnabled($deviceUserId);
			// Check for WAN access
			// If WAN access enabled allow WAN and LAN requests.
			if ($wanAccessEnabled || isLanRequest()) {
				// Get User ID and update http session (php session)
				$deviceUser = $deviceUsersDb->getDeviceUser($deviceUserId);
				$userId = $deviceUser['user_id'];
				setSessionUserId($userId);
				$results = array('status' => 'success', 'user_id' => $userId, 'wd2go_server' => getCentralServerHost());
				$this->generateSuccessOutput(200, 'login', $results, $output_format);
			} else {
				$this->generateErrorOutput(401, 'login', 'WAN_LOGIN_NOT_ALLOWED', $output_format);
			}
		} else {
			$this->generateErrorOutput(401, 'login', 'INVALID_AUTH_CODE', $output_format);
		}
	}
}
?>