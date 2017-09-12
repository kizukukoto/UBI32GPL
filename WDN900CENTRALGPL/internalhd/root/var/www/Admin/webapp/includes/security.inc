<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('authenticate.php');
require_once('deviceusersdb.inc');
require_once('globalconfig.inc');
require_once('httpclient.inc');
require_once('shareaccess.php');
require_once('shareaccessdb.php');
require_once('shareaccessshell.php');
require_once('usersdb.inc');
require_once('usersharesdb.php');
require_once('util.inc');

/**
 * Creates a user in the Users table and returns the new userId (primary key).  Does not affect the central servers.
 * The user can be either a Local User (username and password required) or a Central User (a DeviceUser should be added later)
 * Security: Calling function must verify that the user has necessary admin permission.
 * @param String $username Must be non-null for a Local User (a use with an OS account)
 * @param String $password Must be non-null for a Local User (a use with an OS account)
 * @param String $isAdmin
 * @return integer $userId the userId of the newly created account.
 */
function createUser($username, $password, $fullname, $is_admin=0, $chg_pass=1)
{
	$usersDb = new UsersDB();
	$insert_id = $usersDb->createUser($username, $password, $fullname, $is_admin, $chg_pass);
	if($insert_id == -1){
		return false;
	}

	//TODO: Add account to local operating system using low-level calls

	return $insert_id;
}


/**
 * Creates a Central User by calling a service of the Central Servers.  Does not affect any local tables.
 * Result is output in XML format.
 * Security: This is a safe method since it does not provide any secure information.
 * @param $email Primary identifier of the Central User that is to be created
 * @param $password Password
 * @return boolean indicating whether the user was successfully created.
 */
function createCentralAccount($email, $password)
{
	$config = getGlobalConfig('remoteuser');
	$globalConfig = getGlobalConfig('global');
	$accQueryParams = array();

	$accQueryParams['email'] = $email;
	$accQueryParams['password'] = $password;

	$serverUrl = getServerBaseUrl().$config['CREATEACC_RESTURL'];
	if ($serverUrl == null )
	{
		return ('No Server URL!');
	}
	$serverUrl = urlReplaceQueryParams($serverUrl, $accQueryParams);
	if (validUrl($serverUrl) == false)
	{
		return( 'Invalid Server URL: ' . $serverUrl);
	}

	$hc = new HttpClient();
	$response = $hc->get($serverUrl);

	if ($response['status_code'] != 200) {
		if ($response['status_code'] == 409) {
			//return ('User with that e-mail address already exists');
			return (409);
		}
		else {
			return('Server URL call returned failure: '. $response['response_text'] . $serverUrl);
		}
	}

	return true;
}


/**
 * Adds DAC access to the specified user.  Calls central service so that central DB also holds the info.
 * Security: Calling function must verify that the user is admin or is the specified userId.
 * @param int $userId Account to which the DAC will provide access.
 * @return String Returns the DAC that needs to be shown to the user (e.g. in 10 foot UI or Flex UI).  Returns false if it fails.
 */
function addDACAccessToUser($userId, $dacFlag)
{
	// Call server deviceuser service to add central device user with NULL email
	$deviceConfig = getGlobalConfig('device');
	$globalConfig = getGlobalConfig('global');
	$duQueryParams = array();
	$duQueryParams['deviceId'] = getDeviceId();
	$duQueryParams['deviceauth'] = getDeviceAuthCode();
	$duQueryParams['dac'] = $dacFlag;

	// Add device user for this device
	// 'null' user - we need to get a device activation code for this device user
	$serverUrlDeviceUser = getServerBaseUrl().$deviceConfig['ADD_DEVICENULLUSER_RESTURL'];
	$serverUrlDeviceUser = urlReplaceQueryParams($serverUrlDeviceUser, $duQueryParams);

	$hc = new HttpClient();
	$response = $hc->get($serverUrlDeviceUser);
	if($response['status_code'] != 200){
		return false;
	}
	$deviceXml = $response['response_text'];
	//check response
	$deviceUser = simplexml_load_string($deviceXml);
	if (stristr($deviceUser->status, 'success') == false) {
		//log error
		return false;
	}
	$dac = NULL;
	$dacExpiration = NULL;
	//$enableWanAccess = false;
	//if($dacFlag == 1){
	$dac = (string)$deviceUser->dac;
	$dacExpiration = (string)$deviceUser->dac_expiration;
	$enableWanAccess = true;
	//}

	// Add device_user with user_id reference, auth and device user id (from server).
	// email is NULL
	$deviceUserDb = new DeviceUsersDB();
	$deviceUserDb->createDeviceUser($userId, $deviceUser->device_user_id,
					$deviceUser->device_user_auth,'', '', '', false, $enableWanAccess, $dac,$dacExpiration);
	if($dacFlag == 1){
		return array('device_user_id' => $deviceUser->device_user_id,
					'device_user_auth' => $deviceUser->device_user_auth,
					'dac' => $dac, 'dac_expiration' =>$dacExpiration);
	}else{
		return array('device_user_id' => $deviceUser->device_user_id,
					'device_user_auth' => $deviceUser->device_user_auth);
	}
}


