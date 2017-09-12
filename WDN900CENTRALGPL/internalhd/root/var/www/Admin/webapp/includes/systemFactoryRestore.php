<?php
//require_once('mionetRegistered.php');
require_once('logMessages.php');

class SystemFactoryRestore{

    var $percent = 0;
    var $status = 'none';
	var $logObj;

    function SystemFactoryRestore() {
		$this->logObj = new LogMessages();
	}
    
    function getStaus(){

		unset($output);
		$status = 'idle';
		$percent = '';
		$useShellScriptForWipeStatus = false;
		$nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
		if ($nasConfig === false || !isset($nasConfig['admin']['WIPE_STATUS_FILE']))
		{
		    $useShellScriptForWipeStatus = true;
		}
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForWipeStatus)
        {
            $wipeStatusFile = $nasConfig['admin']['WIPE_STATUS_FILE'];
    		if (is_file($wipeStatusFile))
    		{
		        $wipeStatus = @file_get_contents($wipeStatusFile);
		        if ($wipeStatus === false)
		        {
		            $useShellScriptForWipeStatus = true;
		        }
		        else
		        {
    		        $statusInfo = explode(" ", trim($wipeStatus));
        		    if (isset($statusInfo[1]))
        		    {
        		        $status = $statusInfo[0];
        		        $percent = $statusInfo[1];
        		    }
		        }
    		}
        }

	    if (!SKIP_SHELL_SCRIPT || $useShellScriptForWipeStatus)
	    {
	        unset($output);
	        exec("sudo /usr/local/sbin/getWipeFactoryRestoreStatus.sh", $output, $retVal);
    		if($retVal !== 0) 
    		{
    			return NULL;
    		}		
    			
    		$status = explode(" ", $output[0]);
    		
    		
    		if (isset($status[1]))
    		{			
    			return array('status' => $status[0], 'completion_percent' => $status[1]);
    		}
    		else
    		{
    			return array('status' => $status[0], 'completion_percent' => '');
    		}
	    }
		return array('status'=>$status, 'completion_percent'=>$percent);

    }

	function restore($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
		if( !isset($changes["erase"]) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){

            return 'BAD_REQUEST';
        }
/*
		$mioRegObj = new MionetRegistered();
		$mionetInfo = $mioRegObj->getRegistered();
		
		// if this user is registered with Mionet then un-register
		if ($mionetInfo['registered'] === "true")
		{
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'unregistering from Mionet for factory restore- result is');
			$mionetChanges = array('registered' => false, 'mionet_username' => $mionetInfo['mionet_username']);
								
			$result = $mioRegObj->modifyRegistered($mionetChanges);

			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $result);
		}
*/
        		
		if (strcasecmp($changes['erase'],"zero") === 0)
		{
			
			// get the status of the wipe and send bad request if a wipe is happening already
			unset($output);
			exec("sudo /usr/local/sbin/getWipeFactoryRestoreStatus.sh", $output, $retVal); 
			if($retVal !== 0) 
			{
				return 'SERVER_ERROR';
			}
			
			$status = explode(" ", $output[0]);
						
			// the status must be idle in order to do a wipe
			if (strcasecmp($status[0],"idle") !==0)		
			{
				return 'BAD_REQUEST';
		
			}	
			else
			{
				
				// do the factory wipe
				unset($output);
				exec("sudo nohup /usr/local/sbin/wipeFactoryRestore.sh 1>/dev/null &", $output, $retVal);
				//exec("sudo nohup /usr/local/sbin/wipeFactoryRestore.sh &", $output, $retVal);
				if($retVal !== 0) 
				{
					return 'SERVER_ERROR';
				}

				return 'SUCCESS';				
				
			}
		}
		
		// if we have gotten this far then the request is for factory restore, not wipe
		
		unset($output);
		exec("sudo /usr/local/sbin/factoryRestore.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return 'SERVER_ERROR';
		}
		
		// reboot after factory wipe
		unset($output);
		exec("sudo reboot ", $output, $retVal);
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
