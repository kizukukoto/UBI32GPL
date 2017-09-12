<?php

class FirmwareInfo{

	function FirmwareInfo() {
    }
    
	function getAvailPackages(){
		//!!!This where we gather up response  
		//!!!Return NULL on error
			
		$current = array();
		
		$update = array();
	    $upgrades = array();
		$available = '';
/*
        $useShellScriptForUpdateInfo = false;
		if (SKIP_SHELL_SCRIPT)
		{
    		unset($output);
            $output = @file_get_contents('/tmp/fw_update_info');
            if ($output === false)
            {
                $useShellScriptForUpdateInfo = true;
            }
            else 
            {
                $output = explode("\n", $output);
            }
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForUpdateInfo)
        {
            unset($output);
            exec("sudo /usr/local/sbin/getNewFirmwareAvailable.sh", $output, $retVal);
		    if($retVal !== 0) 
		    {
			    return NULL;
		    }
        }
		if (strcasecmp(trim($output[0],'"'),"no upgrade") ===0)
		{
			
			$update = array(
				'available' => 'false', 'package' => array());
				
			array_push($update['package'], 
				array('name' => "", 'version' => "", 
						'description' => ""));

		}
		else if (strcasecmp(trim($output[0],'"'),"error") ===0)
		{
			
			$update = array(
				'available' => 'error', 'package' => array());
				
			array_push($update['package'], 
				array('name' => "", 'version' => "", 
						'description' => ""));

		}
		else
		{
			// if we've gotten to this point then there are updates available
			$update = array('available' => 'true', 'package' => array());
			if (!empty($output))
			{
    			foreach ($output as $package)
    			{
                    if (isset($package) && !empty($package))
                    {
        				$packageWithBlanks = explode('"',$package);
        
        				//strip the blanks and spaces from the device array
        				foreach ($packageWithBlanks as $key=>$value){
        					if ($value == " ") {
        						unset($packageWithBlanks[$key]);	
        					}	
        				}	
        				
        				// unset the leading blank string
        				unset($packageWithBlanks[0]);		
        				
        				$packageContents = array_values($packageWithBlanks);
        				
        
        				array_push($update['package'], 
        					array('name' => $packageContents[0], 'version' => $packageContents[1], 
        							'description' => $packageContents[2]));
                    }
    			}
			}
		}
*/
        $name = 'MyBookLive ';
		$sysConf = @parse_ini_file('/etc/system.conf');
		if (isset($sysConf['modelName']))
		{
			$name = $sysConf['modelName'];
			$name = $name . ' ';
		}
        $useShellScriptForPackage = false;
        $useShellScriptForUpgrade = false;
	    $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false)
        {
            $useShellScriptForPackage = true;
            $useShellScriptForUpgrade = true;
        }
        else
        {
            if (!isset($nasConfig['global']['VERSION_FILE']) || !isset($nasConfig['admin']['VERSION_BUILDTIME_FILE']) || !isset($nasConfig['admin']['VERSION_UPDATE_FILE']))
            {
                $useShellScriptForPackage = true;
            }
            if (!isset($nasConfig['admin']['UPGRADE_LINK']) || !isset($nasConfig['admin']['UPGRADE_INFO']))
            {
                $useShellScriptForUpgrade = true;
            }
        }
        
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForPackage)
        {
            $version = @file_get_contents($nasConfig['global']['VERSION_FILE']);
            if ($version === false)
            {
                $useShellScriptForPackage = true;
            }
            else
            {
                $version = trim($version);
                $description = 'Core F/W';
                if (file_exists($nasConfig['global']['SSH_LOGIN_TRIGGER']))
                {
                    $description = $nasConfig['global']['SSH_LOGIN_DESCRIPTION_STRING'];
                }
                $buildTime = @file_get_contents($nasConfig['admin']['VERSION_BUILDTIME_FILE']);
                if ($buildTime === false)
                {
                    $useShellScriptForPackage = true;
                }
                else 
                {
                    $buildTime = trim($buildTime);
                    $lastUpgradeTime = @file_get_contents($nasConfig['admin']['VERSION_UPDATE_FILE']);
                    if ($lastUpgradeTime === false)
                    {
                        $useShellScriptForPackage = true;
                    }
                    else 
                    {
                        $lastUpgradeTime = trim($lastUpgradeTime);
                        $current = array('package'=>array());
                        array_push($current['package'],
                            array('name'=>$name, 'version'=>$version,'description'=>$description, 
                            'package_build_time'=>$buildTime,'last_upgrade_time'=>$lastUpgradeTime));
                    }
                }
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForPackage)
        {
            unset($output);
            exec("sudo /usr/local/sbin/getCurrentFirmwareDesc.sh", $output, $retVal);
            if($retVal !== 0) 
            {
                return NULL;
            }
            
            $current = array('package' => array());
            foreach ($output as $package)
            {
                $currentPackageWithBlanks = explode('"',$package);
                
                //strip the blanks and spaces from the device array
                foreach ($currentPackageWithBlanks as $key=>$value){
                    if ($value == " ") {
                        unset($currentPackageWithBlanks[$key]);	
                    }	
                }	
                //unset the leading blank string
                unset($currentPackageWithBlanks[0]);
                
                $currentPackageContents = array_values($currentPackageWithBlanks);
                
                array_push($current['package'], 
                    array('name' => $currentPackageContents[0], 'version' => $currentPackageContents[1], 
                    	'description' => $currentPackageContents[2] , 
                    	'package_build_time'=>$currentPackageContents[3],
                    	'last_upgrade_time'=>$currentPackageContents[4] ));
            }
        }
        
    	// add new upgrade xml body format
		unset($upgradesOutput);
		$upgradesOutput = array();
		
		if (SKIP_SHELL_SCRIPT && !$useShellScriptForUpgrade)
		{
    		if (file_exists($nasConfig['admin']['UPGRADE_LINK']))
    		{
        		$delAbortedAttempts = @unlink($nasConfig['admin']['UPGRADE_LINK']);
        		if ($delAbortedAttempts === false)
        		{
        		    $useShellScriptForUpgrade = true;
        		}   
    		}
    		if (!$useShellScriptForUpgrade)
    		{
    		    $upgradesOutput = @file_get_contents($nasConfig['admin']['UPGRADE_INFO']);
        		if ($upgradesOutput === false)
        		{
        		    $useShellScriptForUpgrade = true;
        		}
        		else
        		{
        		    $upgradesOutput = explode("\n", $upgradesOutput);
        		}
    		}
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForUpgrade)
		{
		    unset($upgradesOutput);
		    exec("sudo /usr/local/sbin/getNewFirmwareUpgrade.sh", $upgradesOutput, $retVal);
    		if($retVal !== 0) 
    		{
    			//return NULL;
    		}
		}
				
		if (strcasecmp(trim($upgradesOutput[0],'"'),"no upgrade") ===0)
		{
		    $available = 'false';
		}
		else if (strcasecmp(trim($upgradesOutput[0],'"'),"error") ===0)
		{
		    $available = 'error';
		}
		else
		{
		    $available = 'true';
		}
		    
		if ($available != 'true')
		{
			// 3G response format (legacy)
			$update = 	array
						( 
						'available' => $available,
						'package' => array()
						);
						
			array_push ( $update['package'], array 
			                (
							'name' => "", 
							'version' => "", 
							'description' => ""
							)
						);
						
		    // 3.5G response format
			$upgrades = array
						(
						'available' => $available,
						'message' => "",
						'upgrade' => array()
						);
					
			array_push ( $upgrades['upgrade'], array 
			                    (
								'version' => "",
								'image' => "",
								'filesize' => "",
								'releasenotes' => "",
								'message' => "",
								'name' => "",
								'description' => ""
								)
						);
		}
		else
		{
			// there are upgrades available
			$name = $upgradesOutput[0];
		    $version = $upgradesOutput[1];
		    $description = $upgradesOutput[2];
		    $image = $upgradesOutput[3];
		    $message = $upgradesOutput[4];
		    $releasenotes = $upgradesOutput[5];
		    $filesize = $upgradesOutput[6];
		    
		    // get second package if it exists
   		    $name_1 = $upgradesOutput[7];
		    $version_1 = $upgradesOutput[8];
		    $description_1 = $upgradesOutput[9];
		    $image_1 = $upgradesOutput[10];
		    $message_1 = $upgradesOutput[11];
		    $releasenotes_1 = $upgradesOutput[12];
		    $filesize_1 = $upgradesOutput[13];

			// 3G response format (legacy)
			$update = array('available' => 'true', 'package' => array());
			
            array_push($update['package'], array 
                (
            	'name' => $name, 
            	'version' => $version, 
            	'description' => $description
                )
            );
					
		    // 3.5G response format
			$upgrades = array 
			        (
					'available' => 'true',
					'message' => "",
					'upgrade' => array()
					);

            // put a loop here if needed (currently only two packages are handled below)
            //			foreach ($output as $package)
            //			{
            //			}
    		array_push ( $upgrades['upgrade'], array 
    		                (
       						'name' => $name,
							'version' => $version,
    						'description' => $description,
							'image' => $image,
							'message' => $message,
							'releasenotes' => $releasenotes,
							'filesize' => $filesize
							)
    					);

    		if ( $name_1 == "apnc" ) {
        		array_push ( $upgrades['upgrade'], array 
        		            (
       						'name' => $name_1,
							'version' => $version_1,
    						'description' => $description_1,
							'image' => $image_1,
							'message' => $message_1,
							'releasenotes' => $releasenotes_1,
							'filesize' => $filesize_1
							)
						);		    
    		}
		}

		
		return array('current' => $current, 'update' => $update, 'upgrades' => $upgrades);       //$fullPackage;     //
		
	}
	function modifyUpdateAvailable(){
		
		//Actually do change
		unset($output);
		exec("sudo /usr/local/sbin/getNewFirmwareUpgrade.sh immediate", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}		

		return 'SUCCESS';
	}	
  }

/*
		    //// test code which uses hudson build server
    		unset($output);
    		exec("sudo /sbin/ifconfig | grep -A 2 eth0 | grep 'inet addr' | cut -d ':' -f 2 | cut -d ' ' -f 1", $output, $retVal);
    		if($retVal !== 0) {
    			$ipAddress = "localhost";
    		}
    		$ipAddress = $output[0];
    		//$imageLink = 'http://wdscnasbuild01.wdc.com:8080/job/PROJ-Apollo3_5g_dev/lastSuccessfulBuild/artifact/apnc-020018-344-20110605.deb';
    		////
*/    		
  
/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>