/**
 * Adds email access to the specified user.  Calls central service so that central DB also holds the info.
 * Security: Calling function must verify that the user is admin or is the specified userId.
 * @param int $userId Account to which the DAC will provide access.
 * @param String $email email address to which access should be given
 * @return String returns boolean indicating whether the change was successful
 */
function addEmailAccessToUser($userId, $email, $sender = NULL)
{
	// Call server deviceuser service to add central device user with email

	$deviceConfig = getGlobalConfig('device');
	$globalConfig = getGlobalConfig('global');
	// Add device user for this device

	$duQueryParams = array();
	$duQueryParams['deviceId'] = getDeviceId();
	$duQueryParams['deviceauth'] = getDeviceAuthCode();
	$duQueryParams['email'] = $email;
	if (!empty($sender)) {
		$duQueryParams['sender'] = $sender;		
	}
	$serverUrlDeviceUser = getServerBaseUrl().$deviceConfig['ADD_DEVICEUSER_RESTURL'];
	$serverUrlDeviceUser = urlReplaceQueryParams($serverUrlDeviceUser, $duQueryParams);

	$hc = new HttpClient();
	$response = $hc->get($serverUrlDeviceUser);

	$deviceXml = $response['response_text'];

	$deviceUser = @simplexml_load_string($deviceXml); //suppress errors for bad XML
	//check response

	if ( $deviceUser == false || stristr($deviceUser->status, 'success') == false ) {
	 	//log error
	 	return false;
	}

	// Add device_user with user_id reference, auth and device user id (from server).
	$deviceUserDb = new DeviceUsersDB();
	$deviceUserDb->createDeviceUser($userId,
									$deviceUser->device_user_id,
									$deviceUser->device_user_auth,
									$email,
									'',
									'webuser',
									true,
									true);
										
	return array('device_user_id' => $deviceUser->device_user_id,
				'device_user_auth' => $deviceUser->device_user_auth);
}


function updateDeviceUser($deviceUserId, $deviceType, $deviceName, $deviceEmail, $isActive,  $typeName, $application)
{
	$deviceConfig = getGlobalConfig('device');
	$globalConfig = getGlobalConfig('global');
/*
	//check deviceUserAuthCode
	$userId = authenticateCentralUser($deviceUserId, $deviceUserAuthCode);
	if ($userId === false) {
		//failed to authenticate
	 	return false;
	}
*/
	//update DeviceUser table
	$deviceUserDb = new DeviceUsersDB();
	$status =  $deviceUserDb->updateDeviceUser($deviceUserId, $deviceType, $deviceName, $deviceEmail, $isActive, $typeName, $application);
	return $status;
}


/**
 * Returns an array of DeviceUsers which belong to the specified UserId.
 * Only uses the local DeviceUsers table and does not depend on center.
 * Each DeviceUser is an array which contains: device_user_id, user_id, auth, email, and create_date.
 * Security: The calling function must verify that the user has proper authentication to receive this information.
 * @param integer $userId Identifier of the user (primary key of Users table) for which the DeviceUsers are to be retrieved
 * @return Array An Array of DeviceUsers
 */
function getDeviceUsers($userId)
{
	$deviceUsersDb = new DeviceUsersDB();
	$deviceUsers = $deviceUsersDb->getDeviceUsersForUserId($userId);
	return $deviceUsers;
}


/**
 * Returns a DeviceUser which is identified by the provided deviceUserId (primary key of DeviceUsers table).
 * Only uses the local DeviceUsers table and does not depend on center.
 * The DeviceUser is an array which contains: device_user_id, user_id, auth, email, and create_date.
 * Security: The calling function must verify that the user has proper authentication to receive this information.
 * @param integer $userId Identifier of the user (primary key of Users table) for which the DeviceUsers are to be retrieved
 * @return Array An Array which represents a single  DeviceUser as described above
 */
