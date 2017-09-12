<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class LocalLogin extends RestComponent {

	public function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('usersdb.inc');
		require_once('util.inc');
	}

	public function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$username = isset($queryParams['username']) ? trim($queryParams['username']) : '';
		$password = isset($queryParams['password']) ? trim($queryParams['password']) : '';
		$usersDB  = new UsersDB();

		if (!isLanRequest()) {
			$this->generateErrorOutput(401, 'local_login', 'WAN_LOGIN_NOT_ALLOWED', $output_format);
			return;
		}

		if (!$usersDB->isValid($username)) {
			$this->generateErrorOutput(401, 'local_login', 'INVALID_LOGIN', $output_format);
			return;
		}

		if (!authenticateLocalUser($username, $password)) {
			$this->generateErrorOutput(401, 'local_login', 'UNAUTHENTICATED_LOGIN', $output_format);
			return;
		}

		$results = array('status' => 'success', 'wd2go_server' => getCentralServerHost());
		$this->generateSuccessOutput(200, 'local_login', $results, $output_format);
	}
}
?>