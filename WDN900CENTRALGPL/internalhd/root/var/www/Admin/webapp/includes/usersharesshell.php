<?php

require_once('sharelist.php');
require_once('logMessages.php');
    
class UserSharesShell {
    
    static $logObj;
    static $os_shares;
    static $os_shareAccess;
    static $userSharesDB;
	
	public function __construct() {
  
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
		if (!isset(self::$os_shares)) {
			self::$os_shares = new Sharelist();
		}
        if (!isset(self::$os_shareAccess)) {
            self::$os_shareAccess = new Acllist();
        }
        if (!isset(self::$userSharesDB)) {
            self::$userSharesDB = new UserSharesDB(); 
        }
	}

	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	private function isValidShareName($shareName) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		$result = self::$os_shares->isValidShareName($sharename);
		return $result;
	}

	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	private function isValidDescription($shareDescription) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareDescription=$shareDescription)");
	    
        $result = self::$os_shares->isValidDescription($shareDescription);
		return $result;
	}	

	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function shareExists($shareName) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		$result = self::$os_shares->shareExists($shareName);
		return $result;	
	}

	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function createShare($shareName, $shareDescription) {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, shareDescription=$shareDescription)");
        
		$result = self::$os_shares->createShare($shareName, $shareDescription);
        return $result;
    }
    
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function updateShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess) {

        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, newShareName=$newShareName, newDescription=$newDescription, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess)");
        
        $result = true;

        // If the share being updated belongs to a dynamic volume, make a call that will persist
        // the updated share settingss so they will be used every time the dynamic volume is
        // connected.  It will also change the share name (if a new name was given), so set it to
        // empty so that the modify share script will not try to change it again.

        $shareList = self::$userSharesDB->getShareForName($shareName);
        if (strcasecmp($shareList[0]['dynamic_volume'],'true') == 0) {
            $result = self::$os_shares->modifyDynamicShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess);
            if (!empty($newShareName)) {
                $shareName = $newShareName;
            }
            $newShareName = '';
        }
        
        // Change the share name and/or description if they have changed.

		if ( (strcasecmp($shareName, 'public') != 0) && (!empty($newShareName) || $newDescription !== null)) {
            $result = self::$os_shares->modifyShare($shareName, $newShareName, $newDescription);
            if (!empty($newShareName)) {
                $shareName = $newShareName;
            }
        }

        // If public access, media serving, or remote access has changed, update the share with those changes.

        if (($result === true) && (!empty($publicAccess) || !empty($mediaServing) || !empty($remoteAccess))) {
            $result = self::$os_shareAccess->modifyAcl($shareName, $remoteAccess, $publicAccess, $mediaServing);
        }

        return $result;			
    }

	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function deleteShare($shareName) {
        
      self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");

      $result = self::$os_shares->deleteShare($shareName);
      return $result;
    }
    
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function deleteAllShares() {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");

        $result = self::$os_shares->deleteAllShares($shareName);
        return $result;
    }
    
	/**
	 * Calls the supporting method in the OS services class
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function getShareSize($shareName) {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
		// output in MBs
		unset($output);
		exec("sudo cat /var/local/nas_file_tally/\"$shareName\"/total_size_bytes | awk '{printf(\"%.0f\",$1/1000000)}'", $output, $retVal);
		if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
			$output[0] = "0";
		}
		$size = trim($output[0]);		
       
        return $size;						
    }

	/**
	 * Calls the supporting system administration script in the OS services layer
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
    public function saveUserShareState() {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
        
    	// save the share state in case it needs to be restored
		unset($output);
		$retVal = 0;
		exec("sudo /usr/local/sbin/saveUserShareState.sh", $output, $retVal);
		if($retVal !== 0) {
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: 'saveUserShareState.sh' call failed!");
			return false;
		}
    	return true;
    }

	/**
	 * Calls the supporting system administration script in the OS services layer
	 * 
	 * @return boolean $status indicates the success or failure of this operation
	 */
	public function restoreUserShareState() {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
	    
		// restore the share state in case something was modified before the server error
		unset($output);
		$retVal = 0;
		exec("sudo /usr/local/sbin/restoreUserShareState.sh", $output, $retVal);
		if ($retVal !== 0) {
		    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: 'restoreUserShareState.sh' call failed!");
			return false;
		}
		
		return true;
    }
}
	
?>