function getDeviceUser($deviceUserId)
{
	$deviceUsersDb = new DeviceUsersDB();
	$deviceUser = $deviceUsersDb->getDeviceUser($deviceUserId);
	return $deviceUser;
}


/**
 * Deletes the specified DeviceUser from both the local DeviceUsers table and centrally by calling a service.
 * @param $deviceUserId The identifier of which DeviceUser is to be deleted (primary key of DeviceUsers table)
 * @return boolean Indicates whether the deletion was successful.
 */
function deleteDeviceUser($deviceUserId, $deviceUserAuthCode)
{
	//todo: delete the information from the Central Servers as well (similar to how addEmailAccessToUser posted the information to the center
	$deviceUsersDb = new DeviceUsersDB();

	$isValid = $deviceUsersDb->isValid($deviceUserId, $deviceUserAuthCode);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'isValid', $isValid);

	if (!$isValid) {
		return false;
	}

	$status = $deviceUsersDb->deleteDeviceUser($deviceUserId, $deviceUserAuthCode);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', $status);
		
	if ($status) {
		// DELETE DEVICE USER FROM CENTRAL SERVER
		$status = deleteDeviceUserFromCS($deviceUserId, $deviceUserAuthCode);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'status', $status);
	}
	return $status;
}


function deleteAllDeviceUser()
{
	//todo: delete the information from the Central Servers as well (similar to how addEmailAccessToUser posted the information to the center
	$deviceUsersDb = new DeviceUsersDB();
	$status = $deviceUsersDb->deleteAllDeviceUser();
	return $status;
}


/**
 * Verifies whether the provided email is the correct authentication for the specified email address.
 * Contacts the Central Servers to determine the result (no info of passwords for Central Users is stored locally)
 * @param $email
 * @param $password
 * @return boolean returns boolean indicating whether the authentication is correct
 */
