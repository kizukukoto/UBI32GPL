<?php

class FirmwareUpdateConfiguration{

    var $auto_install = '';
    var $auto_install_day = '';
    var $auto_install_hour = '';

    function FirmwareUpdateConfiguration() {
    }
    
    function getConfig(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

        $useShellScriptForConfig = false;
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false || !isset($nasConfig['admin']['AUTO_UPDATE_FILE']))
        {
            $useShellScriptForConfig = true;
        }
		if (SKIP_SHELL_SCRIPT && !$useShellScriptForConfig)
		{
		    $firmwareConfig = @parse_ini_file($nasConfig['admin']['AUTO_UPDATE_FILE']);
    		if ($firmwareConfig === false)
    		{
    		    $useShellScriptForConfig = true;
    		}
    		else 
    		{
        		$this->auto_install = $firmwareConfig['au_enable'];
        		$this->auto_install_day = $firmwareConfig['au_day'];
        		$this->auto_install_hour = $firmwareConfig['au_hour'];
    		}
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForConfig)
		{
            unset($output);	
            exec("sudo /usr/local/sbin/getAutoFirmwareUpdateConfig.sh", $output, $retVal);
    		if($retVal !== 0) {
    			return NULL;
    		}
    		
    		$firmwareConfig = explode(" ", $output[0]);
    		$this->auto_install = $firmwareConfig[0];
    		$this->auto_install_day = $firmwareConfig[1];
    		$this->auto_install_hour = $firmwareConfig[2];
		}
		

        return( array( 
                    'auto_install' => "$this->auto_install",
                    'auto_install_day' => "$this->auto_install_day",
                    'auto_install_hour' => "$this->auto_install_hour"));
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["auto_install"]) ||
            !isset($changes["auto_install_day"]) ||
            !isset($changes["auto_install_hour"]) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }

			
        //Actually do change
		unset($output);
			exec("sudo /usr/local/sbin/modAutoFirmwareUpdateConfig.sh \"{$changes["auto_install"]}\" \"{$changes["auto_install_day"]}\" \"{$changes["auto_install_hour"]}\"", $output, $retVal);
		if($retVal !== 0) {
				return 'SERVER_ERROR';
		}		

        return 'SUCCESS';
//        return 'SERVER_ERROR';

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
