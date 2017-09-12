<?php

require_once('acl.php');
require_once('logMessages.php');
require_once('lock.php');

class Acllist{
    
    static $logObj;
    static $userSharesDB;
    static $lockObj;
    
    public function __construct() {
    	if (!isset(self::$lockObj)) {
    		self::$lockObj = new Lock("Acllist_lock");
    	}        
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
		if (!isset(self::$userSharesDB)) {
			self::$userSharesDB = new UserSharesDB();
		}
	}
	
    /**
     * 
     * Enter description here ...
     * @param unknown_type $shareName
     */
    function getAcls($shareName){
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
        
        $aclsArray = array();
        $remote_access = '';
        $public_access = '';
        $media_serving = '';
        $fullaccess_user_arr = array();
        $readonly_user_arr = array();
        $output = array();
        
        $shareType = array('public', 'private');
//        foreach($shareType as $type){
//            //Get list of shares
//            unset($output);
//            exec("sudo /usr/local/sbin/getShares.sh $type", $output, $retVal);
//            if($retVal !== 0) {
//                return NULL;
//            }

            //For each share, 
            //-Set remote access 'getShareRemoteAccess.sh'
            //-Set public_access of each to true
            //-fill in users with access
//            foreach($output as $shareName) {

//		unset($output);
//		exec("sudo /usr/local/sbin/getShares.sh \"private\"", $output, $retVal);
//		if($retVal !== 0) {
//			return 'SERVER_ERROR';
//		}	
//		$privateShares = $output;

        unset($output);
        exec("sudo /usr/local/sbin/getShareRemoteAccess.sh $shareName", $output, $retVal);
        if($retVal !== 0) {
            return NULL;
        }
        $remote_access = $output[0];

        unset($output);
        exec("sudo /usr/local/sbin/getShareMediaServing.sh $shareName", $output, $retVal);
        if($retVal !== 0) {
            return NULL;
        }
        $media_serving = $output[0];

        $fullaccess_user_arr = array();
        $readonly_user_arr = array();

		if ($this->isPrivateShare($shareName)){
			
			$public_access = "disable";

            $accessType = array('RW', 'RO');
            foreach($accessType as $access){
                unset($output);
                //Get ACL
                exec("sudo /usr/local/sbin/getAcl.sh $shareName $access", $output, $retVal);
                if($retVal !== 0) {
                    return NULL;
                }
                foreach($output as $userwithAccess){
                    if($access === 'RW'){
                        array_push($fullaccess_user_arr, array('user_name' => "$userwithAccess"));
                    } else {
                        array_push($readonly_user_arr, array('user_name' => "$userwithAccess"));
                    }
                }
            }
        }
		else {
			$public_access = "enable";
		}

        $aclObj = new Acl( $shareName,
                           $remote_access,
                           $public_access, 
                           $media_serving,
                           $fullaccess_user_arr,
                           $readonly_user_arr
                         );

        array_push($aclsArray, $aclObj);
        return $aclsArray;
    }

    /**
     * 
     * Enter description here ...
     * @param unknown_type $sharename
     */
	function isPrivateShare($sharename) {
		
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$sharename)");
	    
	    $output = array();
       // if( !isset($privateShares) ) {
    		exec("sudo /usr/local/sbin/getShares.sh \"private\"", $output, $retVal);
    		if($retVal !== 0) {
    			return false;
    		}	
		    $privateShares = $output;
       // }
		
