<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class VerifyRemoteUser extends RestComponent {

	function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		if ($this->verifyUser($urlPath, $queryParams) == true) {
			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'verify_remote_user', $results, $output_format);
		} else {
			$this->generateErrorOutput(500, 'verify_remote_user', 'VERIFY_REMOTE_USER_FAILED', $output_format);
		}
	}


	private function verifyUser($urlPath, $queryParams)
	{
		if (isset($queryParams['email']) && isset($queryParams['uuap']) ) {
			return verifyCentralUserAuthentication($queryParams['email'], $queryParams['uuap']);
		}
		return false;
	}
}
?>