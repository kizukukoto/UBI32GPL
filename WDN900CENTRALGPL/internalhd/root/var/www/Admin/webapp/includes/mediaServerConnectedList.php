<?php

class MediaServerConnectedList{

    function MediaServerConnectedList() {
    }
    
    function getConnectedList(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

		$mediaServerList = array();

		unset($MediaServerConnectedOutput);
		exec("sudo /usr/local/sbin/getMediaServerConnectedList.sh", $MediaServerConnectedOutput, $retVal);
		if($retVal !== 0) {
			return NULL;
		}			
		
		$index = 0;
		
		foreach($MediaServerConnectedOutput as $mediaConnected) {
													
			$deviceWithBlanks = explode('"',$mediaConnected);

			//strip the spaces from the device array
			foreach ($deviceWithBlanks as $key=>$value){
				//if ($value == "" || $value == " ") {
				if ($value == " ") {
					unset($deviceWithBlanks[$key]);	
				}	
			}
			
			// unset the leading blank string
			unset($deviceWithBlanks[0]);
						
			$device = array_values($deviceWithBlanks);
			
			$macAddress = trim($device[0], '"');
			$ipAddress = trim($device[1], '"');
			$friendlyName = trim($device[2], '"');
			$deviceDesc = trim($device[3], '"');
			$deviceEnable = trim($device[4], '"');
							
			$mediaServerList[$index] = array('mac_address' => $macAddress, 'ip_address' => $ipAddress, 'friendly_name' => $friendlyName, 
						'device_description' => $deviceDesc, 'device_enable' => $deviceEnable);						
			$index++;
		}
		
		return  $mediaServerList;
    }
	
	
    function modifyMediaDevice($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE) {
            return 'BAD_REQUEST';
        }

		foreach ($changes['device'] as $device)
		{			
			//$deviceEnabled = ($device['device_enable'] === 'enable') ? 'enabled' : 'disabled';
						
			//Back up files in case of failure
			exec("sudo /usr/local/sbin/modMediaServerEnable.sh \"{$device["mac_address"]}\" \"{$device["device_enable"]}\" ", $output, $retVal);
			if($retVal !== 0) {
				
				return 'DEVICE_NOT_FOUND';
			}
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
