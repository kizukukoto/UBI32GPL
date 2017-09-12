<?php
 
class ItunesConfiguration{

    var $enable_itunes_server = '';

    function ItunesConfiguration() {
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
		unset($itunesServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh itunes", $itunesServiceOutput, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		if ($itunesServiceOutput[0] === 'enabled') {
			$itunesService ="enable";
		}
		else {
			$itunesService ="disable";			
		}		
		
		
        return( array( 
                    'enable_itunes_server' => "$itunesService",
                    ));
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["enable_itunes_server"]) ){
            return 'BAD_REQUEST';
        }
		
		//Verify changes are valid
		if(!$this->isValidServiceState($changes["enable_itunes_server"])){

			return 'BAD_REQUEST';
		}
		
        //Actually do change

		$itunesServiceStateRequested = ($changes["enable_itunes_server"] === 'enable') ? 'enabled' : 'disabled';

		unset($itunesServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh itunes", $itunesServiceOutput, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		
		// if the service is currently in the requested state then exit with sucess 
		if ($itunesServiceStateRequested === $itunesServiceOutput[0]){
			return 'SUCCESS';	
		}     

		unset($itunesServiceOutput);
		exec("sudo /usr/local/sbin/setServiceStartup.sh itunes \"$itunesServiceStateRequested\"", $itunesServiceOutput, $retVal);
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
