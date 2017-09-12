<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('alertConfiguration.php');
require_once('mailClient.php');
require_once('logMessages.php');

class Alert_configuration_test{
	var $logObj;

    function Alert_configuration_test(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
    	header("Allow: POST");
        header("HTTP/1.0 405 Method Not Allowed");
    }
    
    function sendTestEmail($from, $to, $xml, $mailClient, $index){
		// Send test email    	
    	if(isset($to) && strcmp($to,"") != 0){
			$emailStatus = $mailClient->sendTestEmail($from, $to);
			$statusStr = "Fail";
			if($emailStatus == true){
				$statusStr = "Success";
			}
			$statusArr_0 = array("Status"=>$statusStr);
			$xml->element('email_recipient_'.$index, $to, $statusArr_0);
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $statusStr);
		}
	}
   	
    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        // Send email notification.
    	$mailClient = new MailClient();

        header("Content-Type: application/xml");
        $xml = new NasXmlWriter();
        $xml->push('alert_configuration_test');
        
    	$alertConfigObj = new AlertConfiguration();
        $alertConfigObj = $alertConfigObj->getConfig();
        
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $alertConfigObj);

		$this->sendTestEmail($alertConfigObj['email_returnpath'], 
        				$alertConfigObj['email_recipient_0'], $xml, $mailClient, 0);
        $this->sendTestEmail($alertConfigObj['email_returnpath'], 
        				$alertConfigObj['email_recipient_1'], $xml, $mailClient, 1);
        $this->sendTestEmail($alertConfigObj['email_returnpath'], 
        				$alertConfigObj['email_recipient_2'], $xml, $mailClient, 2);
        $this->sendTestEmail($alertConfigObj['email_returnpath'], 
        				$alertConfigObj['email_recipient_3'], $xml, $mailClient, 3);
        $this->sendTestEmail($alertConfigObj['email_returnpath'], 
        				$alertConfigObj['email_recipient_4'], $xml, $mailClient, 4);
        				
		$xml->pop();
		echo $xml->getXml();
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
