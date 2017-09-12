<?php

class HddStandbyTime{

    var $enable_hdd_standby = '';
    var $hdd_standby_time_minutes ='';

    function HddStandbyTime() {
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

		// get the network configuration
		$useShellScriptForStandby = false;
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false || !isset($nasConfig['admin']['HDD_STANDBY_FILE']))
        {
            $useShellScriptForStandby = true;
        }
		if (SKIP_SHELL_SCRIPT && !$useShellScriptForStandby)
		{
            $standByConf = @parse_ini_file($nasConfig['admin']['HDD_STANDBY_FILE']);
            if ($standByConf === false)
            {
                $useShellScriptForStandby = true;
            }
            else
            {
                if ($standByConf['standby_enable'] == 'enabled')
                {
                    $this->enable_hdd_standby = 'enable';
                }
                else 
                {
                    $this->enable_hdd_standby = 'disable';
                }
    		    $this->hdd_standby_time_minutes = $standByConf['standby_time'];
            }
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForStandby)
        {
            unset($output);
    		exec("sudo /usr/local/sbin/getHddStandbyConfig.sh", $output, $retVal);
    		if($retVal !== 0) {
    			return NULL;
    		}
    		$hddStandby = explode(" ", $output[0]);
    		$this->enable_hdd_standby = ($hddStandby[0] === 'enabled') ? 'enable' : 'disable';
    		
    		$this->hdd_standby_time_minutes = $hddStandby[1];
        }
        return( array( 'enable_hdd_standby' => "$this->enable_hdd_standby",
                       'hdd_standby_time_minutes' => "$this->hdd_standby_time_minutes"));
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["enable_hdd_standby"]) ||
            !isset($changes["hdd_standby_time_minutes"]) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE)
		{
            return 'BAD_REQUEST';
        }

		$hddEnable = ($changes["enable_hdd_standby"] === 'enable') ? 'enabled' : 'disabled';

		unset($output);
			exec("sudo /usr/local/sbin/modHddStandbyConfig.sh \"$hddEnable\" \"{$changes["hdd_standby_time_minutes"]}\"", 
				$output, $retVal);
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
