<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class RemoteAccount extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	function post($urlPath, $queryParams, $output_format='xml') {

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'urlPath', print_r($urlPath,true));

		$status = $this->createAccount($urlPath, $queryParams);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', print_r($status,true));

		if ($status === true) {
			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'remote_account', $results, $output_format);
		} else if ($status == 409) {
			$this->generateErrorOutput(409, 'remote_account', 'REMOTE_ACCOUNT_CONFLICT', $output_format);
		} else {
			$this->generateErrorOutput(500, 'remote_account', 'REMOTE_ACCOUNT_FAILED', $output_format);
		}
	}

	private function createAccount($urlPath, $queryParams) {
		$email = $queryParams['email'];
		$password = $queryParams['uuap'];
		return createCentralAccount($email, $password);
	}
}
?>