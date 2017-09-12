<?php

require_once('timeZones.php');
require_once('logMessages.php');

class DateTimeConfiguration{
	var $logObj;

    function DateTimeConfiguration() {
		$this->logObj = new LogMessages();
	}
    
    function getConfig(){
        $datetime = date('U');

        $useShellScriptForNtpdate = false;
        $useExecForNtpServers = false;
        $useShellScriptForTimeZone = false;
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false)
        {
            $useShellScriptForNtpdate = true;
            $useExecForNtpServers = true;
            $useShellScriptForTimeZone = true;
        }
        else
        {
            if (!isset($nasConfig['admin']['NTPDATE_FILE']))
            {
                $useShellScriptForNtpdate = true;
            }
            if (!isset($nasConfig['admin']['SH_SETUP_PARAM_FILE']))
            {
                $useExecForNtpServers = true;
            }
            if (!isset($nasConfig['admin']['TIMEZONE_FILE']))
            {
                $useShellScriptForTimeZone = true;
            }
        }
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForNtpdate)
        {
            unset($ntpdate);
            $ntpdate = @file_get_contents($nasConfig['admin']['NTPDATE_FILE']);
            if ($ntpdate === false)
            {
                $useShellScriptForNtpdate = true;
            }
            else
            {
                $ntpdate = trim($ntpdate);
                if ($ntpdate == 'enabled'){
                    $ntpservice = 'on';
                }
                else{
                    $ntpservice = 'off';
                }
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForNtpdate)
        {
            unset($ntpdate);
            exec("sudo /usr/local/sbin/getServiceStartup.sh ntpdate", $ntpdate, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $ntpservice = ($ntpdate[0] === 'enabled') ? 'on' : 'off';
        }

//retrieve ntpdate file name
        $ntpsrv = array();
        $ntpsrv_user = '';
        if (SKIP_SHELL_SCRIPT && !$useExecForNtpServers)
        {
            $handle = @fopen($nasConfig['admin']['SH_SETUP_PARAM_FILE'], 'r');
            unset($ntpConfig);
            if ($handle === false)
            {
                $useExecForNtpServers = true;
            }
            else
            {
                while (($buffer = fgets($handle)) !== false)
                {
                    if ($buffer[0] != '#')
                    {
                        $data = explode('=', trim($buffer));
                        if ($data[0] == 'ntpConfig')
                        {
                            $ntpConfig = str_replace('"','', $data[1]);
                            break;
                        }
                    }
                }
                fclose($handle);
                if (!isset($ntpConfig))
                {
                    $useExecForNtpServers = true;
                }
                else
                {
                    $handle = @fopen($ntpConfig, 'r');
                    if ($handle !== false)
                    {
                            while (($buffer = fgets($handle)) !== false)
                            {
                                    if ($buffer[0] != '#')
                                    {
                                            $data = explode('=', trim($buffer));
                                            if ($data[0] == 'NTPSERVERS')
                                            {
                                                $ntpsrvinfo = (explode(' ', str_replace('"','',trim($data[1]))));
                                                if (count($ntpsrvinfo) == 3)
                    			    {
                                                        $ntpsrv[0] = $ntpsrvinfo[1];
                                                        $ntpsrv[1] = $ntpsrvinfo[2];
                                                        $ntpsrv_user = $ntpsrvinfo[0];
                                                }
                                                else
                                                {
                                                        $ntpsrv[0] = $ntpsrvinfo[0];
                                                        $ntpsrv[1] = $ntpsrvinfo[1];
                                                }
                                                break; 
                                            }
                                    }
                            }
                            fclose($handle);
                    }
                    else
                    {
                            $useExecForNtpServers = true;
                    }
                }
            }
        }
        
        if (!SKIP_SHELL_SCRIPT || $useExecForNtpServers)
        {
            unset($ntpsrv);
            exec("sudo /usr/local/sbin/getFixedNtpServer.sh", $ntpsrv, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            unset($output);
            exec("sudo /usr/local/sbin/getExtraNtpServer.sh", $output, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
    
            if(count($output) === 0){
                $ntpsrv_user = '';
            } else {
                $ntpsrv_user = $output[0];
            }    
        }

        unset($time_zone_name);
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForTimeZone)
        {
            $time_zone_name = @file_get_contents($nasConfig['admin']['TIMEZONE_FILE']);
            if ($time_zone_name === false)
            {
                $useShellScriptForTimeZone = true;
            }
            else
            {
                $time_zone_name = trim($time_zone_name);
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForTimeZone)
        {
            unset($time_zone_name);
            exec('sudo sed -n -e \'1p\' /etc/timezone',  $time_zone_name, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $time_zone_name = $time_zone_name[0];
        }
        

        return ( array (
                     'datetime'       => "$datetime",
                     'ntpservice'     => "$ntpservice",
                     'ntpsrv0'        => "$ntpsrv[0]",
                     'ntpsrv1'        => "$ntpsrv[1]",
                     'ntpsrv_user'    => "$ntpsrv_user",
//                     'time_zone_name' => "$time_zone_name[0]",
                     'time_zone_name' => $time_zone_name,
                     ));
    }

    function modifyConf($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["datetime"]) ||
            !isset($changes["ntpservice"]) ||
            !isset($changes["ntpsrv0"]) ||
            !isset($changes["ntpsrv1"]) ||
            !isset($changes["ntpsrv_user"]) ||
            !isset($changes["time_zone_name"]) ){

            return 'BAD_REQUEST';
        }
		
			
        //Validate $changes["ntpservice"]
        if($changes["ntpservice"] !== 'on' && $changes["ntpservice"] !== 'off'){
            return 'BAD_REQUEST';
        }

		$timeZonesObj = new TimeZones();
        $timeZoneList = $timeZonesObj->getTimeZones();
        if($timeZoneList === NULL){
            return 'SERVER_ERROR';
        }
					
        //Validate $changes["time_zone_name"]
        if(!array_key_exists($changes["time_zone_name"], $timeZoneList)){
            return 'BAD_REQUEST';
        }

        //Back up files in case of failure
        exec("sudo /bin/cp /etc/timezone /etc/timezone.bak", $output, $retVal);
        if($retVal !== 0) {
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'timezone backup failed');
			return 'SERVER_ERROR';
        }

        exec("sudo /bin/cp /etc/localtime /etc/localtime.bak", $output, $retVal);
        if($retVal !== 0) {
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'local time backup failed');
			return 'SERVER_ERROR';
        }

        //Actually do change
        $restoreList = array();

        do{

			$curConfig = $this->getConfig();
            if($curConfig === NULL) 
			{
                break;
            }
				
			if($changes['ntpservice'] === 'off'){			
				unset($output);

				exec("sudo date -s@\"{$changes["datetime"]}\"", $output, $retVal);
				if($retVal !== 0) {
					$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'setting date failed');
					return 'SERVER_ERROR';
				}

			}
			
            //Update time zone if it changed
            if($curConfig['time_zone_name'] !== $changes['time_zone_name']){
                exec("sudo bash -c '(echo \"{$changes['time_zone_name']}\">/etc/timezone)'", $output, $retVal);
                if($retVal !== 0) {
						$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'updating timezone failed');
						break;
                }
                array_push($restoreList, "sudo /bin/mv /etc/timezone.bak /etc/timezone");
                exec("sudo /bin/cp /usr/share/zoneinfo/\"{$changes['time_zone_name']}\" /etc/localtime", $output, $retVal);
                if($retVal !== 0) {
						$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'updating timezone 2 failed');
						break;
                }
                array_push($restoreList, "sudo /bin/mv /etc/localtime.bak /etc/localtime");
            }

            //UPdate user defined NTP server if it changed
            if($curConfig['ntpsrv_user'] !== $changes['ntpsrv_user']){
                unset($output);
                if($changes['ntpsrv_user'] === ''){
                    exec("sudo /usr/local/sbin/modExtraNtpServer.sh", $output, $retVal);
                } else {
                    exec("sudo /usr/local/sbin/modExtraNtpServer.sh \"{$changes['ntpsrv_user']}\"", $output, $retVal);
                }
                if($retVal !== 0) {
						$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'updating ntp service failed');
						break;
                }
                array_push($restoreList, "sudo /usr/local/sbin/modExtraNtpServer.sh {$curConfig['ntpsrv_user']}");
            }

            //Update NTP service enable if it changed
            if($curConfig['ntpservice'] !== $changes['ntpservice']){
                unset($output);
                if($changes['ntpservice'] === 'on'){
                    //Enable start up service
						exec("sudo /usr/local/sbin/setServiceStartup.sh ntpdate enabled", $output, $retVal);
                    if($retVal !== 0) {
							$this->logObj->LogParameters(get_class($this), __FUNCTION__, $output);
							$this->logObj->LogParameters(get_class($this), __FUNCTION__, $retVal);
							
							$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'enabling NTP startup service failed');
							break;
                    }
                    array_push($restoreList, "sudo /usr/local/sbin/setServiceStartup.sh ntpdate disabled");
                    //Update time
                    unset($output);
//                    exec("sudo /usr/sbin/ntpdate-debian", $output, $retVal);
//                    if($retVal !== 0) {
//							$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'updating time failed');
//							break;
//                    }
                } else {
                    //Disable NTP start up service
                    exec("sudo /usr/local/sbin/setServiceStartup.sh ntpdate disabled", $output, $retVal);
                    if($retVal !== 0) {
						
							$this->logObj->LogParameters(get_class($this), __FUNCTION__, $output);
							$this->logObj->LogParameters(get_class($this), __FUNCTION__, $retVal);
							$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'disabling NTP startup service failed');
							break;
                    }
                    array_push($restoreList, "/usr/local/sbin/setServiceStartup.sh ntpdate enabled");
                    unset($output);
                    //Set time
					
					unset($output);						
					exec("sudo date -s@\"{$changes["datetime"]}\"", $output, $retVal);
                    if($retVal !== 0) {
							$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'setting time failed');
							break;
                    }
                }
            }

			return 'SUCCESS';
        } while(FALSE);

        //Restore on error
        foreach($restoreList as $restoreItem){
            exec("$restoreItem");
        }
        return 'SERVER_ERROR';
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
