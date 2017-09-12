<?php
require_once('globalconfig.inc');
require_once('httpclient.inc');
require_once('util.inc');

/**
 * DeviceControl Class
 * 
 * Singleton Class which provides a Controller for the NAS device
 * 
 * 5/19/11 Modifed prcedural Device API to be a Singleton class that controls the NAS Device
 * 
 * @author sapsford_j
 *
 */

class DeviceControl {
	
	private static $instance;
	
	//make constructor private, so class cannont be instantiated by outside code
	private function __construct()  {
    	
    }
    
    /**
     * get the singleton instance
     */
    public static function getInstance() {
    	if (!isset(self::$instance)) {
    		$thisClass = __CLASS__;
            self::$instance = new $thisClass;
    	}
    	return self::$instance;
    }
	
    /**
     * Get the device name and device description from the O/S 
     * 
     * Result is an associate array with keys: 'machine_name' and 'machine_desc'
     */
	public function getDeviceDescription() {
        if (SKIP_SHELL_SCRIPT)
        {
            $machine_name = php_uname('n');
        }
        else 
        {
    		unset($machine_name);
            exec("hostname ", $machine_name, $retVal);
    	    if($retVal !== 0) {
    	        return NULL;
    	    }
    	    $machine_name = $machine_name[0];
        }


		return( array( 'machine_name' => $machine_name));
	}
	    
	/**
	 * Update the device name
	 * 
	 * @param $name new device Name
	 */
	
	public function updateDeviceName($name)
	{
		$deviceConfig = getGlobalConfig('device');
		$globalConfig = getGlobalConfig('global');
		$deviceQueryParams['name'] = $name;
		$deviceQueryParams['deviceId'] = getDeviceId();
		if(empty($deviceQueryParams['deviceId'])){
			return false;
		}
		$deviceQueryParams['auth'] = getDeviceAuthCode();
		$updateServerUrl = getServerBaseUrl().$deviceConfig['UPDATE_DEVICE_RESTURL'];
		if ($updateServerUrl == null ) {
			//log error
			return false;
		}
		$updateServerUrl = urlReplaceQueryParams($updateServerUrl, $deviceQueryParams);
		if (validUrl($updateServerUrl) == false) {
			//log error
			return false;
		}
		$hc = new HttpClient();
		$response = $hc->get($updateServerUrl);
		if($response['status_code'] != 200 ){
			return false;
		}
		return true;
	}
	
	/**
	 * Posts to a central service to create a new device and stores the resulting deviceId and deviceAuthentication in the local config file.
	 * This will not be of much value unless
	 * @param String $urlPath passed in by the service, but is not currently used
	 * @param Array $queryParams Must contain 'devicename'. An optional queryParam is 'email'
	 * @return boolean Inidicates whether the function was successful.
	 */
	
	public function registerDeviceWithName($deviceName, $email = null) {
	
		$deviceConfig = getGlobalConfig('device');
		$globalConfig = getGlobalConfig('global');
		$dynamicConfig = getUpnpStatus('config');
		
		
		$deviceQueryParams = array();
		$deviceQueryParams['deviceName'] = $deviceName;
		$deviceQueryParams['devicePort'] = $dynamicConfig['INTERNAL_PORT'];
		$deviceQueryParams['device_ssl_port'] = $dynamicConfig['DEVICE_SSL_PORT'];
		if (!empty($email)) {
			$deviceQueryParams['email'] = $email;
		}
		
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
		$serverBaseUrl = $device->{'device'}->{'server_url'};
		$deviceName = $queryParams['device_name'];

		setDeviceRegistrationConfig($deviceId, $deviceAuth, $serverBaseUrl);
		deleteAllDeviceUser();
		//get SSL cert from server
		$this->getDeviceSSLCertificate($device->{'device'}->{'server_domain'}, $deviceId, $deviceAuth);
		//If an email is provided, that email should receive remote access
		if(isset($email)) {
			if (addEmailAccessToUser(getSessionUserId(), $email)) {
				return $device->{'device'};
			}
		} else {
			return $device->{'device'};
		}
	}
	
	
	/**
	 * Registers the device - this function gets the device name from the O/S and then calls 
	 * registerDeviceWithName to perform the registration
	 */
	
	public function registerDevice() {
		$deviceDescription = $this->getDeviceDescription();
		if ($deviceDescription == null) {
			return false;
		}
		return $this->registerDeviceWithName($deviceDescription['machine_name']);
	}
	
	public function getDeviceSSLCertificate($serverDomain, $deviceId, $deviceAuthCode) {
		
		$sslcert = new SslCertificate();
		$signed_cert = $sslcert->getSignedCert($serverDomain,$deviceId, $deviceAuthCode);
		$status = true;
	
		if( !empty($signed_cert) ){
			// Create cert file.
			$sslConfig = getGlobalConfig('openssl');
			$crtFileName = $sslConfig['CERT_PATH'].'server.crt';
			$fp = fopen($crtFileName, 'w');
			if ($fp) {
				fwrite($fp, $signed_cert);
				fclose($fp);
				// reload apache configuration
	        	$ulObj = new Userlist();
	        	$ulObj->reloadApacheConfig();
			}
			else {
				$status = false;
			}
		}
		else {
			$status = false;
		}
		return $status;
	}

}

/**
 * Call to count device number.
 * @return intthe number of device users.
 */
function getDeviceCount() {
	$deviceUsersDB = new DeviceUsersDB();
	return $deviceUsersDB->getNumberOfDeviceUsers();
}

?>