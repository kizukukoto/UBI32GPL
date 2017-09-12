<?php
class ShareAccessShell 
{
	static $os_shareAccess;
    static $logObj;
    
	public function __construct() {

	    require_once('acllist.php');
	    require_once('logMessages.php');
        
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
		if (!isset(self::$os_shareAccess)) {
			self::$os_shareAccess = new Acllist();
		}
	}
    
	public function updateShareAccess($shareName, $publicAccess, $mediaServing, $remoteAccess) {
	    
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess)");
        
		$status = self::$os_shareAccess->modifyAcl($shareName, $remoteAccess, $publicAccess, $mediaServing);
		return true;
	}
	
	public function getAclForShare($shareName) {
        
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
    
	    $aclsArray = array();
	    $aclsArray = self::$os_shareAccess->getAcls($shareName);
	    // var_dump($aclsArray);
		return $aclsArray;
	}
	
	public function addAclForShare($shareName, $userName, $accessLevel) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userName=$userName, accessLevel=$accessLevel)");
	    
	    $status = self::$os_shareAccess->modAcl($shareName, $userName, $accessLevel);
		return true;
	}
	
	public function updateAclForShare($shareName, $userName, $accessLevel) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userName=$userName, accessLevel=$accessLevel)");
	    
	    $status = self::$os_shareAccess->modAcl($shareName, $userName, $accessLevel);
	    return true;
	}
	
	public function deleteAclForShare($shareName, $userName) {
        
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userName=$userName)");
   	    
		$status = self::$os_shareAccess->deleteAcl($shareName, $userName);
   	    return true;
	}
	
	public function deleteAllAclsForShare($shareName ) {
        
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		return true;
	}
	
	public function deleteAllAclsForUser($userName ) {
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userName=$userName )");
        unset($output);
        exec("sudo /usr/local/sbin/wdAutoMountAdm.pm deleteUserAccess \"$userName\"", $output, $retVal);
        if($retVal !== 0) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: wdAutoMountAdm.pm deleteUserAccess failed $retVal");
        }				
		return true;
	}

	public function isPrivateShare($shareName) {
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		if (strcasecmp($shareName, 'public') === 0) {
			return false;
		}
	    
		$isPrivate =  self::$os_shareAccess->isPrivateShare($sharename);
		
		return $isPrivate;
	}
	
}


?>