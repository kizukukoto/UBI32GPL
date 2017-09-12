<?php

class DeviceRegistration{

	function DeviceRegistration() {
	}

	function getDeviceRegistration(){
		if(!is_file("/etc/.device_registered")){
			return(array('registered' => 'no'));
		}

		return(array('registered' => 'yes'));
	}
	
	function register($data){
		//Require entire representation
		if( !isset($data["registered"]) ){
			return 'BAD_REQUEST';
		}

		//Verify values are valid
		if($data['registered'] !== 'yes'){
			return 'BAD_REQUEST';
		}

		//This is a one time resource creation and we want to preserve
		//registration time stamp
		if(is_file("/etc/.device_registered")){
			return 'SUCCESS';
		}

		//set the device registration as registered
		exec("sudo /bin/touch /etc/.device_registered", $output, $retVal);
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
