<?php
// Copyright (c) 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class Device extends RestComponent {

	function __construct() {
		require_once('device.inc');
		require_once('deviceusersdb.inc');
		require_once('globalconfig.inc');
		require_once('httpclient.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('sslcertificate.inc');
		require_once('util.inc');
		require_once('userlist.php');
	}

	/**
	 * Posts to a central service to create a new device and stores the resulting deviceId and deviceAuthentication in the local config file.
	 * This will not be of much value unless
	 * @param String $urlPath passed in by the service, but is not currently used
	 * @param Array $queryParams Must contain 'devicename'. An optional queryParam is 'email'
	 * @return boolean Inidicates whether the function was successful.
	 */
	private function registerDevice($urlPath, $queryParams) {
		//TODO: test this method with no email provided (this will be the case for WDTV)
		$deviceConfig = getGlobalConfig('device');
		$globalConfig = getGlobalConfig('global');
		$upnpConfig = getUpnpStatus('config');
		$deviceQueryParams = array();
		if(isset($queryParams['email'])){
			$email = $queryParams['email'];
			$deviceQueryParams['email'] = $email;
		}
		$deviceQueryParams['deviceName'] = $queryParams['name'];
		$deviceQueryParams['devicePort'] = $upnpConfig['INTERNAL_PORT'];
		$deviceQueryParams['device_ssl_port'] = $upnpConfig['DEVICE_SSL_PORT'];
		$serialNum = '';
		$serialNumScript = $deviceConfig['SERIAL_NUM_SCRIPT'];
		if (isset($serialNumScript) && !empty($serialNumScript)) {
			$serialNumScript = str_replace('%DQ%','"', $serialNumScript);
			exec($serialNumScript, $serialNumArr);
			if (isset($serialNumArr[0]) ){
				$serialNum = $serialNumArr[0];
			}
		}
		$deviceQueryParams['serial_no'] = $serialNum;
		$deviceQueryParams['type'] = $globalConfig['TYPE'];
		if (isset($queryParams['email'])) {
			$serverUrl = getServerBaseUrl().$deviceConfig['ADD_DEVICE_RESTURL'];
		} else {
			$serverUrl = getServerBaseUrl().$deviceConfig['ADD_DEVICE_NOEMAIL_RESTURL'];
		}
		if ($serverUrl == null ) {
			//log error
			return false;
		}
		$serverUrl = urlReplaceQueryParams($serverUrl, $deviceQueryParams);
		if (validUrl($serverUrl) == false) {
			//log error
			return false;
		}
		$hc = new HttpClient();
		$response = $hc->get($serverUrl);
		if($response['status_code'] != 200 ){
			return false;
		}
		$device = json_decode($response['response_text']);
		$status = $device->{'device'}->{'status'};
		$deviceId = $device->{'device'}->{'device_id'};
		$deviceAuth = $device->{'device'}->{'device_auth'};
		$deviceName = $queryParams['device_name'];
		setDeviceId($deviceId);
		setDeviceAuthCode($deviceAuth);
		deleteAllDeviceUser();
		//If an email is provided, that email should receive remote access
		if(isset($email)) {
			if (addEmailAccessToUser(getSessionUserId(), $email)) {
				return $device->{'device'};
			}
		} else {
			return $device->{'device'};
		}
	}

	private function updateDevice($urlPath, $queryParams) {
		return DeviceControl::getInstance()->updateDeviceName($queryParams['name']);
	}

	function get($urlPath, $queryParams=null, $output_format='xml') {
		$deviceId     = getDeviceId();
		$deviceType   = getDeviceType();
		$commStatus   = getDeviceCount() <= 0 ? 'disabled': getCommunicationStatus();
		$remoteAccess = getRemoteAccess();
		$ipsAndPorts  = getIPAddresesAndPorts();
		$defaultPortsOnly = getDefaultPortsOnly();

		$remoteAccess = strtolower($remoteAccess);
		$defaultPortsOnly = strtolower($defaultPortsOnly);

		$results = array(
			'device_id' => $deviceId,
			'device_type' => $deviceType,
			'communication_status' => $commStatus,
			'remote_access' => $remoteAccess,
			'local_ip' => $ipsAndPorts['INTERNAL_IP'],
			'default_ports_only' => $defaultPortsOnly
		);
		$this->generateSuccessOutput(200, 'device', $results, $output_format);
	}


	function post($urlPath, $queryParams=null, $output_format='xml') {

		$name = isset($queryParams['name']) ? trim($queryParams['name']) : null;

		if (empty($name)) {
			$this->generateErrorOutput(400, 'device', 'DEVICE_NAME_MISSING', $output_format);
			return;
		}

		if (!preg_match('/^[a-zA-Z0-9-]+$/i', $name) || strlen($name) > 63 || substr($name,0,1) == '-' || substr($name,-1) == '-' || strpos($name, '--') !== false) {
			$this->generateErrorOutput(400, 'device', 'DEVICE_NAME_INVALID', $output_format);
			return;
		}

		$status = $this->registerDevice($urlPath, $queryParams);
		if (!$status) {
			$this->generateErrorOutput(500, 'device', 'DEVICE_REG_FAILED', $output_format);
			return;
		}

		//Get SSL Certificate from Server
		$sslcert = new SslCertificate();
		$signed_cert = $sslcert->getSignedCert($status->{'server_domain'},getDeviceId(), getDeviceAuthCode());

		if(!empty($signed_cert)){
				// Create cert file.
				$sslConfig = getGlobalConfig('openssl');
				$crtFileName = $sslConfig['CERT_PATH'].'server.crt';
				$fp = fopen($crtFileName, 'w');
				fwrite($fp, $signed_cert);
				fclose($fp);
				// reload apache configuration
				$ulObj = new Userlist();
				$ulObj->reloadApacheConfig();
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(201, 'device', $results, $output_format);
	}


	function put($urlPath, $queryParams=null, $output_format='xml') {

		$name          = isset($queryParams['name'])          ? trim($queryParams['name'])          : null;
		$remote_access = isset($queryParams['remote_access']) ? trim($queryParams['remote_access']) : null;
		$default_ports_only = isset($queryParams['default_ports_only']) ? trim($queryParams['default_ports_only']) : null;

		if (empty($name) && empty($remote_access)&& empty($default_ports_only)) {
			$this->generateErrorOutput(400, 'device', 'PARAMETER_MISSING', $output_format);
			return;
		}

		if (!empty($name)) {
			if (!preg_match('/^[a-zA-Z0-9-]+$/i', $name) || strlen($name) > 63 || substr($name,0,1) == '-' || substr($name,-1) == '-' || strpos($name, '--') !== false) {
				$this->generateErrorOutput(400, 'device', 'DEVICE_NAME_INVALID', $output_format);
				return;
			}
			$deviceId = getDeviceId();
			if (empty($deviceId)) {
				$this->generateErrorOutput(403, 'device', 'DEVICE_NOT_REGISTERED', $output_format);
				return;
			}
			$status = $this->updateDevice($urlPath, $queryParams);
			if ($status === false) {
				$this->generateErrorOutput(500, 'device', 'DEVICE_UPDATE_FAILED', $output_format);
				return;
			}
		}

		if (!empty($remote_access)) {
			$remoteAccess = $queryParams['remote_access'];
			$status = setRemoteAccess($remoteAccess);
			if ($status === false) {
				$this->generateErrorOutput(500, 'device', 'DEVICE_UPDATE_FAILED', $output_format);
				return;
			}
		}

		if (!empty($default_ports_only)) {
			$status = setDefaultPortsOnly($default_ports_only);
			if ($status === false) {
				$this->generateErrorOutput(500, 'device', 'DEVICE_UPDATE_FAILED', $output_format);
				return;
			}
		}

		//start or stop orion remote access services depending on remote access setting
		$status  = updateRemoteServices();
		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'device', $results, $output_format);
	}
}

/*
//test code
$queryParams = array();
$queryParams["email"] = "joe1@interwebmail.com";
$queryParams["devicename"] = "nas1";
$queryParams["uuap"] = "unGuess1blystr0ngp4sswOrd";
$path = array();
$path[0] = "device";
$deviceObj = new Device();
$deviceObj->get($path);
*/
?>