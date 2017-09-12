<?php

class SshConfiguraion{

    var $enablessh = '';

    function SshConfiguraion() {
        
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
		
		unset($sshServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh ssh", $sshServiceOutput, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		if ($sshServiceOutput[0] === 'enabled') {
			$sshService ="enable";
		}
		else {
			$sshService ="disable";			
		}		
		
		return $sshService;
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["enablessh"]) ){
            return 'BAD_REQUEST';
        }

        //Verify changes are valid
		if(!$this->isValidServiceState($changes["enablessh"])){

            return 'BAD_REQUEST';
        }

		$sshServiceStateRequested = ($changes["enablessh"] === 'enable') ? 'enabled' : 'disabled';

		unset($sshServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh ssh", $sshServiceOutput, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		
        // if the service is currently in the requested state then exit with sucess 
		if ($sshServiceStateRequested === $sshServiceOutput[0]){
			return 'SUCCESS';	
		}     
		
		unset($sshServiceOutput);
		exec("sudo /usr/local/sbin/setServiceStartup.sh ssh \"$sshServiceStateRequested\"", $sshServiceOutput, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
        return 'SUCCESS';

    }

	function isValidServiceState($serviceState){
		if ((strcasecmp($serviceState,'enable') == 0) ||
			 (strcasecmp($serviceState,'disable') == 0)) {
				return TRUE;
			} else {
				return FALSE;
			}
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
