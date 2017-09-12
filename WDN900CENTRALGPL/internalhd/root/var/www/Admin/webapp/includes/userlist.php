<?php

require_once('lock.php');
require_once('logMessages.php');
require_once('user.php');

class Userlist {

	static $logObj;
	static $lockObj;

	public function __construct()
	{
		if (!isset(self::$lockObj)) {
			self::$lockObj = new Lock("Userlist_lock");
		}
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
	}


	function getSharesArray($type)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (type=$type)");

		$sharesArray = array();

		exec("sudo /usr/local/sbin/getShares.sh $type", $output, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		//Get description of all shares
		foreach($output as $shareName) {
			unset($outputDescription);
			exec("sudo /usr/local/sbin/getShareDescription.sh $shareName", $outputDescription, $retVal);
			if($retVal !== 0) {
				return NULL;
			}
			$shareDescription = $outputDescription[0];

			//Build array of shareName => shareDescription
			$sharesArray["$shareName"] = "$shareDescription";
		}
		return $sharesArray;
	}


	function getUserSummary()
	{
		 self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");

//		$userArray = $this->getUsers();
//		if($userArray === NULL) {
//			return NULL;
//		}

		exec("sudo /usr/local/sbin/getUsers.sh", $outputUsers, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		$userArray = $outputUsers;

		$shareArray = $this->getSharesArray('private');
		if($shareArray === NULL) {
			return NULL;
		}

		exec("sudo /usr/local/sbin/getShares.sh private", $output, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		$shareArray = $output;

		$user = array('user' => array());

		foreach  ($userArray as $testUser)
		{
			unset($outputFullName);
			exec("sudo /usr/local/sbin/getUserInfo.sh \"$testUser\"", $outputFullName, $retVal);
			if($retVal !== 0) {
				return NULL;
			}
			$fullName = $outputFullName[0];

			$user['user']["$testUser"] = array('username' => "$testUser", 'fullname' => "$fullName", 'share' => array());
		}

		//For each private share
		foreach($shareArray as $share){

			unset($outputDescription);
			exec("sudo /usr/local/sbin/getShareDescription.sh \"$share\"", $outputDescription, $retVal);
			if($retVal !== 0) {
				return NULL;
			}
			$shareDescription = $outputDescription[0];

			$accessType = array('RW', 'RO');
			foreach($accessType as $access){
				unset($output);
				//Get ACL
				exec("sudo /usr/local/sbin/getAcl.sh $share $access", $output, $retVal);
				if($retVal !== 0) {
					return NULL;
				}
				// foreach user returned --NEED to Verify user exists and log it if it doesn't
				foreach($output as $u){

					array_push($user['user']["$u"]['share'], array('sharename' => "$share",
							'sharedesc' => "$shareDescription", 'access' => $access));
				}
			}
		}
		return $user;
	}


	/**
	 * Gets all user IDs from the OS services layer (currently uses the 'fullname' field)
	 *
	 * @param string $userName the identification index for the user
	 * @return ArrayObject User($userName, $userId)
	 */
	function getUsers()
	{
		// self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");

		$usersArray = array();

		unset($outputUsers);
		exec("sudo /usr/local/sbin/getUsers.sh", $outputUsers, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		foreach($outputUsers as $userName) {
			unset($output);
			exec("sudo /usr/local/sbin/getUserInfo.sh $userName 'userid'", $output, $retVal);
			if($retVal !== 0) {
				return NULL;
			}
			$userId = $output[0];

			unset($output);
			exec("sudo /usr/local/sbin/getUserInfo.sh $userName 'fullname'", $output, $retVal);
			if($retVal !== 0) {
				return NULL;
			}
			$fullname = $output[0];

			array_push($usersArray, array( 'username' => $userName, 'fullname' => $fullname, 'userid' => $userId));
		}
		return $usersArray;
	}


	/**
	 * Gets the userId from the OS services layer (currently uses the 'fullname' field)
	 *
	 * @param string $userName the identification index for the user
	 * @return ArrayObject ('fullname' => $fullName)
	 */
	function getUser($userName)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userName=$userName)");

		unset($outputFullName);
		exec("sudo /usr/local/sbin/getUserInfo.sh \"$userName\" 'userid'", $outputFullName, $retVal);
		if($retVal !== 0) {
			return NULL;
		}
		$fullName = $outputFullName[0];

		return array('fullname' => $fullName);
	}


	function isValidUserName($userName)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userName=$userName)");

		//For now let UI do all validation
		return TRUE;

