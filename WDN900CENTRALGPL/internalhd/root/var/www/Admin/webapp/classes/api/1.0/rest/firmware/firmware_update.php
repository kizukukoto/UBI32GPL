<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('firmwareUpdate.php');
require_once('logMessages.php');

class Firmware_update{
	var $logObj;

    function Firmware_update(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){

        header("Content-Type: application/xml");
        
		$fwUpdateObj = new FirmwareUpdate();
        $result = $fwUpdateObj->getFirmwareUpdate();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('firmware_update');
			$xml->element('status', $result['status']);
			$xml->element('completion_percent', $result['completion_percent']);
			$xml->element('error_code', $result['error_code']);
			$xml->element('error_description', $result['error_description']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
            if($result['completion_percent'] != ''){
                $this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result['completion_percent'].'%');
            }
        } else {
            //Failed to collect info
            $this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SERVER_ERROR');
            header("HTTP/1.0 500 Internal Server Error");
        }
    }


    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }
        
        parse_str(file_get_contents("php://input"), $queryParams);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $queryParams);
		
		$fwUpdateObj = new FirmwareUpdate();
        $result = $fwUpdateObj->automaticFWUpdate($queryParams);

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
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

		parse_str(file_get_contents("php://input"), $queryParams);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $queryParams);

		$fwUpdateObj = new FirmwareUpdate();
		$result = $fwUpdateObj->manualFWUpdate($queryParams);
		
		//this is the way to receive files from command
//		$result = $fwUpdateObj->manualFWUpdate($_FILES);

        switch($result){
        case 'SUCCESS':
            header("HTTP/1.0 200 OK");
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


    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET, PUT, POST");
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
