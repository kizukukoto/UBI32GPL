<?php
/**
 * Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
 * This is the global config library for retrieving INI file settings. 
 * 
 * Each module should have its own section in the INI file (e.g. [ALBUMS]) and should retreive the
 * module-specific settings using its section name. INI settings are returned as associative arrays
 * (see http://php.net/manual/en/function.parse-ini-file.php)
 */

  require_once("util.inc");
  
  define('RETRY_COUNT', 5); // count of read, write time   

  if (!isset($_GLOBALS["GLOBALCONFIGPATH"])) {
      $GLOBALS["GLOBALCONFIGPATH"] = "globalconfig.ini";
  }

  if (!isset($_GLOBALS["DYNAMICCONFIGPATH"])) {
      $GLOBALS["DYNAMICCONFIGPATH"] = "%DYNAMIC_CONFIG%/dynamicconfig.ini";
  }
  
  if (!isset($_GLOBALS["UPNPCONFIGPATH"])) {
      $GLOBALS["UPNPCONFIGPATH"] = "upnpstatus.conf";
  }

  
  /**
   * To ensure we can read associatve arrays from a PHP INI file with PHP 5.2 and earlier, we
   * are using a format that can be parsed in 5.2 and earlier, example:
   * 
   * AUDIO_MIMETYPES[]="mp3:audio/mp3" will get converted to UDIO_MIMETYPES["mp3"] = "audio/mp3"
   * by this function
   * 
   * @param unknown_type $config
   * @return unknown_type
   */
  
  function readAssocArrays($config)
  {
      $newConfig = array();
      foreach ($config as $module=>$moduleConfig) {
          $newModuleConfig = array();
          foreach ($moduleConfig as $name=>$value) {
                if (is_array($value)) {
                    $arrayVal = array();
                    foreach ($value as $arrayEle) {
                        if (($pos = strpos($arrayEle, ":")) !== false) {
                            $var = substr($arrayEle, 0, $pos);
                            $val = substr($arrayEle, $pos+1);
                            $arrayVal[$var]=$val;
                        } else {
                            array_push($arrayVal, $arrayEle);
                        }
                    }
                    $newModuleConfig[$name] = $arrayVal;
                } else {
                    $newModuleConfig[$name]=$value;
                }                
          }
          $newConfig[$module] = $newModuleConfig; 
      }
      return $newConfig;
  }
  
  /**
   * To ensure we can read associative arrays from a PHP INI file with PHP 5.2 and earlier, we
   * are using a format that can be parsed in 5.2 and earlier, example:
   * 
   * AUDIO_MIMETYPES["mp3"] = "audio/mp3" will get converted to: 
   * AUDIO_MIMETYPES[]="mp3:audio/mp3" by this function
   * 
   * @param unknown_type $globalConfig
   * @return unknown_type
   */
  
  function writeAssocArrays($config)
  {
      $newConfig = array();
      foreach ($config as $module=>$moduleConfig) {
          $newModuleConfig = array();
          foreach ($moduleConfig as $name=>$value) {
                if (is_array($value)) {
                    $keys = array_keys($value);
                    $arrayVal = array();
                    foreach ($keys as $key)
                    {
                        if (!is_int($key)) {
                            //key is not array index, therefore associative key
                            $value[$key] = $key . ':' . $value[$key];
                        }
                        array_push($arrayVal, $value[$key]);
                    }
                    $newModuleConfig[$name] = $arrayVal;
                }
                else {
                    $newModuleConfig[$name]=$value;
                }
          }
          $newConfig[$module] = $newModuleConfig; 
      }
      return $newConfig;
  }
  
  function parseConfigFilePath($filePath, $defaultConfigPath) {
  	$pos1 = strpos($filePath, "%");
  	if ($pos1 === FALSE) {
  		//no substitution, use default config path
  		return $defaultConfigPath . $filePath;
  	}
  	$pos2 = strpos($filePath, "%", $pos1+1);
  	if ($pos2 === FALSE || ($pos2-$pos1 <= 1)) {
  		//no substitution, single ocurrence of '%', use default config path
  		return $defaultConfigPath . $filePath;
  	}
  	$configKey = substr($filePath, $pos1+1,$pos2-$pos1-1);
	$searchVal = substr($filePath, $pos1,$pos2-$pos1+1);
  	$configGlobal = getGlobalConfig("global");
	$configVal = $configGlobal[$configKey];
	if ( isset($configVal) ) {
		$filePath = str_replace($searchVal, $configVal, $filePath );
	}
  	return $filePath;
  }
  
  function getConfigFilePath($configId)
  {
	if ( (isset($_SERVER["WINDIR"]) && (stripos($_SERVER["WINDIR"], "windows") !== false)) ||
	     (isset($_SERVER["windir"]) && (stripos($_SERVER["windir"], "windows") !== false)) 
	) {
  		//we are running on windows
  		$defaultConfigPath = getWebAppPath() . "/config/Windows/"; 
  	}
  	else {
  		//'tis Linux
  		$defaultConfigPath = getWebAppPath() . "/config/"; 
  	}
	$globalConfigPath = !empty($GLOBALS[$configId . "PATH"]) ? $GLOBALS[$configId . "PATH"] : 'globalconfig.ini';
	$configFilePath = parseConfigFilePath($globalConfigPath, $defaultConfigPath);

  	if (!isset($configFilePath)) {
  	    //echo("Config file path is not defined for Path ID: " + $configId);
		return false;
  	}
  	return  $configFilePath;
  }
    
  function getTmpConfigFilePath($configId) {
  	//we only need to do this for dynamicconfig,ini, so this is hard-coded for now
  	if ($configId == "DYNAMICCONFIG") {
	  	$config = getConfig("GLOBALCONFIG", "global");
  		return $config["DYNAMIC_CONFIG_TMP"];
  	}
  	return NULL;
  }
  
  /* Read config ini file
   * CAUTION: apc function is different between php 5.3 and 5.2.
   * */
  function readConfigFile($configId) {
  	$configSettings = array();
  	$configFilePath = getConfigFilePath($configId);

  	   	  			
  	$isExit     = true;
  	$countRetry = 0;
    $readData   = null;
  	do {
  		//check APC first
  		if($configId == 'DYNAMICCONFIG') {
  			$readData = apc_fetch($configId);
  			if(version_compare(PHP_VERSION, '5.3.0') < 0) { //PHP < 5.3	
  				$readData = $readData[0];
  			}	 
  		}
  		if(!empty($readData)) { 
	  		$configSettings = parse_ini_string_apc($readData, true); 		
	  		$configSettings = readAssocArrays($configSettings);
	  		$GLOBALS[$configId] = $configSettings;	

	  		return true;  			
  		} else if (isset($configFilePath) && file_exists($configFilePath) && !is_dir($configFilePath)) {  	
	  		$configSettings = parse_ini_file($configFilePath, true);
	  		if(!empty($configSettings)) {
	  		    $configSettings = readAssocArrays($configSettings);
 		    
	  		    $GLOBALS[$configId] = $configSettings;
				$isExit = false;
	  			return true;
	  		} else {
	  			usleep(200);
	  			$countRetry++;
	  		}
	  	} else {
	  		usleep(500);
	  		$countRetry++;
	  	}
  	} while($isExit && $countRetry < RETRY_COUNT);  	
  		
  	return false;
  }
  

  /* Write config ini file
   * 
   * */
  function writeConfigFile($configId) {
    if (isset($GLOBALS[$configId])) {
        $cr = PHP_EOL;
    	
        $configFilePath    = getConfigFilePath($configId);        
  	    $tmpConfigFilePath = getTmpConfigFilePath($configId);

  	    $isExistTmpFile = file_exists($tmpConfigFilePath) ? true : false;

  	    $content      = ""; // buffer of config/dynamicconfig.ini
  	    $contentTmp   = ""; // buffer of /tmp/dynamicconfig.ini
  	    
  	    $globalConfig = $GLOBALS[$configId];
  	    $globalConfig = writeAssocArrays($globalConfig);
  	    foreach ($globalConfig as $module=>$moduleConfig) {
  	        $content .= "[". $module . "]"  . $cr;
  	        
  	        foreach ($moduleConfig as $name=>$value) {
                if (!is_array($value)) {
                    $content .= $name . "=\"" . $value . "\""  . $cr;
                    if ($isExistTmpFile) {
                    	$contentTmp .= $name . "=\"" . $value . "\""  . $cr;
                    }
                } else {
                    foreach ($value as $arrayEle) {
                        $content .= $name . "[]=\"" . $arrayEle . "\"" . $cr;
                    	if ($isExistTmpFile) { 
                    		$contentTmp .= $name . "[]=\"" . $arrayEle . "\"" . $cr; 
                    	}
                	}
                }
  	        }
  	    }
  	    
  	    $isExit     = true;
  	    $countRetry = 0;    
  	    do {
	  	    if(file_put_contents($configFilePath, $content, LOCK_EX)) {
	  	    	if($configId == 'DYNAMICCONFIG') { 
	  	    		if(version_compare(PHP_VERSION, '5.3.0') >= 0) { //PHP >= 5.3			
	  	    			apc_store($configId, $content);
	  	    		} else {	  	    			
	  	    			apc_store($configId, array($content));
	  	    		}
	  	    		file_put_contents($configFilePath . "_safe", $content, LOCK_EX);
	  	    	}				  	    	
	  	    	$isExit = false;
	  	    } else {	   	
	  	    	$countRetry++;
	  	    	usleep(200);  	
	  	    }	  	    
  	    } while($isExit && $countRetry < RETRY_COUNT); 

		$isExit     = true;
  	    $countRetry = 0;
  	    do {
	  	    if (!empty($contentTmp) && file_put_contents($tmpConfigFilePath, $contentTmp, LOCK_EX)) { 
	  	    	$isExit = false;
	  	    } else {
				$countRetry++;
	  	    	usleep(200);
	  	    }  
	  	    	
  	    } while($isExit && $countRetry < RETRY_COUNT);  
    }
  	return true;
  }


    // added to restore config file from safe_copy
	function restoreConfig($configId) {
		$configFilePath = getConfigFilePath($configId);
	  	$safeCopyPath = $configFilePath . "_safe";
	  	
		if(!file_exists($safeCopyPath)) {
	  		return false;	  		
	  	}	  	
	  	
	  	$safeSettings = parse_ini_file($safeCopyPath, true);
	  	
	  	//check that we have all of the settings from the safe copy and it
	  	//is not corrupted
	  	$totalSafeSettings = isset($safeSettings['config']['TOTAL_SETTINGS']) ? $safeSettings['config']['TOTAL_SETTINGS'] : null;
	  	if (empty($totalSafeSettings) || (sizeof($safeSettings["config"]) != $totalSafeSettings )) {
	  		//echo("Unable to restore: " . $configId . ", safe copy is truncated<br />");
	  		return false;	  		
	  	} else {	  		
	  		return copy($safeCopyPath, $configFilePath);
	  	}
	}
  
  
  function getConfig($configId, $section) {
  	if (!isset($GLOBALS[$configId])) {
  		readConfigFile($configId); 
  		//If not, copy safe copy to live copy as safe copy should have the latest edits
  		if ($configId == "DYNAMICCONFIG") {  			
  			$configSettings = isset($GLOBALS["DYNAMICCONFIG"]["config"]) ? $GLOBALS["DYNAMICCONFIG"]["config"] : null;
  			$totalSettings  = isset($configSettings['TOTAL_SETTINGS'])   ? $configSettings['TOTAL_SETTINGS']   : null;
  			if (!isset($totalSettings) || (sizeof($GLOBALS["DYNAMICCONFIG"]["config"]) != $totalSettings )) {
  				//try restoring missing settings from local safe copy
  			  	if (!restoreConfig($configId)) {
  					return false;  					
  					//echo("Dynamic Config file was truncated, failed to restore it from safe copy");
  				}
  			}
  		}
  	}
  	
  	$globalConfig = $GLOBALS[$configId];
  	if (isset($globalConfig[$section])) {
  		return($globalConfig[$section]);
  	}
  	
  	//trace error here
  	return NULL;
  }
  
  
  
  function setConfig($configId, $section, $name, $value)
  {
  	$config = getConfig($configId, $section);
  	//for now and for security, we are only going to allow values 
	//to be set for parameters that are already defined in 
	//globalconfig.ini
	
  	//JS - allow values to be written even if they were not read from teh config file
  	//this makes the config file self-repairing instead of propogating a damaged file
  	//if (isset($GLOBALS[$configId][$section][$name])) {

  	$GLOBALS[$configId][$section][$name] = $value;
  	writeConfigFile($configId);
  	return TRUE;	
  	//}
  	//return FALSE;
  }
  
  
  /* Array version of setConfig to write all changing in one time
   * 
   * */
  function setConfigArray($changedArray) {
  	$configId = $changedArray['configId'];
  	$section  = $changedArray['section'];
  	
  	$config = getConfig($configId, $section);

  	//for now and for security, we are only going to allow values
	//to be set for parameters that are already defined in
	//globalconfig.ini
	foreach($changedArray['name'] as $key => $val) {
		$GLOBALS[$configId][$section][$key] = $val;			
	}
  	
  	writeConfigFile($configId);
  	return TRUE;
  } 
  
  
  
  //convenience functions
  
  function getGlobalConfig($section) {
      return getConfig("GLOBALCONFIG", $section);
  }
  
  function setGlobalConfig($section, $name, $value) {
      return setConfig("GLOBALCONFIG", $section, $name, $value);
  }
  
  function getUpnpStatus($section) {
      return getConfig("DYNAMICCONFIG", $section);
  }
  
  function setUpnpStatus($name, $value) {
      return setConfig("DYNAMICCONFIG", "config", $name, $value);
  }
  
  function getDeviceId() {
  	$config = getConfig("DYNAMICCONFIG", "config");
  	return $config["DEVICEID"];
  }
  
  function getDeviceType() {
  	$config = getConfig("GLOBALCONFIG", "global");
  	return $config["DEVICETYPE"];
  }
  
  function setDeviceId($deviceId) {
      return setConfig("DYNAMICCONFIG", "config", "DEVICEID", $deviceId);
  }
  
  function getDeviceAuthCode() {
  	$config = getConfig("DYNAMICCONFIG", "config");
  	return $config["DEVICEAUTH"];
  }
  
  function setDeviceAuthCode($authCode) {
      return setConfig("DYNAMICCONFIG", "config", "DEVICEAUTH", $authCode);
  }
  
  function getCommunicationStatus() {
  	$config = getConfig("DYNAMICCONFIG", "config");
	if( empty($config["DEVICEID"]) || empty($config["DEVICEAUTH"])) {
		return 'disabled';
	}     	
  	return $config["COMMUNICATION_STATUS"];
  }
  
  
  function getCentralServerHost() {
  	$portalHost = "www.wd2go.com";

  	$dynamicConfig = getUpnpStatus("config");
  	if (isset($dynamicConfig)) {
  		$portalUrl = $dynamicConfig["SERVER_BASE_URL"];
  		if (isset($portalUrl)) {
  			$portalHost = parse_url($portalUrl,PHP_URL_HOST);
  		}
  	}

  	return $portalHost;
  }
  
  function getCentralServerProtocol() {
  	$portalProtocol = "https://";

  	$dynamicConfig = getUpnpStatus("config");
  	if (isset($dynamicConfig)) {
  		$portalUrl = $dynamicConfig["SERVER_BASE_URL"];
  		if (isset($portalUrl)) {
  			$portalProtocol = parse_url($portalUrl,PHP_URL_SCHEME)."://";
  		}
  	}

  	return $portalProtocol;
  }


  
  function getIPAddresesAndPorts() {
  	$config = getConfig("DYNAMICCONFIG", "config");
  	$ipsAndPorts = array("EXTERNAL_IP"   => $config["EXTERNAL_IP"], 
  						 "INTERNAL_IP"   => $config["INTERNAL_IP"],
  						 "EXTERNAL_PORT" => $config["EXTERNAL_PORT"],
  						 "INTERNAL_PORT" => $config["INTERNAL_PORT"]);
  	return $ipsAndPorts;
  }
  
  function isPortForwardedConnection() {
  	$config = getConfig("UPNPCONFIG", "config");
  	return (strcasecmp($config["COMMUNICATION_STATUS"],"portforwarded") == 0);
  }

  function isRelayConnection() { 
  	$config = getConfig("UPNPCONFIG", "config");
  	return (strcasecmp($config["COMMUNICATION_STATUS"],"relayed") == 0);
  }
    
  function getRemoteAccess() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["REMOTEACCESS"];
  }
 
  function setRemoteAccess($remoteAccess) {
  	 if ( ( strcasecmp($remoteAccess,"TRUE") == 0) || (strcasecmp($remoteAccess, "FALSE") == 0)) {
	     return setConfig("DYNAMICCONFIG", "config", "REMOTEACCESS", strtoupper($remoteAccess));
  	 } 
  	 else {
  	 	return false;
  	 }
  }

  function getDefaultPortsOnly() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["DEFAULT_PORTS_ONLY"];
  }
 
  function setDefaultPortsOnly($defaultPortsOnly) {
  	 if ( ( strcasecmp($defaultPortsOnly,"TRUE") == 0) || (strcasecmp($defaultPortsOnly, "FALSE") == 0)) {
	     return setConfig("DYNAMICCONFIG", "config", "DEFAULT_PORTS_ONLY", strtoupper($defaultPortsOnly));
  	 }else {
  	 	return false;
  	 }
  }
  
  function setManualPortForwardConfig($manualPortForward, $manualExternalHttpPort, $manualExternalHttpsPort){
	$changedArr = array( 'configId' => 'DYNAMICCONFIG',
						 'section'  => 'config',
						 'name'     => array('DEFAULT_PORTS_ONLY' => 'FALSE',
						 				   'MANUAL_PORT_FORWARD'        => strtoupper($manualPortForward),
											 'MANUAL_EXTERNAL_HTTP_PORT'  => $manualExternalHttpPort,
											 'MANUAL_EXTERNAL_HTTPS_PORT' => $manualExternalHttpsPort));
	
  	setConfigArray($changedArr);
  }

  function clearManualPortForwardConfig(){
	$changedArr = array( 'configId' => 'DYNAMICCONFIG',
						 'section'  => 'config',
						 'name'     => array('MANUAL_PORT_FORWARD'        => 'FALSE',
											 'MANUAL_EXTERNAL_ROUTER_IP'  => '',
											 'MANUAL_EXTERNAL_HTTP_PORT'  => '',
											 'MANUAL_EXTERNAL_HTTPS_PORT' => ''));
	setConfigArray($changedArr);
  }

  function setDeviceRegistrationConfig($deviceId, $deviceAuth, $serverBaseUrl) {
	$changedArr = array( 'configId' => 'DYNAMICCONFIG',
						 'section'  => 'config',
						 'name'     => array('DEVICEID'			 => $deviceId,
											 'DEVICEAUTH' 		 => $deviceAuth,
											 'SERVER_BASE_URL'   => $serverBaseUrl));

	setConfigArray($changedArr);

  }  
  
  function getManualportForward() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["MANUAL_PORT_FORWARD"];
  }

  function getManualExternalRouterIp() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["MANUAL_EXTERNAL_ROUTER_IP"];
  }

  function getManualExternalHttpPort() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["MANUAL_EXTERNAL_HTTP_PORT"];
  }
  
  function getManualExternalHttpsPort() {
	$config = getConfig("DYNAMICCONFIG", "config");
    return $config["MANUAL_EXTERNAL_HTTPS_PORT"];
  }
  	
  /*
 * This function returns list of media crawler voulmes.
 * Each entry in the list contains volume name, volume path, media crawler db path.
 */
	function getMediaVolumesInfo(){
		$volInfo = array();
		$dbConfig = getGlobalConfig("db");
		$mediaCrawlerDbDir = "";
		if(isset($dbConfig["MEDIA_CRAWLER_DB_DIR_NAME"])){
			$mediaCrawlerDbDir = $dbConfig["MEDIA_CRAWLER_DB_DIR_NAME"];
		}
		
		if(isset($dbConfig["MEDIA_CRAWLER_HOST_FILE_PATH"])){
			$mediaHosts = simplexml_load_file($dbConfig["MEDIA_CRAWLER_HOST_FILE_PATH"]);
			foreach($mediaHosts->Volumes->Volume as $key => $vol){
				$mediaCrawlerDbPath = (string)$vol->Root.$mediaCrawlerDbDir.'/mediacrawler.db';
				$volInfo[]= array("Name"=>(string)$vol->Name, 
									"Path"=>(string)$vol->Root,
									"DbPath" => $mediaCrawlerDbPath);
			}	
		}
		return $volInfo;
	}

	function getMediaCrawlerDirName(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["MEDIA_CRAWLER_DB_DIR_NAME"])){
			return $dbConfig["MEDIA_CRAWLER_DB_DIR_NAME"];
		}
		return null;
	}

	function getMediaCrawlerHostFilePath(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["MEDIA_CRAWLER_HOST_FILE_PATH"])){
			return $dbConfig["MEDIA_CRAWLER_HOST_FILE_PATH"];
		}
		return null;
	}
	
	function getMediaCrawlerPath(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["MEDIA_CRAWLER_PATH"])){
			return $dbConfig["MEDIA_CRAWLER_PATH"];
		}
		return null;
	}
	
	
	function getFileInfoColumns(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["FILE_INFO_COLUMNS"])){
			return $dbConfig["FILE_INFO_COLUMNS"];
		}
		return null;	
	}
	
	function getDirInfoColumns(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["DIR_INFO_COLUMNS"])){
			return $dbConfig["DIR_INFO_COLUMNS"];
		}
		return null;	
	}
	
	function getSharePath(){
		$dbConfig = getGlobalConfig("directories");
		if(isset($dbConfig["SHARES_PATH"])){
			return $dbConfig["SHARES_PATH"];
		}
		return null;	
	}

	function getMioCrawlerXcodeIniPath(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["MIO_CRAWLER_XCODE_INI_FILE_PATH"])){
			return $dbConfig["MIO_CRAWLER_XCODE_INI_FILE_PATH"];
		}
		return null;	
	}
	
	function getMioCrawlerPath(){
		$dbConfig = getGlobalConfig("db");
		if(isset($dbConfig["MIO_CRAWLER_PATH"])){
			return $dbConfig["MIO_CRAWLER_PATH"];
		}
		return null;
	}
	
	function getServerBaseUrl(){
		$serverUrl = apc_fetch('SERVER_BASE_URL');
		if(!empty($serverUrl)){
			return $serverUrl;
		}
		
		$globalConfig = getGlobalConfig('global');
		$srvUrl = parse_url($globalConfig["SERVER_BASE_URL"]);
		$srvUrl['host'] = gethostbyname($srvUrl['host']);
		$updateUrl = $srvUrl["scheme"]."://".$srvUrl['host'];
		apc_store('SERVER_BASE_URL', $updateUrl);
		return $updateUrl;
	}
			
	/* This will be enabled function in PHP < 5.3
	 *
	 */				
    function parse_ini_string_apc($str, $ProcessSections=false) {
    	if(version_compare(PHP_VERSION, '5.3.0') >= 0) { //PHP >= 5.3
    		return(parse_ini_string($str, $ProcessSections));
    	}
    	
        $lines  = explode(PHP_EOL, $str);
        $return = array();
        $inSect = false;
        
        $lines = preg_replace('/"/i', '', $lines);
        foreach($lines as $line){
            $line = trim($line);
            if(!$line || $line[0] == "#" || $line[0] == ";")
                continue;
            if($line[0] == "[" && $endIdx = strpos($line, "]")){
                $inSect = substr($line, 1, $endIdx-1);
                continue;
            }
            if(!strpos($line, '='))
                continue;
           
            $tmp = explode("=", $line, 2);
            if($ProcessSections && $inSect)
                $return[$inSect][trim($tmp[0])] = ltrim($tmp[1]);
            else
                $return[trim($tmp[0])] = ltrim($tmp[1]);
        }
        return $return;
    }	
			
	
  //test code
  //$deviceId = getDeviceId();
  //var_dump($deviceId);
  //$filePath = getConfigFilePath("DYNAMICCONFIG");
  //$filepath = getConfigFilePath("GLOBALCONFIG"); 
  //$settings = getGlobalConfig("albums");
  //var_dump($settings);
  //$settings = getUpnpStatus("config");
  //var_dump($settings); 
  //$settings =  getConfig("DYNAMICCONFIG", "config");
  
  
  ?>