		if( (strlen($userName) > 32) ||
			(preg_match("/^[a-z0-9][a-z0-9_]*$/i", $userName) == 0) ) {
			return FALSE;
		} else {
			return TRUE;
		}
	}


	function isValidFullName($fullname)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (fullname=$fullname)");

		//For now let UI do all validation
		return TRUE;

		if($fullname === ''){
			return TRUE;
		}

		if( (strlen($fullname) > 256) ||
			(preg_match("/^[a-z0-9]/i", $fullname) == 0) ) {
			return FALSE;
		} else {
			return TRUE;
		}
	}


	function isValidBool($change_passwd)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (change_passwd=$change_passwd)");

		if($change_passwd === 'true' || $change_passwd === 'false'){
			return TRUE;
		} else {
			return FALSE;
		}
	}

	function isValidPassword($userpasswd)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userpasswd=$userpasswd)");

		//For now let UI do all validation
		return TRUE;

		if( (strlen($userpasswd) > 16) ||
			(strpos($userpasswd, '"') !== FALSE) ){
			return FALSE;
		} else {
			return TRUE;
		}
	}

	function isExistingUser($userName)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userName=$userName)");
        $ulist = array();

        $ul = new Userlist();
    	$ulist = $ul->getUsers();

    	//var_dump($ulist);

    	if($ulist === NULL) {
    		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "WARN: getUsers() call failed");
    	}

    	for( $i = 0; $i<sizeof($ulist); $i++ ) {

    	    //var_dump($ulist[$i]['username']);

   		    // username
  		    $username = $ulist[$i]['username'];

			if($username === $userName) {
				return true;
			}
		}
		return false;
	}

	function createApacheUser($userName, $userId, $password=null)
	{
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$userName, password=***)");

		// create apache user
		unset($output);
		if ($password != null && $password != '') {
    		exec("sudo /usr/local/sbin/addUser_apache.sh $userId $userName $password", $output, $retVal );
		}
    	else {
    	    exec("sudo /usr/local/sbin/addUser_apache.sh $userId $userName", $output, $retVal );
    	}
		if($retVal !== 0) {
        	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: addUser_apache.sh call failed");
        	//return false;
		}

/*
 * these system scripts are now called from modAcl()
 *
		// Update apache configuration files
		// regenerate the apache group authorizations file
		exec("sudo /usr/local/sbin/genApacheGroupsFile.sh", $output, $retVal);
	    if($retVal !== 0) {
        	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: genApacheGroupsFile.sh call failed");
        	return false;
        }
        // regenerate the apache group 'require' directives file
        exec("sudo /usr/local/sbin/genApacheAccessRules.sh", $output, $retVal);
	    if($retVal !== 0) {
        	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: genApacheAccessRules.sh call failed");
        	return false;
        }
*/
		return true;
	}

	function deleteApacheUser($action, $userName, $userId=null)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (action=$action, userId=$userId, username=$userName)");

		// delete the apache user
        unset ($output);
        exec("sudo /usr/local/sbin/deleteUser_apache.sh $action $userName $userId", $output, $retVal );
        if($retVal !== 0) {
          	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: deleteUser_apache.sh call failed, $output, $retVal");
        	//return false;
        }
