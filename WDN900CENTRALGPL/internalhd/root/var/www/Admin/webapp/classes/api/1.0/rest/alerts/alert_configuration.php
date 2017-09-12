<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('alertConfiguration.php');
require_once('logMessages.php');

class Alert_configuration{
	var $logObj;

    function Alert_configuration(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        
        $alertConfigObj = new AlertConfiguration();
        $result = $alertConfigObj->getConfig();

        if($result !== NULL){
        	header("Content-Type: application/xml");
        	$xml = new NasXmlWriter();
            $xml->push('alert_configuration');
            $xml->element('email_enabled', $result['email_enabled']);
//            $xml->element('email_returnpath', $result['email_returnpath']);
            $xml->element('email_recipient_0', $result['email_recipient_0']);
            $xml->element('email_recipient_1', $result['email_recipient_1']);
            $xml->element('email_recipient_2', $result['email_recipient_2']);
            $xml->element('email_recipient_3', $result['email_recipient_3']);
            $xml->element('email_recipient_4', $result['email_recipient_4']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Failed to collect info
            header("HTTP/1.0 500 Internal Server Error");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SERVER_ERROR');
		}
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        parse_str(file_get_contents("php://input"), $changes);
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        $alertConfigObj = new AlertConfiguration();
        $result = $alertConfigObj->modifyConfig($changes);

        switch($result){
        case 'SUCCESS':
            break;
        case 'BAD_REQUEST':
            header("HTTP/1.0 400 Bad Request");
            break;
        case 'SERVER_ERROR':
            header("HTTP/1.0 500 Internal Server Error");
            break;
       }
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
	}

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET, PUT");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET, PUT");
        header("HTTP/1.0 405 Method Not Allowed");
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