		foreach($privateShares as $testShare){
			if (strcasecmp($testShare,$sharename) == 0){
				return true;
			}						
		}	
		return false;		
	}

    /**
     * 
     * Enter description here ...
     * @param unknown_type $share
     * @param unknown_type $remote_access
     * @param unknown_type $public_access
     * @param unknown_type $media_serving
     */
    function modifyAcl($share, $remote_access, $public_access, $media_serving){
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (share=$share, remote_access=$remote_access, public_access=$public_access, media_serving=$media_serving)");
        
        //Require entire representation 
        if( /* ITR 36324 !isset($remote_access)      || */
            !isset($public_access)      ||
            !isset($media_serving) ){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: required parameter not set");
            return false;
        }

        // convert local dialects and verify:
        // $remote_access
        if ($remote_access === true || 
	        $remote_access === null || /* ITR 36324 */
            $remote_access === 'true' ||
            $remote_access === 'enabled' ||
            $remote_access === 'enable' ||
            $remote_access === '1' ||
            $remote_access === 1
            ) {
            $remote_access = 'enable';
        } 
        else {
            $remote_access = 'disable';
        }
        
        // $public_access
        if ($public_access === true || 
            $public_access === 'true' || 
            $public_access === 'enabled' ||
            $public_access === 'enable' ||
            $remote_access === '1' ||
            $remote_access === 1
            ) {
            $public_access = 'enable';
        } 
        else {
            $public_access = 'disable';
        }

        // $media_serving
        if ($media_serving === 'all') $media_serving = 'any';
        if( $media_serving !== 'none' && 
            $media_serving !== 'any' && 
            $media_serving !== 'music_only' && 
            $media_serving !== 'pictures_only' && 
            $media_serving !== 'videos_only'){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: invalid 'media_serving' parameter: $media_serving");
            return false;
        } 
        
        //Make sure ACL exists
        /* don't waste time here - let the lower level scripts error out if needed
		$aclList = $this->getAcls($share);
        if($aclList === NULL){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: ACL is empty for share $share");
            return false;
        }
        foreach($aclList as $testAcl) {
            if($testAcl->shareName == $share) {
                break;
            }
        }
        if($testAcl->shareName !== $share) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: sharename $share not found");
            return false;
        }
		*/
        
        // Ok to do the changes:
		$status = true;
		unset($output);
		// lock all operations which can change common resources
		// -----------------------------------------------------
		self::$lockObj->Get();
		// -----------------------------------------------------
		do{
        //If remote access changed ...
        if($testAcl->remote_access !== $remote_access){
            unset($output);
            exec("sudo /usr/local/sbin/modShareRemoteAccess.sh $share \"{$remote_access}\"", $output, $retVal);
            if($retVal !== 0) {
                $status = false;
                break;
            }
        }
        //If public access changed ...
        if($testAcl->public_access !== $public_access){
            unset($output);
            if($public_access === 'enable'){
                exec("sudo /usr/local/sbin/setSharePublic.sh $share", $output, $retVal);
                if($retVal !== 0) {
	                $status = false;
	                break;
                }
            } else {
                //Make Private
                exec("sudo /usr/local/sbin/setSharePrivate.sh $share", $output, $retVal);
                if($retVal !== 0) {
	                $status = false;
	                break;
                }
            }
        }
        //If media serving changed ...
        if($testAcl->media_serving !== $media_serving){
            unset($output);
            exec("sudo /usr/local/sbin/modShareMediaServing.sh $share \"$media_serving\"", $output, $retVal);
            if($retVal !== 0) {
                $status = false;
                break;
            }
        }
        } while(false);
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        return $status;
    }

    function modAcl ($shareName, $userName, $accessLevel) {

        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userName=$userName, accessLevel=$accessLevel)");

        //Require entire representation 
        if( !isset($shareName)     ||
            !isset($userName)      ||
            !isset($accessLevel) ) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: required parameter not set");
            return false;
        }
        if (strcasecmp($shareName, 'public') == 0) {
        	// don't bother to change public acl
        	return true;
        }        
        //Verify changes are valid
        if( $accessLevel !== 'RW' && 
            $accessLevel !== 'RO' && 
            $accessLevel !== 'none'){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: invalid 'accessLevel' parameter");
            return false;
        }

        $status = true;
        unset($output);
        // lock all operations which can change common resources
        // -----------------------------------------------------
        self::$lockObj->Get();
        // -----------------------------------------------------
        $output = array();
        exec("sudo /usr/local/sbin/modAcl.sh $shareName $userName $accessLevel", $output, $retVal);
        if($retVal !== 0) {
            $status = false;
        }
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        
        // If this is not an internally generate request, make a call that will persist the updated
        // share access for dynamic volumes so that the access specified will be used every time the
        // dynamic volume is connected.  If the share is not a dynamic volume, no action will be
        // taken.  The request will be internally generated when the access is being restored from
        // a dynamic volume's settings, which are already saved.  No error checking is performed
        // since the call is made unconditionally and will fail for a non-dynamic volume (and the
        // unlikely failure to persist should not prevent the use from changing the settings).

        if (strcasecmp(getenv("INTERNAL_REQUEST"), 'true') != 0) {
            unset($output);
            exec("sudo /usr/local/sbin/wdAutoMountAdm.pm updateShareAccess \"$shareName\" \"$userName\" \"$accessLevel\"", $output, $retVal);
        }
        if($retVal !== 0) {
        	$status = false;
        }
        
        return $status;;
    }
    

    function deleteAcl ($shareName, $userName) {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, userName=$userName)");
        
        //Require entire representation 
        if( !isset($shareName) || !isset($userName) ) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: required parameter not set");
            return false;
        }
        if (strcasecmp($shareName, 'public') == 0) {
        	// don't bother to change public acl
        	return true;
        }
        
        $status = true;
        unset($output);
        // lock all operations which can change common resources
        // -----------------------------------------------------
        self::$lockObj->Get();
        // -----------------------------------------------------
        $output = array();
        exec("sudo /usr/local/sbin/modAcl.sh $shareName $userName 'none'", $output, $retVal);
        if($retVal !== 0) {
            $status = false;
        }
        
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        
        // If this is not an internally generate request, make a call that will persist the updated
        // share access for dynamic volumes.  If the share is not a dynamic volume, no action will
        // be taken.  The request will be internally generated when the access is being deleted due
        // to a dynamic volume being removed.  No error checking is performed since the call is made
        // unconditionally and will fail for a non-dynamic volume (and the unlikely failure to
        // persist should not prevent the use from changing the settings).

        if (strcasecmp(getenv("INTERNAL_REQUEST"), 'true') != 0) {
            unset($output);
            exec("sudo /usr/local/sbin/wdAutoMountAdm.pm updateShareAccess \"$shareName\" \"$userName\" \"none\"", $output, $retVal);
        }
        if($retVal !== 0) {
        	$status = false;
        }
        
        return $status;
    }
	
