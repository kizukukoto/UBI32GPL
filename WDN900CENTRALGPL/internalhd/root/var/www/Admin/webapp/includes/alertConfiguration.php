<?php

class AlertConfiguration{

    var $alert_email_config_path = '/etc/alert_email.conf';
    var $alert_notify_time_path = '/CacheVolume/alert_notify_time';
    

    function AlertConfiguration() {
    }
    
    function getLastNotifiedTime(){
	  	if (file_exists($this->alert_notify_time_path)) {
	 		exec("sudo chmod 666 ".$this->alert_notify_time_path);
	  		$alert_notify = parse_ini_file($this->alert_notify_time_path, true);
	  		return $alert_notify[last_notified_time];
	  	}else{
	  		return 0;
	  	}
    }

    function updateLastNotifiedTime($last_notified_time){
        if (file_exists($this->alert_notify_time_path)) {
        	// Make sure we can write.
	 		exec("sudo chmod 666 ".$this->alert_notify_time_path);
        }

        $fh = fopen($this->alert_notify_time_path, 'w') or die("Failed to open alerts config file");

		fwrite($fh, "# Last email notified time\n");
		fwrite($fh, 'last_notified_time="'.$last_notified_time.'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");
		
		fclose($fh);
    }
    
    function getConfig(){
	  	if (file_exists($this->alert_email_config_path)) {
        	// Make sure we can read.
	 		exec("sudo chmod 666 ".$this->alert_email_config_path);
	  		$alert_config = parse_ini_file($this->alert_email_config_path, true);
	  		if (isset($alert_config)) {
        		return( array( 
                    'email_enabled' => $alert_config['email_enabled'],
                    'email_returnpath' => $alert_config['email_returnpath'],
                    'email_recipient_0' => $alert_config['email_recipient_0'],
                    'email_recipient_1' => $alert_config['email_recipient_1'],
                    'email_recipient_2' => $alert_config['email_recipient_2'],
                    'email_recipient_3' => $alert_config['email_recipient_3'],
                    'email_recipient_4' => $alert_config['email_recipient_4'],
                    ));
	  		}else{
	  			return NULL;
	  		}
	  	}     	
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["email_enabled"]) ||
//            !isset($changes["email_returnpath"]) ||
            !isset($changes["email_recipient_0"]) ||
            !isset($changes["email_recipient_1"]) ||
            !isset($changes["email_recipient_2"]) ||
            !isset($changes["email_recipient_3"]) ||
            !isset($changes["email_recipient_4"]) ){

            return 'BAD_REQUEST';
        }

	
        // Save changes to config file.
        if (file_exists($this->alert_email_config_path)) {
        	// Make sure we can write.
	 		exec("sudo chmod 666 ".$this->alert_email_config_path);
        }
        $fh = fopen($this->alert_email_config_path, 'w') or die("Failed to open alerts config file");

		fwrite($fh, "# Email notifications on/off\n");
		fwrite($fh, 'email_enabled="'.$changes["email_enabled"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");

		fwrite($fh, "#Email return address\n");
//		fwrite($fh, 'email_returnpath="'.$changes["email_returnpath"].'"');
		fwrite($fh, 'email_returnpath="nas.alerts@wdc.com"');
		
		fwrite($fh, "\n");
		fwrite($fh, "\n");
		
		fwrite($fh, "# Email address 1\n");
		fwrite($fh, 'email_recipient_0="'.$changes["email_recipient_0"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");


		fwrite($fh, "# Email address 2\n");
		fwrite($fh, 'email_recipient_1="'.$changes["email_recipient_1"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");


		fwrite($fh, "# Email address 3\n");
		fwrite($fh, 'email_recipient_2="'.$changes["email_recipient_2"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");
		
		
		fwrite($fh, "# Email address 4\n");
		fwrite($fh, 'email_recipient_3="'.$changes["email_recipient_3"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");
		
		
		fwrite($fh, "# Email address 5\n");
		fwrite($fh, 'email_recipient_4="'.$changes["email_recipient_4"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");

		/*
		fwrite($fh, "# Last email notified time\n");
		fwrite($fh, 'last_notified_time="'.$changes["last_notified_time"].'"');
		fwrite($fh, "\n");
		fwrite($fh, "\n");
		*/
		
		fclose($fh);
		
		$this->updateLastNotifiedTime(time());
		
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