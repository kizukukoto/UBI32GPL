<?php

class NetworkWorkgroup{

    var $workname = '';

    function NetworkWorkgroup() {
    }
    
    function getConfig(){

		unset($output);
		exec("sudo /usr/local/sbin/getWorkgroup.sh", $output, $retVal);
		if($retVal !== 0) {
			return NULL;
		}		
		$this->workname = $output[0];

        return( array( 
                    'workname' =>   "$this->workname",
                    ));
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["workname"]) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }

        //Actually do change
		exec("sudo /usr/local/sbin/modWorkgroup.sh \"{$changes["workname"]}\"", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}

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