/*
 * usershares.php
 * 
 *     public function createShare($shareName, $shareDescription, $publicAccess, $mediaServing, $remoteAccess)  {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, shareDescription=$shareDescription, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess)");
        
        $status = self::$userSharesShell->createShare($shareName, $shareDescription); 	
        if ($status === true) {
             $status = self::$shareAccessShell->updateShareAccess($shareName, $publicAccess, $mediaServing, $remoteAccess);

             if ($status === true) {
              	 $status = self::$userSharesDB->createUserShare($shareName, $shareDescription, $publicAccess, $mediaServing, $remoteAccess);
              	 $this->updateMediaCrawlerConfigFile();
            }
        }		  
        return $status;
    }
 */


/*
 * shareaccessdb.php
 * 
 * $success = self::$shareAccessDb->addAclForShare($shareName, $userId, $accessLevel);
 * 
 */
    function setSharesConfigXdb(){
        
        //self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "");
        
        $aclsArray = array();
        $remote_access = '';
        $public_access = '';
        $media_serving = '';
        $output = array();

        $userSharesDB = new UserSharesDB();
        $shareAccessDb = new ShareAccessDb();
        
		unset($allShares);
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $allShares, $retVal);
		if($retVal !== 0) {
		    self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "getShares.sh call failed");
			return NULL;
		}
		
        foreach($allShares as $shareName) {
            //var_dump($shareName);
            
            if ( $shareName === "Public" ) {
                continue;
            }
            unset($output);
            exec("sudo /usr/local/sbin/getShareRemoteAccess.sh $shareName", $output, $retVal);
            if($retVal !== 0) {
       		    self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "getShareRemoteAccess.sh call failed");
                return NULL;
            }
            
            $remote_access = $output[0] === 'enable' ? 'true' : 'false';
    
            unset($output);
            exec("sudo /usr/local/sbin/getShareMediaServing.sh $shareName", $output, $retVal);
            if($retVal !== 0) {
       		    self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "getShareMediaServing.sh call failed");
                return NULL;
            }
            
            $media_serving = $output[0];
    
			unset($output);
			exec("sudo /usr/local/sbin/getShareDescription.sh $shareName", $output, $retVal);
			if($retVal !== 0) {
       		    self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "getShareDescription.sh call failed");
                return NULL;
			}
			$description = $output[0];
    
    		if ($this->isPrivateShare($shareName)){
    			
    			$public_access = 'false';
                $status = $userSharesDB->createUserShare($shareName, $description, $public_access, $media_serving, $remote_access);
                
                $accessType = array('RW', 'RO');
                foreach($accessType as $access){
                    unset($usernameOutput);
                    //Get ACL
                    exec("sudo /usr/local/sbin/getAcl.sh $shareName $access", $usernameOutput, $retVal);
                    if($retVal !== 0) {
                        self::$logObj->LogData('INFO', get_class($this),  __FUNCTION__,  "getAcl.sh call failed");
                        return NULL;
                    }
                    $username = $usernameOutput;
                    foreach($username as $userwithAccess){
                        // userId
    			        unset($outputUserId);
            			exec("sudo /usr/local/sbin/getUserInfo.sh $userwithAccess 'userid'", $outputUserId, $retVal);
            			if($retVal !== 0) {
            			    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "INFO: getUserInfo.sh call failed");
            				return NULL;
            			}
            			$userId = $outputUserId[0];
            			if ($userId !== "") {
                            $shareAccessDb->addAclForShare($shareName, $userId, $access);
            			} else {
            			    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "WARN: userId is not set! ($userId)");
            			}
                    }
                }
            }
    		else {
                $public_access = 'true';
                $status = $userSharesDB->createUserShare($shareName, $description, $public_access, $media_serving, $remote_access);
    		}
        }
    }

    // configure shares in the 'Xternal' db from OS settings
    function getSummary () {
        
        $slObj = new Sharelist();
        
        $shareSummary = $slOjb->getShareSummary();
        
        //var_dump($shareSummary);
        
   		//Create new share with same name, if one does not already exist
//		$UserSharesDB = new UserSharesDB();
//		$share_name    = "upgradedShare";
//		$description   = 'Volume Label';
//		$public_access = 'true';
//		$media_serving = 'any';
//		$remote_access = 'true';
//		$status = $UserSharesDB->createUserShare($share_name, $description, $public_access, $media_serving, $remote_access);
//		if ($status == -1) return false;
//		return $status;     
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
?>
