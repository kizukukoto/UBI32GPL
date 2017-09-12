<?php
require_once('device.inc');

class DeviceDescripton{

    function DeviceDescripton() {
    	
    }
    
    function getDescription(){
    	//JS moved function to DeviceControl as code in DeviceControl needs to call it and there would be
    	//a circular dependency between DeviceDescription and DeviceControl
        return DeviceControl::getInstance()->getDeviceDescription();
    }

    function isValidMachineName($name){
        if( (strlen($name) > 15) ||
            (strlen($name) == 0) ){
            return FALSE;
        }

        // Don't allow all numbers
        if (preg_match("/^[0-9]+$/", $name)){
            return FALSE;
        }

        // Allow alphanumeric and hyphens
        if(preg_match("/^[a-z0-9\-]+$/i", $name)){
            return TRUE;
        } else {
            return FALSE;
        }
    }

    function isValidMachineDescription($desc){
        if($desc === ''){
            return TRUE;
        }

        if( (strlen($desc) > 42) ||
            (preg_match("/^[a-z0-9]/i", $desc) == 0) ) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    function modifyDescription($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["machine_name"]) ||
            !isset($changes["machine_desc"]) ){

            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(!$this->isValidMachineName($changes["machine_name"])  || 
           !$this->isValidMachineDescription($changes["machine_desc"]) ){

            return 'BAD_REQUEST';
        }

       //Actually do change
       exec("sudo /usr/local/sbin/modDeviceName.sh \"{$changes["machine_name"]}\" \"{$changes["machine_desc"]}\"", $output, $retVal);
       if($retVal !== 0) {
           return 'SERVER_ERROR';
       }

       // Update name with orion.
       DeviceControl::getInstance()->updateDeviceName($changes["machine_name"]);
       
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