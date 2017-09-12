<?php

class Power{

    var $state = '';

    function Power() {
    }
    
    function modifyState($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["state"]) ){
            return 'BAD_REQUEST';
        }
        //Verify changes are valid
		if(!$this->isValidState($changes["state"])) {
            return 'BAD_REQUEST';
        }

		if (strcasecmp($changes["state"], "halt") ==0) {
			unset($output);
			exec("sudo halt ", $output, $retVal);
			if($retVal !== 0) {
				return 'SERVER_ERROR';
			}

		}
		else {
			unset($output);
			exec("sudo reboot ", $output, $retVal);
			if($retVal !== 0) {
				return 'SERVER_ERROR';
			}
			
		}
		
		return 'SUCCESS';
    }
	
	function isValidState($state){
		if((strcasecmp($state, "halt") ==0)  || (strcasecmp($state, "reboot") ==0)) {
			return TRUE;					
		}
		return FALSE;		
		
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
