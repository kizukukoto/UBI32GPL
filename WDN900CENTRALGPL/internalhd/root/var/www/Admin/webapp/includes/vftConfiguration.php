<?php

class VftConfiguration{

    var $enablevft = '';

    function VftConfiguration() {
        
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
		
		unset($vftServiceOutput);
		exec("sudo /bin/pidof vft", $vftServiceOutput, $retVal);

		if ($retVal != 0) {
			$vftService ="disable";
		}
		else {
			$vftService ="enable";			
		}		
		
		return $vftService;
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["enablevft"]) ){
            return 'BAD_REQUEST';
        }

        //Verify changes are valid
		if(!$this->isValidServiceState($changes["enablevft"])){

            return 'BAD_REQUEST';
        }

		$vftServiceStateRequested = ($changes["enablevft"] === 'enable') ? 'enabled' : 'disabled';
		
        // if the service is currently in the requested state then exit with sucess 
		unset($vftServiceOutput);
		if ($vftServiceStateRequested === 'enabled'){
	        if(is_file("/etc/.eula_accepted")){
	            header("HTTP/1.0 401 Unauthorized");
				return;
			}
			exec( "sudo nohup /usr/local/sbin/vft 1>/dev/null 2>&1 &", $vftServiceOutput, $retVal);
		}
		else {
			exec( "sudo killall vft 1>/dev/null 2>&1", $vftServiceOutput, $retVal);
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
