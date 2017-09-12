<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('systemInformation.php');
require_once('logMessages.php');

class System_information{
	var $logObj;

    function System_information(){
		$this->logObj = new LogMessages();
	}


    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Content-Type: application/xml");
        
        $infoObj = new SystemInformation();
        $result = $infoObj->getInfo();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('system_information');
            $xml->element('manufacturer', $result['manufacturer']);
            $xml->element('manufacturer_url', $result['manufacturer_url']);
            $xml->element('model_description', $result['model_description']);
            $xml->element('model_name',  $result['model_name']);
            $xml->element('model_url',  $result['model_url']);
            $xml->element('model_number',  $result['model_number']);
            $xml->element('capacity',  $result['capacity']);
            $xml->element('serial_number', $result['serial_number']);
            $xml->element('mac_address',  $result['mac_address']);
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
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET");
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