function verifyCentralUserAuthentication($email, $password)
{
	$deviceConfig = getGlobalConfig('remoteuser');
	$globalConfig = getGlobalConfig('global');
	$deviceQueryParams = array();

	$deviceQueryParams['email'] = $email;
	$deviceQueryParams['password'] = $password;

	$serverUrl = getServerBaseUrl().$deviceConfig['VERIFY_RESTURL'];

	if ($serverUrl == null )
	{
		//log error
		return false;
	}
	$serverUrl = urlReplaceQueryParams($serverUrl, $deviceQueryParams);
	if (validUrl($serverUrl) == false)
	{
		//log error
		return false;
	}

	$hc = new HttpClient();
	$response = $hc->get($serverUrl);

	$responseTxt = $response['response_text'];

	if (!$responseTxt || (stristr($responseTxt, 'success') == false) ) {
		//log error
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Authenticating/Deauthenticating of current session
//-----------------------------------------------------------------------------


/**
 * Clears all session parameters that relate to authentication.
 * @return none
 */
function deauthenticate()
{
	setSessionUserId(-1);
	setSessionLocalUsername(NULL);
	setSessionPassword(NULL);
	setSessionDeviceUserId(-1);
	session_destroy();
	//clear and expire the session cookie - deletes the cookie on Firefox
	//setcookie(session_name(), session_id(), 1, '/');
}


/**
 * Authenticates the provided local credentials and sets the appropriate session parameters.
 * Depends on local Users table and does not contact Central Servers.
 * After authentication it sets the appropriate session parameters.
 * @param String $localUsername
 * @param String $password The password may not be required if the account does not have a password set
 * @return integer UserId Returns UserId of the authenticated user (otherwise false)
 */
function authenticateLocalUser($username, $password)
{
	//todo: verify authentication with OS
	//todo: define error codes

	//temp testing method allows auth to fail
	/*if ($password == 'bad') {
		return false;
	} */

	$usersDb = new UsersDB();
	$userId  = $usersDb->getUserId($username);
	
	$isAuth = false;
	if (!$userId) {
		echo("FAILED TO GET USER_ID <br />");
		return false;
	}
	else if (!isPasswordRequiredForLocalUser($username)) {
		$isAuth = true;
	} else {
        $lines = array();
		exec("awk -F \":\" 'BEGIN{OFS=\"\\n\";}{if ($1 == \"$username\") {print $1,$2}}' /etc/shadow ", $lines, $retVal);
		if ($retVal != 0) {
			//log error
		} else {
			$uname = $lines[0];
			$pwdHash = $lines[1];
			
			if (empty($uname)) {
				//log err
	
			} else {
				//unix authenticate
				$salt = substr($pwdHash, 0, 12);
				unset($challenge);
				$challenge = crypt($password, $salt);
				if ($challenge === $pwdHash) {
					$isAuth = true;
				}
			}
		}
	}
	
	if ($isAuth) {
		setSessionUserId($userId);
		setSessionLocalUsername($username);
		setSessionPassword($password);
		updateLastAccessedTime();
		return $userId;
	}
	
	return false; 
		
	
}

/**
 * Authenticates the provided central user based on the DeviceUserId and the AuthenticationCode.
 * Depends on local DeviceUsers table and does not contact Central Servers.
 * After authentication it sets the appropriate session parameters.
 * @param $deviceUserId Identifier that is stored locally which matches the same identifier stored centrally.
 * @param String $auth AuthenticationCode that is to be verified against the local database.
 * @return integer $userId Returns UserId of the authenticated user (otherwise false)
 */
function authenticateCentralUser($deviceUserId, $auth)
{
	$deviceUsersDb = new DeviceUsersDB();
	$isValid = $deviceUsersDb->isValid($deviceUserId, $auth);

	if(!$isValid) {
		return false;
	}
	$deviceUser = $deviceUsersDb->getDeviceUser($deviceUserId);
	$wanAccessEnabled = $deviceUsersDb->isWanAccessEnabled($deviceUserId);
	if($wanAccessEnabled || isLanRequest()){
		if (!isset($deviceUser)) {
			return false;
		}
		$userId = $deviceUser['user_id'];
		//check we got a result set from the DB and it contained the userId
		if (!isset($userId)) {
			//TO DO - add log message here - DB may be corrupt
			return false;
		}

		setSessionUserId($userId);
		setSessionDeviceUserId($deviceUserId);
		updateLastAccessedTime();
		return $userId;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Setting Session Info
//-----------------------------------------------------------------------------


/**
 * Sets the UserId for the current session (see also getSessionUserId()).
 * @param integer $userId
 * @return none
 */
function setSessionUserId($userId)
{
	if ( empty( $_SESSION['s_userId']) || ($_SESSION['s_userId'] == -1) ) {
		@session_start();
	}
	$_SESSION['s_userId'] = $userId;
}


/**
 * Sets the local username for the current session (see also getSessionLocalUsername()).
 * @param String $localUsername
 * @return none
 */
function setSessionLocalUsername($localUsername)
{
	if ( empty( $_SESSION['s_localUsername'] )) {
		@session_start();
	}
	$_SESSION['s_localUsername'] = $localUsername;
}


/**
 * Sets the local password for the current session (see also getSessionPassword()).
 * This should only be called for local users (the NAS never knows passwords of central users).
 * @param String $password
 * @return none
 */
function setSessionPassword($password)
{
	if ( empty( $_SESSION['s_password'] )) {
		@session_start();
	}
	$_SESSION['s_password'] = $password;
}


/**
 * Sets the DeviceUserId for the current session (see also getSessionDeviceUserId()).
 * This would be used for iPhones or central email-based accounts which access this device.
 * @param integer $deviceUserId
 * @return none
 */
function setSessionDeviceUserId($deviceUserId)
{
	if ( empty( $_SESSION['s_deviceUserId']) || ($_SESSION['s_deviceUserId'] == -1) ) {
		@session_start();
	}
	$_SESSION['s_deviceUserId'] = $deviceUserId;
}

//-----------------------------------------------------------------------------
// Getting Session Info
//-----------------------------------------------------------------------------


/**
 * Returns the current UserId for this session.
 * @return integer UserId
 */
function getSessionUserId()
{
	if ( empty( $_SESSION )) {
		@session_start();
	}
	if(isset($_SESSION['s_userId']))
		return $_SESSION['s_userId'];

	// Get Default demo user id from config file.
	// This is auto login with default user to run flash demo and to run WDTV registartion demo.
	// This code must be deleted before product release.
	//$config = getGlobalConfig('global');
	//$defaultUserId = $config['DEFAULT_USER_ID'];
	//return $defaultUserId;
}


/**
 * Returns the local username for this session
 * @return String localUsername
 */
function getSessionLocalUsername()
{
	if ( empty( $_SESSION )) {
		@session_start();
	}
	return $_SESSION['s_localUsername'];
}


/**
 * Even though this value can change during the session, it is still stored since it cannot be retrieved on demand (only hash is stored).
 * This is needed to be passed to the applet for authentication used by WebDAV drive mounting.
 * @return String The local password that was used to authenticate
 */
function getSessionPassword()
{
	if ( empty( $_SESSION )) {
		@session_start();
	}
	return $_SESSION['s_password'];
}


/**
 * Returns the DeviceUserId for this session.
 * @return integer
 */
function getSessionDeviceUserId()
{
	if ( empty( $_SESSION )) {
		@session_start();
	}
	return $_SESSION['s_deviceUserId'];
}

//-----------------------------------------------------------------------------
//Account Detail Functions
//-----------------------------------------------------------------------------


/**
 * Returns true if user is admin otherwise false.  Result is based on a DB query.
 * @param integer $userId UserId for which the admin status is being requested.
 * @return boolean
 */
function isAdmin($userId)
{
	$userDb = new UsersDB();
	return $userDb->isAdmin($userId);
}


/**
 * Returns true if the specified user has a LocalUsername in the database.
 * @param integer $userId
 * @return boolean
 */
function isLocalUser($userId)
{
	$userDb = new UsersDB();
	$localUsername = $userDb->getLocalUsername($userId);
	return $localUsername!=NULL;
}


/**
 * Returns true if a password required for this local user.
 * @param integer $userId
 * @return boolean
 */
function isPasswordRequiredForLocalUser($username)
{    
    $lines = array();
	exec("awk -F \":\" 'BEGIN{OFS=\"\\n\";}{if ($1 == \"$username\") {print $1,$2}}' /etc/shadow ", $lines, $retVal);
	if ($retVal != 0) {
		//log error
	} else {
		$userName = $lines[0];
		$pwdHash = $lines[1];
		if (empty($userName)) {
			//log err
		} else {
			//check if password present
			if (!empty($pwdHash)) {
				return true;
			}
		}
	}
	return false;
}


/**Retruns true if a user is authenticated, otherwise false.
 *
 */
function isAuthenticated($queryParams, $isAdminRequired)
{
	$userId = getSessionUserId();
	if(isset($userId) && $userId != ''){
		return true;
	}

	// Check for request based authentication.

	// Check for old API for compatability
	// TODO : Merge functions authenticateAsOwner and authenticateLocalUser
	// for OS password check.
	if(isset($queryParams['owner']) && isset($queryParams['pw'])){
		if(!isLanRequest()){
			return false;
		}
		//if(authenticateAsOwner($queryParams)){
			// Set current user/owner as session user.
			$usersDb = new UsersDB();
			$userId  = $usersDb->getUserId(strtolower($queryParams['owner']));
			if($isAdminRequired && !isAdmin($userId)){
				return false;
			}
			setSessionUserId($userId);
			return true;
	    //}
	}

	// Check for user name and password.
	// TODO : Merge functions authenticateAsOwner and authenticateLocalUser
	// for OS password check.

	if(isset($queryParams['auth_username']) && isset($queryParams['auth_password'])){
		if(!isLanRequest()){
			return false;
		}
		if(authenticateLocalUser($queryParams['auth_username'], $queryParams['auth_password'])){
			if($isAdminRequired && !isAdmin(getSessionUserId())){
				return false;
			}
			return true;
		}
	}

	// Check for device_user_id and device_user_auth_code.
	if(isset($queryParams['device_user_id']) && isset($queryParams['device_user_auth_code'])){
		if(authenticateCentralUser($queryParams['device_user_id'], $queryParams['device_user_auth_code'])){
			if($isAdminRequired && !isAdmin(getSessionUserId())){
				return false;
			}
			return true;
		}
	}

	// Check for device_user_id and request_auth_code.
	if(isset($queryParams['device_user_id']) && isset($queryParams['request_auth_code'])){
		$deviceUsersDb = new DeviceUsersDB();
		$deviceUser = $deviceUsersDb->getDeviceUser($queryParams['device_user_id']);
		
		//Generate requestAuthHash code from request.
		//$requestArray = explode("&request_auth_code",$_SERVER['REQUEST_URI']);
		//$requestAuthHashStr = $deviceUser['auth'].urldecode($requestArray[0]);
		//$requestAuthHash = hash('sha256', $requestAuthHashStr);
		$requestArray = explode("&request_auth_code",$_SERVER['QUERY_STRING']);
		//$requestAuthHashStr = $deviceUser['auth'].urldecode($_SERVER['SCRIPT_URL']);
		$requestAuthHashStr = $deviceUser['auth'].$_SERVER['SCRIPT_URL'];
        if(isset($requestArray[0])){
        	$requestAuthHashStr = $requestAuthHashStr.'?'.$requestArray[0];
		}
        $requestAuthHash = hash('sha256', $requestAuthHashStr);
		
		if($queryParams['request_auth_code'] === $requestAuthHash){
			if(authenticateCentralUser($queryParams['device_user_id'], $deviceUser['auth'])){
				if($isAdminRequired && !isAdmin(getSessionUserId())){
					return false;
				}
				return true;
			}
		}
	}
	
	
	return false;
}


/**
 * Returns true if request based authentication.
 */
function isRequestBasedAuthentication($queryParams)
{
	// Check for old API for compatability
	if(isset($queryParams['owner']) && isset($queryParams['pw'])){
			return true;
	}

	// Check for user name and password.
	if(isset($queryParams['auth_username']) && isset($queryParams['auth_password'])){
			return true;
	}

	// Check for device_user_id and device_user_auth_code.
	if(isset($queryParams['device_user_id']) && isset($queryParams['device_user_auth_code'])){
			return true;
	}
	
	// Check for device_user_id and device_user_auth_code.
	if(isset($queryParams['device_user_id']) && isset($queryParams['request_auth_code'])){
			return true;
	}
	
	return false;
}


/**
 * Update last accessed time for authenticated session.
 */
function updateLastAccessedTime()
{
	if(empty( $_SESSION ))
		return;
	$userId = getSessionUserId();
	if(isset($userId) && $userId != ''){
		$_SESSION['last_accessed_time'] = time();
	}
}


/**
 * Returns true if session expired
 */
function isSessionExpired()
{
	$config = getGlobalConfig('global');
	$sessionTimeout = $config['SESSION_TIMEOUT'];
	if(isset($_SESSION['last_accessed_time'])){
		if((time() - $_SESSION['last_accessed_time']) > $sessionTimeout){
			return true;
		}
	}
	return false;
}


function getLocalIpAndMaskFromIfConfig(){
	$localIp = array();

	if(getDeviceType() === '5'){
		$Iface = 'br0';
	}else{	
	exec('netstat -i', $netstat_out);
		if(count($netstat_out) == 0){
			exec('sudo netstat -i', $netstat_out);
		}
		foreach($netstat_out as $key => $entry) {
			if (strncmp( $entry, 'Kernel', 6) == 0) {
				unset($netstat_out[$key]);
				break;
			}
		}
		foreach($netstat_out as $key => $entry) {
			if (strncmp( $entry, 'Iface', 5) == 0) {
				unset($netstat_out[$key]);
				break;
			}
		}
		foreach($netstat_out as $key => $entry) {
			if (strncmp( $entry, 'lo', 2) == 0) {
				unset($netstat_out[$key]);
				break;
			}
		}
		foreach($netstat_out as $key => $entry) {
			if (strncmp( $entry, 'tun', 3) == 0) {
				unset($netstat_out[$key]);
				break;
			}
		}
		
		
		foreach($netstat_out as $key => $entry) {
			list( $Iface, $MTU, $Met, $RXOK, $RXERR, $RXDRP, $RXOVR, $TXOK, $TXERR, $TXDRP, $TXOVR, $Flg) = sscanf( $entry, '%s %s %s %s %s %s %s %s %s %s %s %s');
			break;
		}
	}
	
	
	if (empty($Iface))
		return $localIp;

	exec('ifconfig' , $ifconfigout);
	if(count($ifconfigout) == 0){
		exec('sudo ifconfig ', $ifconfigout);
	}

	foreach($ifconfigout as $key => $entry){
		$eth0 = strstr($entry, $Iface );
		if(!empty($eth0))
			break;
	}

	$entry = explode(' ', trim($ifconfigout[++$key]));

	$localIp = array();
	foreach($entry as $key => $value){
		if(empty($value) || $value === 'inet')
			continue;
		$ip = strstr($value, 'addr:' );
		if($ip != false){
			$ipAddress = substr($value, 5); // Skipping 'addr:'
			$localIp['ip'] = $ipAddress;
			continue;
		}
		$mask = strstr($value, 'Mask:' );
		if($mask != false){
			$mask = substr($value, 5); // Skipping 'addr:'
			$localIp['mask'] = $mask ;
			continue;
		}
	}
	return $localIp;
}


/*
 * Returns true if request is from local IP
 */
function isLanRequest()
{
	$remoteAddr = $_SERVER['REMOTE_ADDR'];
	if($remoteAddr === '127.0.0.1')
		return true;

	$localIpAndMask = getLocalIpAndMaskFromIfConfig();
	if(isset($localIpAndMask['ip'])){
		$localIp = ip2long(trim($localIpAndMask['ip']));
		$remoteIp = ip2long($_SERVER['REMOTE_ADDR']);
		$mask = ip2long(trim($localIpAndMask['mask']));
		if((($localIp ^ $remoteIp) & $mask) == 0){
			return true;
		}
	}
	return false;
}


/**
 * Verifies that the provided path has only legal patterns (e.g. no reference to parent such as '..')
 * @param String $pathStr the path to be verified
 * @return boolean Is the path legal
 */
function isPathLegal($path)
{
	//Paths are not allowed to refer to parent directory
	$pattern = '/^\.\.\/|\/\.\.\//';
	if(preg_match_all($pattern, $path, $out) > 0)
		return false;

	//Piping is allowed per ITR: 46675 Abstract: Some characters are not being allowed in files or directories
	//if(strpos($path, '|') !== false)
	//	return false;

	//Paths are not allowed to includes quotes, backslashes or NULL
	//if($path != addslashes($path))
	//	return false;
		
	return true;
}

function getSafeDatabaseText($dbText)
{
	//need to remove slashes for sqlite
	$dbText = stripslashes($dbText);
	return $dbText;
}


/**
 * Returns value indicates whether the session's currently authenticated user can access the specified share.
 * Previous implementation gives everybody access to public and each user can also access the one share that matches their username.
 * New implementation gives everybody access to public and other shares they have given access to.
 *
 * @param String $shareName The name of the share to be accessed
 * @param boolean $isWriteRequested Is the user trying to modify the share (add/remove/modify)
 * @return boolean Is the access allowed
 */
function isShareAccessible($shareName, $isWriteRequested) {
	// All users have RW access to share named "Public"
	if ($shareName == 'Public') {
		return true;
	}
	$user_id = getSessionUserId();
	//printf("<PRE>Security::isShareAccessible.user_id = [%s]</PRE>\n",$user_id);
	// Check if shareName is the public share
	$userSharesDb = new UserSharesDB();
	$share = $userSharesDb->getShareForName($shareName);
	if( $share[0]['public_access'] == 'true'){ //JS - this test was failing because getShareForName() returns a 2D array, not a 1D array
		if (isLANRequest()) {
			//On the LAN, all users can get access to the public share
			return true;
		} else if (isLocalUser($user_id)) {  //JS - fixed type, was $userId, so this test was failing.
			//On the WAN, only local users can get access to the public share
			return true;
		} else {
			return false;
		}
	}
	$ShareAccessDB = new ShareAccessDB();
	$acls = $ShareAccessDB->getAclForShare($shareName, $user_id);

	//printf("<PRE>%s.%s=[%s]</PRE>\n",__METHOD__, 'shareName', print_r($shareName,true));
	//printf("<PRE>%s.%s=[%s]</PRE>\n",__METHOD__, 'user_id', print_r($user_id,true));
	//printf("<PRE>%s.%s=[%s]</PRE>\n",__METHOD__, 'acls', print_r($acls,true));

	foreach ($acls as $acl) {
		if ($acl['access_level'] === 'RW')
			return true;
		if ($acl['access_level'] === 'RO' && !$isWriteRequested)
			return true;
	}
	return false;
}


/**
 * Calls the service of the Central Servers to resend email to the device user.
 * @param integer $deviceUserId Primary identifier of the Central User
 * @return boolean indicating whether the email was successfully sent.
 */
function resendEmail($deviceUserId, $sender = NULL) {
	$globalConfig   = getGlobalConfig('global');
	$config         = getGlobalConfig('remoteuser');
	$deviceUser     = getDeviceUser($deviceUserId);
	$deviceUserAuth = $deviceUser['auth'];
	$serverUrl      = getServerBaseUrl().$config['RESEND_EMAIL_RESTURL'];

	$accQueryParams = array();
	$accQueryParams['dev_user_id']   = $deviceUserId;
	$accQueryParams['dev_user_auth'] = $deviceUserAuth;
	
	if (!empty($sender)) {
		$accQueryParams['sender'] = $sender;
	}
	
	$serverUrl = getServerBaseUrl().$config['RESEND_EMAIL_RESTURL'];

	if ($serverUrl == null) {
		return false;
	}

	$serverUrl = urlReplaceQueryParams($serverUrl, $accQueryParams);
	$serverUrl = str_replace('/device_user?', '/device_user/'.$deviceUserId.'?', $serverUrl);

	if (validUrl($serverUrl) == false) {
		return false;
	}

	$hc = new HttpClient();
	$response = $hc->get($serverUrl);

	if ($response['status_code'] != 200) {
		return false;
	}
	return true;
}


/**
 * Calls the service of the Central Servers to delete the device user.
 * @param integer $deviceUserId Primary identifier of the Central User
 * @param integer $deviceUserAuthCode
 * @return boolean indicating whether the device user was successfully deleted
 */
function deleteDeviceUserFromCS($deviceUserId, $deviceUserAuth) {
	$globalConfig   = getGlobalConfig('global');
	$config         = getGlobalConfig('device');
	$serverUrl      = getServerBaseUrl().$config['DELETE_DEVICEUSER_RESTURL'];
	$accQueryParams = array();
	$accQueryParams['deviceUserId']   = $deviceUserId;
	$accQueryParams['deviceUserAuth'] = $deviceUserAuth;

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'deviceUserAuth', $deviceUserAuth);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'SERVERURL', $serverUrl);
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'accQueryParams', print_r($accQueryParams,true));

	if ($serverUrl == null) {
		return false;
	}

	$serverUrl = urlReplaceQueryParams($serverUrl, $accQueryParams);

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'serverUrl', $serverUrl);

	if (validUrl($serverUrl) == false) {
		return false;
	}

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'serverUrl', $serverUrl);

	$hc = new HttpClient();
	$response = $hc->get($serverUrl);

	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'response', print_r($response,true));

	if ($response['status_code'] != 200) {
		return false;
	}
	return true;
}


