<?php
// Copyright (c) 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Shares extends RestComponent {

	var $logObj;

	public function __construct()
	{
		require_once('authenticate.php');
		require_once('logMessages.php');
		require_once('NasXmlWriter.class.php');
		require_once('share.php');
		require_once('sharelist.php');
		$this->logObj = new LogMessages();
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function get($urlPath, $queryParams=null, $ouputFormat='xml')
	{
		if(!authenticateAsOwner($queryParams)) {
			header('HTTP/1.0 401 Unauthorized');
			return;
		}

		header('Content-Type: application/xml');

		$urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $urlParts[5]);

		// Get specific share:  /shares?owner={owner name}
		if (isset($urlParts[5]) && !(strcmp ( $urlParts[5], '') == 0)){
			$slobj = new Sharelist();
			$testShare = $slobj->getShare($urlParts[5]);
			if($testShare !== null) {
				$xml = new NasXmlWriter();
				$xml->push('share');
				$xml->element('sharedesc', $testShare['sharedesc']);
				$xml->pop();
				echo $xml->getXml();
				$this->logObj->LogData('END', get_class($this), __FUNCTION__, 'SUCCESS');
				return;
			} else {
				// Specific share not found
				header('HTTP/1.0 404 Not Found');
				$this->logObj->LogData('END', get_class($this), __FUNCTION__, 'NOT_FOUND');
				return;
			}
		} else {
			// Get summary of all users:  /users?owner={owner name}&pw={base64 encoded password}
			$ulobj = new Sharelist(); //!!!May want to only consturct one of these
			$result = $ulobj->getShareSummary();
			if ($result !== null){
				$xml = new NasXmlWriter();
				$xml->push('shares');
				foreach($result['share'] as $share){
					$xml->push('share');
					$xml->element('sharename', $share['sharename']);
					$xml->element('sharedesc', $share['description']);
					$xml->element('size', $share['size']);
					$xml->element('remote_access', $share['remote_access']);
					$xml->element('public_access', $share['public_access']);
					$xml->element('media_serving', $share['media_serving']);
					if(count($result['share'][$share['sharename']]['fullaccess_user_arr']) ===0){
						$xml->emptyelement('fullaccess_user_arr');
					} else {
						$xml->push('fullaccess_user_arr');
						foreach($result['share'][$share['sharename']]['fullaccess_user_arr'] as $user){
							$xml->push('user');
							$xml->element('username', $user['username']);
							$xml->element('fullname', $user['fullname']);
							$xml->pop();
						}
						$xml->pop();
					}
					if(count($result['share'][$share['sharename']]['readonly_user_arr']) ===0){
						$xml->emptyelement('readonly_user_arr');
					} else {
						$xml->push('readonly_user_arr');
						foreach($result['share'][$share['sharename']]['readonly_user_arr'] as $user){
							$xml->push('user');
							$xml->element('username', $user['username']);
							$xml->element('fullname', $user['fullname']);
							$xml->pop();
						}
						$xml->pop();
					}

					$xml->pop();
				}
				$xml->pop();
				echo $xml->getXml();
				$this->logObj->LogData('END', get_class($this), __FUNCTION__, 'SUCCESS');
			} else {
				//Failed to collect info
				header('HTTP/1.0 500 Internal Server Error');
				$this->logObj->LogData('END', get_class($this), __FUNCTION__, 'SERVER_ERROR');
			}
		}
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function post($urlPath, $queryParams=null, $ouputFormat='xml')
	{
		//TODO: Need to add lock

		if(!authenticateAsOwner($queryParams)) {
			header('HTTP/1.0 401 Unauthorized');
			return;
		}

		/*
		// save and restore is done at the script level in the event of errors
		// save the share state in case it needs to be restored
		unset($output);
		exec('sudo /usr/local/sbin/saveUserShareState.sh', $output, $retVal);
		if ($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		*/

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $_POST);
		$slobj = new Sharelist();
		$result = $slobj->createShare($_POST);
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);

		switch ($result) {
		case 'SUCCESS':
			header('HTTP/1.0 201 Created');
			break;
		case 'BAD_REQUEST':
			header('HTTP/1.0 400 Bad Request');
			break;
		case 'SERVER_ERROR':
			header('HTTP/1.0 500 Internal Server Error');

		/*
		// save and restore is done at the script level in the event of errors
		// restore the share state in case something was modified before the server error
		unset($output);
		exec('sudo /usr/local/sbin/restoreUserShareState.sh', $output, $retVal);
		*/

			break;
		}
		//TODO: Need to remove lock
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function put($urlPath, $queryParams=null, $ouputFormat='xml')
	{
		//TODO: Need to add lock

		if(!authenticateAsOwner($queryParams)) {
			header('HTTP/1.0 401 Unauthorized');
			return;
		}

		/*
		// save and restore is done at the script level in the event of errors
		// save the share state in case it needs to be restored
		unset($output);
		exec('sudo /usr/local/sbin/saveUserShareState.sh', $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		*/

		$urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));
		if (isset($urlParts[5]) && !(strcmp ( $urlParts[5], '') == 0)){
			$shareName = $urlParts[5];
			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $shareName);
			parse_str(file_get_contents('php://input'), $changes);
			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);
			$ulobj = new Sharelist();
			$result = $ulobj->modifyShare($shareName, $changes);

			switch($result) {
			case 'SUCCESS':
				break;
			case 'BAD_REQUEST':
				header('HTTP/1.0 400 Bad Request');
				break;
			case 'SHARE_NOT_FOUND':
				header('HTTP/1.0 404 Not Found');
				break;
			case 'SERVER_ERROR':
				header('HTTP/1.0 500 Internal Server Error');

				/*
				// save and restore is done at the script level in the event of errors
				// restore the share state in case something was modified before the server error
				unset($output);
				exec('sudo /usr/local/sbin/restoreUserShareState.sh', $output, $retVal);
				if($retVal !== 0) {
					return 'SERVER_ERROR';
				}
				*/

				break;
			}

			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
		} else {
			header('HTTP/1.0 400 Bad Request');
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'BAD_REQUEST');
		}
		//TODO: Need to remove lock
	}


	/**
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $ouputFormat
	 */
	public function delete($urlPath, $queryParams=null, $ouputFormat='xml')
	{
		//TODO: Need to add lock

		if( !authenticateAsOwner($queryParams)) {
			header('HTTP/1.0 401 Unauthorized');
			return;
		}

		/*
		// save and restore is done at the script level in the event of errors
		// save the share state in case it needs to be restored
		unset($output);
		exec('sudo /usr/local/sbin/saveUserShareState.sh', $output, $retVal);
		if ($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		*/

		$urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));
		if (isset($urlParts[5]) && !(strcmp ( $urlParts[5], '') == 0)){
			$shareName = $urlParts[5];
			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $shareName);
			$slobj = new Sharelist();
			$result = $slobj->deleteShare($shareName);

			switch ($result) {
			case 'SUCCESS':
				break;
			case 'SHARE_NOT_FOUND':
				header('HTTP/1.0 404 Not Found');
				break;
			case 'SERVER_ERROR':
				header('HTTP/1.0 500 Internal Server Error');

				/*
				// save and restore is done at the script level in the event of errors
				// restore the share state in case something was modified before the server error
				unset($output);
				exec('sudo /usr/local/sbin/restoreUserShareState.sh', $output, $retVal);
				if($retVal !== 0) {
					return 'SERVER_ERROR';
				}
				*/

				break;
			}
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
		} else {
			header('HTTP/1.0 400 Bad Request');
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'BAD_REQUEST');
		}
		// !!!Need to remove lock
	}


	public function getShares()
	{
		$slobj = new Sharelist();
		$sl = $slobj->getShares();
		return $sl;
	}
}
?>