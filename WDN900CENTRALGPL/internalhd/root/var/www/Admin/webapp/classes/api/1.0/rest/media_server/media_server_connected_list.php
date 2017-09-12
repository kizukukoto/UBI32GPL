<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('mediaServerConnectedList.php');
require_once('logMessages.php');

class Media_server_connected_list{
	var $logObj;

    function Media_server_connected_list(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $connectedListObj = new MediaServerConnectedList();
        $result = $connectedListObj->getConnectedList();

        if($result !== NULL){
			
			
            $xml = new NasXmlWriter();
            $xml->push('media_server_connected_list');
			foreach($result as $device){
                $xml->push('device');
                $xml->element('mac_address', $device['mac_address']);
                $xml->element('ip_address', $device['ip_address']);
				$xml->element('friendly_name', $device['friendly_name']);
				$xml->element('device_description', $device['device_description']);
				$xml->element('device_enable', $device['device_enable']);
                $xml->pop();
            }
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

		$connectedListObj = new MediaServerConnectedList();
		$result = $connectedListObj->modifyMediaDevice($changes);

		switch($result){
			case 'SUCCESS':
				break;
			case 'BAD_REQUEST':
				header("HTTP/1.0 400 Bad Request");
				break;
			case 'DEVICE_NOT_FOUND':
				header("HTTP/1.0 404 Not Found");
				break;
			case 'SERVER_ERROR':
				header("HTTP/1.0 500 Internal Server Error");
				break;
		}
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
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
