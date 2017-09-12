<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Logout extends RestComponent{

	function __construct() {
		require_once('outputwriter.inc');
		require_once('security.inc');
	}
	
	function get($urlPath, $queryParams=null, $output_format='xml') {
		deauthenticate();
		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'logout', $results, $output_format);

		/*
		$deviceUserId = $queryParams['device_user_id'];
		$username = $queryParams['username'];
		//check we have a session for user
		if ((isset($username) && strcmp($_SESSION['s_localUsername'], $username) == 0) ||
			(isset($deviceUserId) && strcmp($_SESSION['s_deviceUserId'], $deviceUserId) == 0 )) {
			deauthenticate();
			$elmValues = array('status'=>'Success');
			$this->generateSuccessOutput(200, 'logout', $elmValues, $output_format);
		} else {
			$this->generateErrorOutput(401, 'logout', "User is not loggedin.", $output_format);
		}
		*/
	}
}
?>