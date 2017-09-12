<?php

class FirmwareUpdate{
	var $logObj;

    function FirmwareUpdate() {
    	$this->logObj = new LogMessages();
    }
    
    function getFirmwareUpdate(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
		
		$update = array();

		unset($output);
		exec("sudo /usr/local/sbin/getFirmwareUpdateStatus.sh", $output, $retVal);
		if($retVal !== 0) 
		{
        return NULL;
		}		

		if (strcasecmp($output[0],"idle") ===0)
		{
			return array('status' => 'idle', 'completion_percent' => '', 'error_code' => '', 
				'error_description' => '');
		}
		// if we've gotten to this point then there is an update status available
				
		$tempStatus = explode(" ", $output[0]);
		
		if (strcasecmp($tempStatus[0],"failed") ===0)
		{

			$status = explode('"', $output[0]);

			$status2 = explode(" ", $status[0]);
			
			$update = array('status' => $status2[0], 'completion_percent' => '', 'error_code' => $status2[1], 
				'error_description' => $status[1]);
			
		}
		else
		{
			$status = explode(" ", $output[0]);

			$update = array('status' => $status[0], 'completion_percent' => $status[1], 'error_code' => '', 
				'error_description' => '');

			
		}
		return $update; 
				
    }
	
	function manualFWUpdate($changes){
		
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);
		
		if( !isset($changes['filepath'])){
			return 'BAD_REQUEST';
		}
		
		// use this code to copy input file to a certain location
//		$filename = "/CacheVolume/".$_FILES["file"]["name"];
//			
//		unset($output);
//		exec("sudo rm -f \"$filename\" ", $output, $retVal);
//		
//		move_uploaded_file($_FILES["file"]["tmp_name"], $filename); 
		
		unset($output);
		exec("sudo nohup /usr/local/sbin/updateFirmwareFromFile.sh \"{$changes["filepath"]}\" 1>/dev/null &", $output, $retVal);	
		
        return 'SUCCESS';
    }

    function automaticFWUpdate($changes){
    	
    	$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);
    	
    	// $changes has URL link to update package location
		//if( !isset($changes['image'])){
		//	return 'BAD_REQUEST';
		//}
		
		unset($output);
        if (isset($changes['reboot_after_update']) && (strcasecmp($changes['reboot_after_update'], 'true') == 0))
		{
			exec("sudo nohup /usr/local/sbin/updateFirmwareToLatest.sh reboot 1>/dev/null 2>&1 &", $output, $retVal);
		}
		else if (isset($changes['image']))
		{	
			$imageLink = $changes['image'];
			exec("sudo nohup /usr/local/sbin/updateFirmwareToLatest.sh \"$imageLink\" 1>/dev/null 2>&1 &", $output, $retVal);			
		}
		else
		{	
			exec("sudo nohup /usr/local/sbin/updateFirmwareToLatest.sh 1>/dev/null 2>&1 &", $output, $retVal);			
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
