<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('systemFactoryRestore.php');
require_once('logMessages.php');

class System_factory_restore{
	var $logObj;

    function System_factory_restore(){
		$this->logObj = new LogMessages();
	}


    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
//        if(!authenticateAsOwner($queryParams))
//        {
//            header("HTTP/1.0 401 Unauthorized");
//            return;
//        }

        header("Content-Type: application/xml");
        
        $sysRestoreObj = new SystemFactoryRestore();
        $result = $sysRestoreObj->getStaus();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('system_factory_restore');
			$xml->element('percent', $result['completion_percent']);
            $xml->element('status', $result['status']);
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
		
		header("Allow: GET POST");
		header("HTTP/1.0 405 Method Not Allowed");

    }

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
//        if(!authenticateAsOwner($queryParams))
//        {
//            header("HTTP/1.0 401 Unauthorized");
//            return;
//        }

		parse_str(file_get_contents("php://input"), $changes);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

		$sysRestoreObj = new SystemFactoryRestore();
		$result = $sysRestoreObj->restore($changes); 

        switch($result){
        case 'SUCCESS':
            header("HTTP/1.0 201 Created");
            break;
        case 'BAD_REQUEST':
            header("HTTP/1.0 400 Bad Request");
            break;
        case 'SERVER_ERROR':
        default:
            header("HTTP/1.0 500 Internal Server Error");
            break;
        }
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
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
