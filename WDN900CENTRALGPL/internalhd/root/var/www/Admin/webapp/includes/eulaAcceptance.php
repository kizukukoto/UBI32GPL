<?php

class EulaAcceptance{

    function EulaAcceptance() {
    }

    function getAcceptance(){
        if(!is_file("/internalhd/etc/orion/.eula_accepted")){
            return NULL;
        }

        return(array('accepted' => 'yes'));
    }
    
    function accept($data){
        //Require entire representation
        if( !isset($data["accepted"]) ){
            return 'BAD_REQUEST';
        }

        //Verify values are valid
        if($data['accepted'] !== 'yes'){
            return 'BAD_REQUEST';
        }

        //This is a one time resource creation and we want to preserve
        //acceptance time stamp
        if(is_file("/internalhd/etc/orion/.eula_accepted")){
            return 'SUCCESS';
        }

        //Actually accept EULA
        exec("touch /internalhd/etc/orion/.eula_accepted", $output, $retVal);
        if($retVal !== 0) {
            return 'SERVER_ERROR';
        }
		
		// Kill any VFT process after EULA acceptance
		//exec( "sudo killall vft 1>/dev/null 2>&1", $output, $retVal);
        return 'SUCCESS';
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