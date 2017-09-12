<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('networkConfiguration.php');
require_once('logMessages.php');

class Network_configuration{
	var $logObj;

    function Network_configuration(){
		$this->logObj = new LogMessages();
	}


    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $netConfObj = new NetworkConfiguration();
        $result = $netConfObj->getNetworkConfig();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('network_configuration');
            $xml->element('proto', $result['proto']);
            $xml->element('ip', $result['ip']);
            $xml->element('netmask', $result['netmask']);
            $xml->element('gateway', $result['gateway']);
            $xml->element('dns0', $result['dns0']);
            $xml->element('dns1', $result['dns1']);
            $xml->element('dns2', $result['dns2']);
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

        $netConfObj = new NetworkConfiguration();
        $result = $netConfObj->modifyNetworkConfig($changes);
		
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
