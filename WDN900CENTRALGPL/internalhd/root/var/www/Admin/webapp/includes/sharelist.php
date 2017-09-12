<?php

require_once('lock.php');
require_once('share.php');
require_once('acllist.php');
require_once('logMessages.php');

class Sharelist{

    var $shareName = '';
    var $shareDesc = '';
    
    static $logObj;
    static $lockObj;
    
    public function __construct() {
    	if (!isset(self::$lockObj)) {
    		self::$lockObj = new Lock("Sharelist_lock");
    	}
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
	}
    
	function getShareSummary(){
			
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
/*    
		$output = array();
		exec("sudo /usr/local/sbin/getShares.sh \"private\"", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}	
		$privateShares = $output;
*/
	    
		// first get the share names
		unset($output);
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}		
		$shareNames = $output;
		$share = array('share' => array());
	    $aclObj = new Acllist();

	    foreach($shareNames as $testShare){			
			
			unset($output);
			exec("sudo /usr/local/sbin/getShareDescription.sh \"$testShare\"", $output, $retVal);
			if($retVal !== 0) {
				return 'SERVER_ERROR';
			}
			$shareDesc = $output[0];
			
			// output in MBs
			unset($output);
			exec("sudo cat /var/local/nas_file_tally/\"$testShare\"/total_size_bytes | awk '{printf(\"%.0f\",$1/1000000)}'", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0";
            }
			$size = trim($output[0]);		
			
			unset($outputRemoteAccess);
			exec("sudo /usr/local/sbin/getShareRemoteAccess.sh \"$testShare\"", $outputRemoteAccess, $retVal);
			if($retVal !== 0) {
				return NULL;
			}			
			$remoteAccess = $outputRemoteAccess[0];
			
			unset($outputMediaServing);
			exec("sudo /usr/local/sbin/getShareMediaServing.sh \"$testShare\"", $outputMediaServing, $retVal);
			if($retVal !== 0) {
				return NULL;
			}			
			$mediaServing = $outputMediaServing[0];
			
			if ($aclObj->isPrivateShare($shareName)){
				$publicAccess = "disable";
			}
			else {
				$publicAccess = "enable";				
			}
			
			$share['share']["$testShare"] = array('sharename' => "$testShare", 'sharedesc' => "$shareDesc",
				'size' => $size, 'remote_access' => $remoteAccess, 'public_access' => $publicAccess,
				'media_serving' => $mediaServing, 'fullaccess_user_arr' => array(), 'readonly_user_arr' => array());

			// only get access information for private shares
			if ($aclObj->isPrivateShare($shareName))
			{
				$accessInfo = array("RW", "RO");
				foreach ($accessInfo as $access) {
									
					unset($output);
					//Get ACL
					exec("sudo /usr/local/sbin/getAcl.sh \"$testShare\" \"$access\"", $output, $retVal);
					if($retVal !== 0) {
						return NULL;
					}
					
					$aclUsers = $output;
					
					// if there are users 
					if($aclUsers != NULL)  {
						
						// foreach user returned 
						foreach($aclUsers as $user){									
							
							unset($output);

							//Get full name
							exec("sudo /usr/local/sbin/getUserInfo.sh \"$user\"", $output, $retVal);
							if($retVal !== 0) {								
								return NULL;
							}							
							$fullname = $output[0];
														
							if ($fullname === null) {							
								$fullname ="*";
							}
								
														
							if ($access === "RW") {
								array_push($share['share']["$testShare"]['fullaccess_user_arr'], 
									array('username' => $user, 'fullname' => $fullname));
							}
							else {
								array_push($share['share']["$testShare"]['readonly_user_arr'], 
									array('username' => $user, 'fullname' => $fullname));							
							}
						}
					}
				}
		    }										
		}				
		return $share;
	}
	
	function getShare($share){
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (share=$share ");
	    
		// Go through each share and collect name and description		
		$output = array();
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}		
		$shareNames = $output;
		
		unset($output);

		exec("sudo /usr/local/sbin/getShareDescription.sh \"$share\"", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		$shareDesc = $output[0];
		
		return array ('sharedesc' => $shareDesc );			
		
	}
	
	
//    function getShares(){
//        // Go through each share and collect name and description
//		
//		$sharesArray = array();
//		
//				
//		// first get the share names
//		unset($output);
//		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
//		if($retVal !== 0) {
//			return 'SERVER_ERROR';
//		}		
//		
//		$shareNames = $output;
//		
//		
//		// loop through each share name and get description and create share instance
//		foreach($shareNames as $testShare){			
//			$this->sharename = $testShare;						
//			unset($output);
//
//			exec("sudo /usr/local/sbin/getShareDescription.sh \"$testShare\"", $output, $retVal);
//			if($retVal !== 0) {
//				return 'SERVER_ERROR';
//			}
//			$this->sharedesc = $output[0];
//						
//			$shareObj = new Share( $this->sharename,
//				$this->sharedesc );			
//			
//			array_push($sharesArray, $shareObj);       
//		}		
//
//		return $sharesArray;
//    }

	function shareExists($shareName){
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		// Go through each share and collect name and description		
		$output = array();
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
		if($retVal !== 0) {
                self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: getShares.sh call failed");
			    return false;
		}		
		$shareNames = $output;
		
		// verify that the share exists
		$shareExists = false;
		foreach($shareNames as $testShare){	
			//
			if (strcasecmp($testShare, $shareName) == 0) {
				$shareName = $testShare;
				$shareExists = true;
				break;
			}			
		}

		return $shareExists;
	}
	
	/**
	 * Handles the shell script executions required for this OS service
	 * 
	 * @param string $shareName holds the current share name
 	 * @param string $newshareName holds the new share name
	 * @param string $newDescription holds the new description of the share
	 * @return boolean $status indicates the success or failure of this operation
	 */
    function modifyShare ($shareName, $newShareName, $newDescription = '') {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, newShareName=$newShareName, newDescription=$newDescription)");
        
        //printf("modifyShare (%s, %s, %s)\n",$shareName, $newShareName, $newDescription);
/*
        if( !isset($shareName) ||
            !isset($newDescription) ){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: required parameter not set");
	        return false;
        }
		//Verify changes are valid
		if(!$this->isValidShareName($newShareName)  || 
			!$this->isValidDescription($newDescription) ){
            return false;
		}
*/	
			// get the share names
            $output = array();
			exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
			if($retVal !== 0) {
                self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: getShares.sh call failed");
			    return false;
			}		
			$shareNames = $output;
			
			// lock all operations which can change common resources
			// -----------------------------------------------------
			self::$lockObj->Get();
			// -----------------------------------------------------

			$status = true;
			do{
				// if the share name should be modified
				if((strcasecmp($shareName, $newShareName) !=0 ) && (!empty($newShareName))) {
					//Don't allow renaming share to an already existing share name.
					foreach($shareNames as $test2Share){								
						if(strcasecmp($test2Share, $newShareName) ==0) {
						    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: this share name is taken: $newShareName");
							$status = false;
							break;
						}
					}
					if ( $status == false ) { 
						break; 
					}
				   //change the share name
					unset($output);
					exec("sudo /usr/local/sbin/modShareName.sh \"$shareName\" \"$newShareName\"", $output, $retVal);
					if($retVal !== 0) {
					    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: modShareName.sh call failed");
						$status = false;
						break;
					}				
				}
	            //change the description
				unset($output);
				exec("sudo /usr/local/sbin/modShareDescription.sh \"$shareName\" \"$newDescription\"", $output, $retVal);
				if($retVal !== 0) {
				    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: modShareDescription.sh call failed");
					$status = false;
				}
				
			} while(false);
			// Release lock
			// -----------------------------------------------------
			self::$lockObj->Release();
			// -----------------------------------------------------
			
			return $status;			
    }
	
    /**
     * Modify a Dynamic Share
     * 
     * @param string $shareName name of share
     * @param string $newShareName new share name if renaming share
     * @param string $shareDescription descripion of share
     * @param boolean $publicAccess whether public access is allowed
     * @param string $mediaServing type of media served by the share
     * @param boolean $remoteAccess whether remote access is allowed
     */

    public function modifyDynamicShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess) {
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, newShareName=$newShareName, newDescription=$newDescription, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess)");
		$status = true;
        unset($output);
        // lock all operations which can change common resources
        // -----------------------------------------------------
        self::$lockObj->Get();
        // -----------------------------------------------------
        exec("sudo /usr/local/sbin/wdAutoMountAdm.pm \"updateShare\" \"$shareName\" \"$newShareName\" \"$newDescription\" \"$publicAccess\" \"$mediaServing\" \"$remoteAccess\"", $output, $retVal);
        if($retVal !== 0) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: wdAutoMountAdm.pm updateShare failed $retVal");
            $status = false;
        }
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        return $status;			
    }

	function isValidShareName($shareName){
	    
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");

        if( (strlen($shareName) > 32) ||
				(preg_match("/^[a-z0-9][a-z0-9_]*$/i", $shareName) == 0) ) {
    	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: $shareName not valid");
			return FALSE;
		} else {
			return TRUE;
		}
	}

	function isValidDescription($shareDescription){
	    
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareDescription)");
	    
		if($shareDescription === ''){
			return TRUE;
		}
		if( (strlen($shareDescription) > 256) ||
				(preg_match("/^[a-z0-9]/i", $shareDescription) == 0) ) {
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: $shareDescription not valid");
			return FALSE;
		} else {
			return TRUE;
		}

	}	

	/**
	 * Handles the shell script executions required for this OS service
	 * 
	 * @param string $shareName holds the new share name
	 * @param string $shareDescription holds the description of the new share
	 * @return boolean $status indicates the success or failure of this operation
	 */
    function createShare($shareName, $shareDescription){

         self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, shareDescription=$shareDescription)");

		/// Default public share already exists
		if (strcasecmp($shareName, "public") == 0) 
		{	
		    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "Create share 'Public' - returing true.");
            return false;
		}		
		
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($shareName) ||
            !isset($shareDescription) ){
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: required parameter not set");
	        return false;
        }
        //Verify changes are valid 
		if(!$this->isValidShareName($shareName)  || 
				!$this->isValidDescription($shareDescription) ){
                self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: Invalid share name.");
				return false;
		}

		$status = true;
		unset($output);
		// lock all operations which can change common resources
		// -----------------------------------------------------
		self::$lockObj->Get();
		// -----------------------------------------------------
        // create the new share
	    exec("sudo /usr/local/sbin/createShare.sh \"$shareName\" \"$shareDescription\"", $output, $retVal);
        if($retVal !== 0) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: 'createShare.sh' call failed");
		    $status = false;
        }
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        return $status;
    }

	/**
	 * Handles the shell script executions required for this OS service
	 * 
	 * @param string $shareName holds the share name to be deleted
	 * @return boolean $status indicates the success or failure of this operation
	 */
    function deleteShare($shareName)
	{
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
	    
		//Don't allow deleting pre-configured public share
		if (strcasecmp($shareName, "public") == 0) 
		{	
		    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: deletion of share 'public' is prohibited");
            return false;
		}		

		$output = array();
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
		if($retVal !== 0) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: 'getShares.sh' call failed");
			return false;
		}		
		$shareNames = $output;

    	// verify that the share exists
    	/*
		$shareExists = false;
		foreach($shareNames as $testShare)
		{		
			if (strcasecmp($testShare, $shareName) == 0) 
			{
				$shareName = $testShare;
				$shareExists = true;
				break;
			}			
		}		
		if (!$shareExists) {	
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: can't delete non-existent share $shareName");
            return true;
		}		
		*/
		
		$status = true;
		unset($output);
		// lock all operations which can change common resources
		// -----------------------------------------------------
		self::$lockObj->Get();
		// -----------------------------------------------------
		exec("sudo /usr/local/sbin/deleteShare.sh \"$shareName\"", $output, $retVal);
		if($retVal !== 0) 
		{
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: call to 'deleteShare.sh' failed with status $status");
			$status = false;
		}
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        return $status;
    }
    
	/**
	 * OS service to delete all user shares
	 * 
	 * @param string $shareName holds the share name to be deleted
	 * @return boolean $status indicates the success or failure of this operation
	 */
    function deleteAllShares()
	{
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");

		$output = array();
		exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
		if($retVal !== 0) {
            self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: 'getShares.sh' call failed");
			return false;
		}
		
		$shareNames = $output;
		$status = true;
		unset($output);
		// lock all operations which can change common resources
		// -----------------------------------------------------
		self::$lockObj->Get();
		// -----------------------------------------------------
		foreach($shareNames as $testShare)
		{		
			if (strcasecmp($testShare, "public") == 0) 
			{
                continue;
			}
			$shareName = $testShare;
			
			unset($output);
    		exec("sudo /usr/local/sbin/deleteShare.sh \"$shareName\"", $output, $retVal);
    		if($retVal !== 0) 
    		{
    			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: call to 'deleteShare.sh' failed with status $status");
    			$status = false;
    			break;
    		}
		}		
        // Release lock
        // -----------------------------------------------------
        self::$lockObj->Release();
        // -----------------------------------------------------
        return $status;		
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
