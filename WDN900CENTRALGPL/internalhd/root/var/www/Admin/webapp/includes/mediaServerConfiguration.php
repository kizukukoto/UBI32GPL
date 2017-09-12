<?php

class MediaServerConfiguration{

    var $enable_media_server = '';

    function MediaServerConfiguration() {
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
	if (file_exists("/etc/nas/service_startup/access")) {
		$media_service = "access";
	} else {
		$media_service = "twonky";
	}

		unset($mediaServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh $media_service", $mediaServiceOutput, $retVal);
		if($retVal !== 0) {
			return NULL;
		}

		if ($mediaServiceOutput[0] === 'enabled') {
			$mediaService ="enable";
		}
		else {
			$mediaService ="disable";			
		}		
		
		return( array( 
			'enable_media_server' => "$mediaService",
			));		
		
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation

	if (file_exists("/etc/nas/service_startup/access")) {
		$media_service = "access";
	} else {
		$media_service = "twonky";
	}

        if( !isset($changes["enable_media_server"]) ){
            return 'BAD_REQUEST';
        }
		//Verify changes are valid
		if(!$this->isValidServiceState($changes["enable_media_server"])){

			return 'BAD_REQUEST';
		}

        //Actually do change
		$mediaServiceStateRequested = ($changes["enable_media_server"] === 'enable') ? 'enabled' : 'disabled';

		unset($mediaServiceOutput);
		exec("sudo /usr/local/sbin/getServiceStartup.sh $media_service", $mediaServiceOutput, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		
		// if the service is currently in the requested state then exit with sucess 
		if ($mediaServiceStateRequested === $mediaServiceOutput[0]){
			return 'SUCCESS';	
		}     

		unset($mediaServiceOutput);
		exec("sudo /usr/local/sbin/setServiceStartup.sh $media_service \"$mediaServiceStateRequested\"", $mediaServiceOutput, $retVal);
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
