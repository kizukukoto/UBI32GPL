<?php

class SystemState{

	var $status = "";
	var $temperature = "";
	var $smart = "";
    var $volume = "";
    var $free_space = "";
	var $RAIDStatus = "";

    function SystemState() {
    }
    
    function getState(){

        //!!!This where we gather up response  
        //!!!Return NULL on error
        $useShellScriptForReadiness = false;
        $useShellScriptForTemperatureStatus = false;
        $useShellScriptForSmartStatus = false;
        $useShellScriptForFreespaceStatus = false;
        $useShellScriptForVolumeStatus = false;
        $useShellScriptForRAIDStatus = false;
        $retArray = array();
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false)
		{
		    $useShellScriptForReadiness = true;
		}
		else 
		{
		    if (!isset($nasConfig['admin']['READY_FILE']))
		    {
		        $useShellScriptForReadiness = true;
		    }
		    if (!isset($nasConfig['admin']['TEMPERATURE_STATUS_FILE']))
		    {
		        $useShellScriptForTemperatureStatus = true;
		    }
		    if (!isset($nasConfig['admin']['SMART_STATUS_FILE']))
		    {
		        $useShellScriptForSmartStatus = true;
		    }
		    if (!isset($nasConfig['admin']['FREESPACE_STATUS_FILE']))
		    {
		        $useShellScriptForFreespaceStatus = true;
		    }
		    if (!isset($nasConfig['admin']['VOLUME_STATUS_FILE']))
		    {
		        $useShellScriptForVolumeStatus = true;
		    }
		}
        if(is_file("/usr/local/sbin/getSystemState.sh"))
		{
		    if (SKIP_SHELL_SCRIPT && !$useShellScriptForReadiness)
		    {
    		    if (is_file($nasConfig['admin']['READY_FILE']))
                {
                	$this->status = 'ready';
                }
                else
                {
                	$this->status = 'initializing';
                }
		    }
		    else
		    {
		        unset($stateOutput);
		        exec("sudo /usr/local/sbin/getSystemState.sh", $stateOutput, $retVal);
		        if($retVal !== 0) {
		            return NULL;
		        }
		        $this->status = $stateOutput[0];
		    }
		}
		else
		{
			$this->status = "ready";
		}
        $retArray['status'] = "$this->status";

		if (strcasecmp($this->status, "ready") === 0)
		{
		    if (SKIP_SHELL_SCRIPT && !$useShellScriptForTemperatureStatus)
		    {
    		    if (is_file($nasConfig['admin']['TEMPERATURE_STATUS_FILE'])){
                	$this->temperature = 'bad';
                }
                else{
                	$this->temperature = 'good';
                }
		    }
		    else 
		    {
    			unset($temperatureOutput);
    			exec("sudo /usr/local/sbin/getTemperatureStatus.sh", $temperatureOutput, $retVal);
    			if($retVal !== 0) {
    				return NULL;
    			}
    			$this->temperature = $temperatureOutput[0];		        
		    }
            $retArray['temperature'] = "$this->temperature";

		    if (SKIP_SHELL_SCRIPT && !$useShellScriptForSmartStatus)
		    {
    		    if (is_file($nasConfig['admin']['SMART_STATUS_FILE'])){
                	$this->smart = 'bad';
                }
                else {
                	$this->smart = 'good';
                }
		    }
		    else
		    {
    			unset($smartOutput);
    			exec("sudo /usr/local/sbin/getSmartStatus.sh", $smartOutput, $retVal);
    			if($retVal !== 0) {
    				return NULL;
    			}		
    			$this->smart = $smartOutput[0];		        
		    }
            $retArray['smart'] = "$this->smart";

		    if (SKIP_SHELL_SCRIPT && !$useShellScriptForVolumeStatus)
		    {
		        if (is_file($nasConfig['admin']['VOLUME_STATUS_FILE'])){
                	$this->volume = 'bad';
                }
                else{
                	$this->volume = 'good';
                }
		    }
		    else 
		    {
	    	    unset($volumeStatusOutput);
    			exec("sudo /usr/local/sbin/getVolumeStatus.sh", $volumeStatusOutput, $retVal);
    			if($retVal !== 0) {
    				return NULL;
    			}
    			$this->volume = $volumeStatusOutput[0];		        
		    }
            $retArray['volume'] = "$this->volume";

		    if (SKIP_SHELL_SCRIPT && !$useShellScriptForFreespaceStatus)
		    {
		        if (is_file($nasConfig['admin']['FREESPACE_STATUS_FILE'])){
		            $this->free_space = 'bad';
		        }
		        else{
		            $this->free_space = 'good';
		        }
		    }
		    else
		    {
		        unset($freeSpaceOutput);
    			exec("sudo /usr/local/sbin/getFreeSpaceStatus.sh", $freeSpaceOutput, $retVal);
    			if($retVal !== 0) {
    			    return NULL;
    			}
    			$this->free_space = $freeSpaceOutput[0];
		    }
            $retArray['free_space'] = "$this->free_space";

            //If data-volume-config component is installed, check RAID status
            if(is_file("/etc/default/nas/data-volume-config"))
            {
                $dataVolumeConfig = @parse_ini_file('/etc/default/nas/data-volume-config', true);
                //Future work:  If and when data-volume-config is installed on
                //systems without RAIDed user data, add DVC_USER_DATA_RAID=true to config.ini
                //and check for it here.

                if (!isset($dataVolumeConfig['data-volume-config']['DVCDriveFailedFlagFile']))
                {
                    $useShellScriptForRAIDStatus = true;
                }

                if (SKIP_SHELL_SCRIPT && !$useShellScriptForRAIDStatus)
                {
                    if (is_file($dataVolumeConfig['data-volume-config']['DVCDriveFailedFlagFile'])){
                        $this->RAIDStatus = 'bad';
                    }
                    else{
                        $this->RAIDStatus = 'good';
                    }
                }
                else 
                {
                    unset($userRaidStatusOutput);
                    exec("sudo /usr/local/sbin/userRaidStatus.sh", $userRaidStatusOutput, $retVal);
                    if($retVal !== 0) {
                        return NULL;
                    }
                    //Failed drive or Failed RAID map to bad.  All other statues map to good.
                    if (preg_match("/FAILED/", $userRaidStatusOutput[0]))
                    {
                        $this->RAIDStatus = 'bad';
                    }
                    else{
                        $this->RAIDStatus = 'good';
                    }
                }
                $retArray['raid_status'] = "$this->RAIDStatus";
            }

            //To allow UI not to set policy, the reported status(ie setting icon text good/bad) is determined by 'reported_status'
            //Current policy is if any are bad, then it report it bad.
            if(in_array('bad', $retArray)) {
                $retArray['reported_status'] = 'bad';
            }
            else
            {
                $retArray['reported_status'] = 'good';
            }

		}
        return ($retArray);
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
