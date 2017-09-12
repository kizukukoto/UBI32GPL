<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('alertConfiguration.php');
require_once('alertlogreader.inc');
require_once('mailClient.php');
require_once('logMessages.php');

class Alert_notify{
	var $logObj;

    function Alert_notify(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
        header("HTTP/1.0 405 Method Not Allowed");
	}
	
	function isLocalHost($host_address){
    	if(($host_address == '127.0.0.1') || 
    		($host_address == 'localhost')){
    		return true;		
    	}
    	return false;
	}
	
    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
    	// Handle request from local host.
    	// Get the Box IP address and to list to make sure we allow only local requests.
    	if($this->isLocalHost($_SERVER['REMOTE_ADDR'])){
			// Get alert config
    		$alertConfigObj = new AlertConfiguration();
	        $alertConfig = $alertConfigObj->getConfig();
				
	        // Read alerts from the last notified time
        	$lastNotifiedTime = $alertConfigObj->getLastNotifiedTime();
        	$alrObj = new AlertLogReader();
    		$alerts = $alrObj->getAlertsNewerThan($lastNotifiedTime, true);
			
    		if(count($alerts) > 0){
	    		// Send email notification.
	    		$mailClient = new MailClient();
	    		$emailStatus = $mailClient->sendEmail($alertConfig, $alerts);
	    		
		        header("Content-Type: application/xml");
		        $xml = new NasXmlWriter();
		        $xml->push('alert_notify');
	    		
	    		if($emailStatus == true){
	    			//$alertConfig['last_notified_time'] = time();
	    			//$alertConfigObj->modifyConfig($alertConfig);
	    			$alertConfigObj->updateLastNotifiedTime(time());
	    			$xml->element('alert_notify_status', "Success");
	    		}else{
	    			$xml->element('alert_notify_status', "Fail");
	    		}
    		}else{
    			$xml->element('alert_notify_status', "Success");
    		}
			$xml->pop();
			echo $xml->getXml();
			
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');

    	}else{
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'Unauthorized');
			header("HTTP/1.0 401 Unauthorized");
            return;
    	}
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
    	header("HTTP/1.0 405 Method Not Allowed");
    }
    
    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
    	header("HTTP/1.0 405 Method Not Allowed");
    }    
}
?>
