<?php

require_once('logMessages.php');

/**
 * Alert log reader class. The funciotns in the class will read the latest n alerts (tail the
 * log file) or the alerts that have been added to the log since a given timestamp.
 * 
 * @author Sapsford_J
 *
 */
	class AlertLogReader {
		var $logObj;

		private static $alertLogPath;
		
		/**
		 * Constructor, sets the alert log path for the O/S environment
		 */
		
		function AlertLogReader() { 

			require_once("alert.inc");
			require_once("tailfile.inc");
			
			$this->logObj = new LogMessages();

			//set the alert log path for the O/S environment we are running in 
			
		    if ( (isset($_SERVER["WINDIR"]) && (stripos($_SERVER["WINDIR"], "windows") !== false)) ||
					   (isset($_SERVER["windir"]) && (stripos($_SERVER["windir"], "windows") !== false)) ) {
				  //we are running on windows
				  self::$alertLogPath = "C:\\SVNBranches\\Nas3G\\REST\\webapp\\htdocs\\logs\\wdalerts.log";
			}
			else {
			  	  self::$alertLogPath = "/var/log/wdalerts.log";
			}
		}  
  
	  /**
	   * fn gets the last n alerts from, the alert log file
	   * @param int $alertLimit number of alerts to get
	   */
	  
	  function getLatestAlerts($alertLimit)
	  {
	  	$alerts = array();
	  	
	  	$alertLogTail = tailFile( self::$alertLogPath, $alertLimit );
	  	if ($alertLogTail !== null) {
		  	foreach($alertLogTail as $alertLogLine) {
		  		$alert = new Alert($alertLogLine);
		  		array_push($alerts, $alert);
		  	}
	  	}
	  	else {
				$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "Failed to open or read alert log file: " . self::$alertLogPath);
	  		
	  	}
	  	return $alerts;
	  }
	
	  
	  /**
	   * fn gets all of the alerts added to the alert log file since the given timestamp
	   * @param long $timestamp - unixtime
	   * @param latestFirst - set true for the latest alert to be the first element in the array
	   */
	  
	  function getAlertsNewerThan($sinceTime, $latestFirst = false)
	  {
	  	$start = 50;
	  	$increment = 0;
	  	$newAlerts = array();
	  	
	  	while( true ) {
	  		//loop until we find all the alerts > given time, or we have read the whole file 
	  		$alerts = $this->getLatestAlerts($start + $increment);
	  		if ($alerts[0]->timestamp <= $sinceTime) {
	  			//we have all alerts since given time
	  			break;
	  		}
	  		if (sizeof($alerts) < ($start + $increment)) {
	  			//we have read the whole file 
	  			break;
	  		}
	  		$increment += 50;
	  	}
	  	
	  	//find alerts where timestamp > $sinceTime
	  	
	  	forEach ($alerts as $alert) {
	  		if ($alert->timestamp > $sinceTime) {
	  			array_push($newAlerts, $alert);
	  		}
	  	}
	  	
	  	if ($latestFirst) {
	  		$newAlerts = array_reverse($newAlerts);
	  	}

	  return $newAlerts;
	  }
  }
  /*
   * test code
   */
  
  //$alertLogReader = new AlertLogReader();
  //$alertArray = $alertLogReader->getLatestAlerts(20);
  //$alertArray = $alertLogReader->getAlertsNewerThan(1267833013, true);
  //var_dump($alertArray);
  
	
?>
