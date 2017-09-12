<?php

require_once('userlist.php');
require_once('logMessages.php');
	    
class UsersShell {

	static $os_service;
	static $logObj;

	public function __construct() {
        
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
		
		if (!isset(self::$os_service)) {
			self::$os_service = new Userlist(); 
		}
	}
	
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function createUser($userId, $username, $password, $fullname) {
        
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$username, password=***, fullname=$fullname)");
	    
		$result = self::$os_service->createUser($userId, $username, $password, $fullname);
        return $result;
	}
	
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function updateUser($userId, $username, $fullname, $newPassword, $isAdmin, $changePassword) {

	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$username, fullname=$fullname, newPassword=***, isAdmin=$isAdmin, changePassword=$changePassword)");
	    
		$result = self::$os_service->modifyUser($userId, $username, $fullname, $newPassword, $isAdmin, $changePassword);
        return $result;
	}
	
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function deleteUser($userId, $username) {
        
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$username");
		
	    $result = self::$os_service->deleteUser($userId, $username);
        return $result;
	}

	/**
	 * Calls the supporting system administration script in the OS services layer
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function saveUsersState() {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
	    
		// save the users state in case it needs to be restored

		$output = array();
		$retVal = 0;
		exec("sudo /usr/local/sbin/saveUserShareState.sh", $output, $retVal);
		if($retVal !== 0) {
		    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: saveUserShareState.sh failed with status=$output");
			false;
		}
		return true;
	}
	
	/**
	 * Calls the supporting system administration script in the OS services layer
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function restoreUsersState() {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
	     
	    // restore the users state in case something was modified before the server error
		$output = array();
		$retVal = 0;
		exec("sudo /usr/local/sbin/restoreUserShareState.sh", $output, $retVal);
		if ($retVal !== 0) {
		    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: restoreUserShareState.sh failed with status=$output");
			false;
		}
		return true;
	}
}
?>