/* Create, update, delete user password for htpasswd of apache (Only for ROUTER)
 * @param string $actionType Action(CREATE, DELETE, UPDATE)
 * @param string $deviceUserId user's account ID
 * @param string $deviceAuthCode user's account password
 * */
function updateApacheHtpassword($actionType, $deviceUserId, $deviceAuthCode=null) {
	$globalConfig = getGlobalConfig('global');
	$deviceConfig = getGlobalConfig('device');		
	$location     = isset($deviceConfig['APACHE_DIGEST_HTPASSWD']) ? $deviceConfig['APACHE_DIGEST_HTPASSWD'] : null;
	$realm        = 'DeviceUser';
		
	// DEVICETYPE 5: router
	$deviceType = isset($globalConfig['DEVICETYPE']) ? $globalConfig['DEVICETYPE'] : null;
	if($deviceType != '5') {
		return false;
	}
	
	if(empty($actionType) 
		|| empty($deviceUserId)
		|| empty($realm) ) {
			return false;	
		}		
	ignore_user_abort(true); // to prevent user abort during saving it
		
	$buffer = file_exists($location) && is_readable($location) ? file_get_contents($location) : null;
			
	if(!empty($buffer)) {
		$buffer = explode(PHP_EOL, $buffer);			
	}
	
	$isExist = false;
	$newline = $deviceUserId . ':' . $realm . ':' .md5($deviceUserId . ':' . $realm . ':' . $deviceAuthCode);
	switch($actionType) {
		case 'CREATE':			
			foreach($buffer as $key => $val) {
				$val = preg_replace('/ /', '', $val);
				if(preg_match("/$deviceUserId/i", $val) || empty($val)) { 
					unset($buffer[$key]);
				}							
			}			
			$buffer[sizeof($buffer)] = $newline;
			break;
		case 'UPDATE':			
			foreach($buffer as $key => $val) {
				$val = preg_replace('/ /', '', $val);
				if(preg_match("/$deviceUserId/i", $val)) {					
					$buffer[$key] = $newline;	
					$isExist = true;
				}
				if(empty($val)) { 
					unset($buffer[$key]); 
				}
			}				
			if(!$isExist) {
				$buffer[sizeof($buffer)] = $newline;
			}
			break;
		case 'DELETE':			
			foreach($buffer as $key => $val) {
				$val = preg_replace('/ /', '', $val);
				if(preg_match("/$deviceUserId/i", $val)) {
					unset($buffer[$key]);
					$isExist = true;
				}
				if(empty($val)) { 
					unset($buffer[$key]); 
				}
			}		
			if(!$isExist) {
				return true;
			}
			break;		
		default:
			return 'WRONG_ACTION';
	}	
	if(file_put_contents($location, implode(PHP_EOL, $buffer), LOCK_EX)) {
		return true;
	}
	return false;
}

?>