/*
 * these system scripts are now called from modAcl()
 *
		// Update apache configuration files
		// regenerate the apache group authorizations file
		exec("sudo /usr/local/sbin/genApacheGroupsFile.sh", $output, $retVal);
	    if($retVal !== 0) {
        	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: genApacheGroupsFile.sh call failed");
        	return false;
        }
        // regenerate the apache group 'require' directives file
        exec("sudo /usr/local/sbin/genApacheAccessRules.sh", $output, $retVal);
	    if($retVal !== 0) {
        	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: genApacheAccessRules.sh call failed");
        	return false;
        }
*/
		return true;
	}

	/**
	 * Handles the shell script executions required for this OS service
	 *
	 * @param integer $userId the identification index for the new user
	 * @param string $userName holds the new user name
	 * @param string $password holds the new user password
	 * @param string $fullname holds the new full name of the user (not used for Orion)
	 * @return boolean $status indicates the success or failure of this operation
	 */
	function createUser($userId, $userName, $password, $fullname)
	{
		 self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$userName, password=***, fullname=$fullname)");

        $apachePwd = "no";
		$decodedPw = '';

        //Require representation of user
		if( !isset($userId)	   ||
			!isset($userName)	  ||
			!isset($password) ){
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: bad input parameters");
				return false;
		}
		//Verify values are valid
		if(!$this->isValidUserName($userName)  ||
		   !$this->isValidPassword($password) ){
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: $userName or $password is invalid");
			return false;
		}

		// let fontend DB verify username..
		//Don't allow creating user with same username.
		//if($this->isExistingUser($userName)){
		//	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: username $userName already exists.");
		//	return false;
		//}

		if ($password !== '' && $password !== null) {
    		$decodedPw = base64_decode($password, false);
	    	if($decodedPw === false){
		    	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: bad password decode");
			    return false;
		    }
		}
		self::$logObj->LogData('tempdebug', get_class($this),  __FUNCTION__,  "decodedPw=$decodedPw");
		// lock all operations which can change common resources
		self::$lockObj->Get();

		//Actually create user

		$restoreList = array();
		do{

			unset($output);
			exec("sudo /usr/local/sbin/addUser.sh $userName $userId $fullname", $output, $retVal);
			if($retVal !== 0) {
				break;
			}
			array_push($restoreList,"sudo /usr/local/sbin/deleteUser.sh $userName");

			if($decodedPw !== '' ) {
				exec("sudo /usr/local/sbin/modUserPassword.sh $userName \"$decodedPw\"", $output, $retVal);
				if($retVal !== 0) {
					break;
				}
				self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "userName=$userName, userId=$userId, decodedPw=$decodedPw");
                if ( ! $this->createApacheUser($userName, $userId, $decodedPw)) {
                    break;
                }
			}
			else {
			    exec("sudo /usr/local/sbin/modUserPassword.sh $userName", $output, $retVal);
				if($retVal !== 0) {
				    self::$logObj->LogData('ERROR', get_class($this),  __FUNCTION__,  "call failed: modUserPassword.sh ($userName)");
					// break;
				}
				self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "userName=$userName, userId=$userId");
			    if ( ! $this->createApacheUser($userName, $userId)) {
			        self::$logObj->LogData('ERROR', get_class($this),  __FUNCTION__,  "call failed: createApacheUser($userName, $userId)");
                    break;
                }
			}

			// Release lock
			self::$lockObj->Release();
			return true;

		} while(false);

		//Clean up the user list if the addUser or modUser operations failed
		foreach($restoreList as $restoreItem){
			exec("$restoreItem");
		}

		// Release lock
		self::$lockObj->Release();
		return false;
	}

	/**
	 * Handles the shell script executions required for this OS service
	 *
	 * @param integer $userId the identification index for the new user
	 * @param string $username holds the new user name
	 * @param string $password holds the new user password
	 * @param string $fullname holds the new full name of the user (not used for Orion)
	 * @return boolean $status indicates the success or failure of this operation
	 */
	function modifyUser($userId, $username, $fullname, $newPassword, $isAdmin, $changePassword)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$username, fullname=$fullname, newPassword=***, isAdmin=$isAdmin, changePassword=$changePassword)");

		if  (!isset($userId) || !isset($username)) {
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: bad user parameters");
			return false;
		}
		// Verify the username format is valid
		if(!$this->isValidUserName($username)) {
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: username is invalid");
			return false;
		}

		// lock all operations which can change common resources
		self::$lockObj->Get();

		$restoreList = array();
		do{
			// Get username based on DeviceUserId
		    unset($userNameOut);
			exec("sudo /usr/local/sbin/getUserNameFromId.sh $userId", $userNameOut, $retVal);
			$existingUser=$userNameOut[0];
			//self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "existinguser= $existingUser newuser= $username");
		    if ($existingUser != '' && $existingUser != $username) {
		    	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "rename user $existingUser to $username");
		    	// do apache username change first so that 'oldname' can still be obtained from /etc/passwd file
				$this->deleteApacheUser('change_name', $username, $userId);
		    	unset($ownerName);
				exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
				if($retVal != 0) {
					break;
				}
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (ownerName=$ownerName)");
				// Call modUserName.sh or changeOwner.sh base on user type
				if($ownerName[0] == $existingUser) {
					unset($output);
					exec("sudo /usr/local/sbin/changeOwner.sh $username", $output, $retVal);
					if($retVal !== 0) {
						break;
					}
					array_push($restoreList, "sudo /usr/local/sbin/changeOwner.sh $existingUser");
				} else {
					exec("sudo /usr/local/sbin/modUserName.sh $existingUser $username", $output, $retVal);
					if($retVal !== 0) {
						break;
					}
					array_push($restoreList, "sudo /usr/local/sbin/modUserName.sh $username $existingUser");
				}
		    }
  			// Check for a password change
			else if ( $changePassword === true ) {
				$decodedPw = '';
				if ($newPassword != '' && $newPassword != null) {
		            $decodedPw = base64_decode($newPassword, false);
    	            if($decodedPw == false){
	    	            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: bad password decode");
	                }
			    }
				// Propgate password changes to etc/shadow and apache authorization files
    			unset($output);
    			if($decodedPw == '' ) {
    			    exec("sudo /usr/local/sbin/modUserPassword.sh $username", $output, $retVal);
    				if($retVal !== 0) {
    				    self::$logObj->LogData('ERROR', get_class($this),  __FUNCTION__,  "call failed: modUserPassword.sh ($username, $decodedPw, $retVal, $output[0])");
    					//break;
    				}
    				$this->deleteApacheUser('delete_pwd', $username, $userId);
    			}
    			else {
    				exec("sudo /usr/local/sbin/modUserPassword.sh $username \"$decodedPw\"", $output, $retVal);
    				if($retVal !== 0) {
    				    self::$logObj->LogData('ERROR', get_class($this),  __FUNCTION__,  "call failed: modUserPassword.sh ($username, $decodedPw, $retVal, $output[0])");
    					//break;
    				}
    				// change apache user password in htdigest file
    			    $this->deleteApacheUser('delete_pwd', $username, $userId );
    			    $this->createApacheUser($username, $userId, $decodedPw);
    			}
		    }
		    else {
		        // check for fullname change
    		    $fullNameOut = array();
                exec("sudo /usr/local/sbin/getUserInfo.sh $username 'fullname' ", $fullNameOut, $retVal);
                if($retVal !== 0) {
    				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: fullname not found for userId = $userId");
    				break;
                }
                $currentFullName = $fullNameOut[0];
    		    if ($currentFullName != $fullname)
    		    {
    	            exec("sudo /usr/local/sbin/modUserPassword.sh $username \"$fullname\" '--fullname'", $output, $retVal);
    				if($retVal !== 0) {
    				    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: modUserPassword.sh failed for user = $username");
    					break;
    				}
    		    }
		    }
			// Release lock
			self::$lockObj->Release();
			return true;

		} while(false);

		//Clean up
		foreach($restoreList as $restoreItem){
			exec("$restoreItem");
		}

		// Release lock
		self::$lockObj->Release();
		return false;
	}


	/**
	 * Handles the shell script executions required for this OS service
	 *
	 * @param integer $userId the identification index for the user
	 * @return boolean $status indicates the success or failure of this operation
	 */
	function deleteUser($userId, $userName)
	{
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, userName=$userName)");

		// lock all operations which can change common resources
		self::$lockObj->Get();

		while (true) {

			//Don't allow deleting owner
			exec("sudo /usr/local/sbin/getOwner.sh", $output, $retVal);
			if($retVal !== 0) {
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: getOwner.sh call failed");
				break;
			}
			if($output[0] === $userName){
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "BAD_REQUEST: trying to delete owner");
				break;
			}

			//// the 'deleteUser.sh' scripts checks existence of the user
			//Make sure user exists
			//exec("sudo /usr/local/sbin/getUsers.sh", $outputUsers, $retVal);
			//if($retVal !== 0) {
			//	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: getUsers.sh call failed");
			//	break;
			//}
			//if(!in_array($userName, $outputUsers)){
			//	self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "WARNING: can't delete non-existent user $userName");
			//	break;
			//}

			// delete the user
			unset ($output);
			exec("sudo /usr/local/sbin/deleteUser.sh $userName", $output, $retVal);
			if($retVal !== 0) {
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: deleteUser.sh call failed");
			}
		    $this->deleteApacheUser('delete_os_user', $userName, $userId);

			// Release lock
			self::$lockObj->Release();
			return true;
		}

		// Release lock
		self::$lockObj->Release();
		return false;
	}

	/* Reloading Apache configuration after change any configuration setting
	 * @param $deviceType device type of hardware (stringray : 5)
	 * */
    function reloadApacheConfig($deviceType=null) {
    	if($deviceType == STINGRAY_DEVICE_TYPE) {
    		exec("/internalhd/root/usr/bin/apache2 -k graceful &", $output, $retVal);
    		exec("/internalhd/root/usr/bin/apache2 -k graceful -f /var/etc/apache2/apache2_https.conf &", $output, $retVal);
    	} else {
      		exec("sudo /usr/sbin/apache2ctl -k graceful &", $output, $retVal);
    	}
    	return true;
    }

}

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
