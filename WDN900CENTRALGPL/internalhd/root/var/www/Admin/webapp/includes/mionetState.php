<?php

class MionetState{

    var $state = 'stop';

    function MionetState() {
    }
    
    function getState(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
/*
	 	$output=exec("sudo /usr/mionet/mionetd status");
        if($output=="1"){
           $this->state = "start";
        }
        else {
           $this->state = "stop";
        } 
        
        return( array( 'state' => "$this->state"));
*/
    	$this->state = "stop";
    	return( array( 'state' => "$this->state"));
    }

    function modifyState($changes){
    	
    	return 'BAD_REQUEST';
/*
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["state"]) ){
            return 'BAD_REQUEST';
        }
	
        //Verify changes are valid
        $output=exec("sudo /usr/mionet/mionetd status");
	    if(($output=="1" &&$changes["state"]=="start") ||($output=="0" &&$changes["state"]=="stop")) {  
	    	return 'BAD_REQUEST';
       	}
		if($changes["state"]=="start"){
	  		$output=exec("sudo /usr/local/sbin/setServiceStartup.sh mionet enabled");
       	}
		else if($changes["state"]=="stop"){
	  		$output=exec("sudo /usr/local/sbin/setServiceStartup.sh mionet disabled");
       	}
        //Actually do change
       	else {
	  		return 'BAD_REQUEST';
		}

        return 'SUCCESS';
*/
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