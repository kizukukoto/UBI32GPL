<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('deviceRegistration.php');
require_once('logMessages.php');

class Device_registration{

	var $logObj;
	
    function Device_registration(){
		$this->logObj = new LogMessages();
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Content-Type: application/xml");
        
        $deviceReg = new DeviceRegistration();
        $result = $deviceReg->getDeviceRegistration();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('device_registration');
            $xml->element('registered', $result['registered']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Resource (EULA Acceptance) not created yet
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'NOT_FOUND');
			header("HTTP/1.0 404 Not Found");
        }
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
		$deviceReg = new DeviceRegistration();			
		$result = $deviceReg->register($_POST);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $_POST);
		
        switch($result){
        case 'SUCCESS':
            header("HTTP/1.0 200 Success");
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
        header("Allow: GET, POST");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET, POST");
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
