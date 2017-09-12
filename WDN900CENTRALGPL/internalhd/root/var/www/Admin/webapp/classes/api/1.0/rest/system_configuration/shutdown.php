<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('power.php');
require_once('logMessages.php');

class Shutdown{

    function Shutdown(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: PUT");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        parse_str(file_get_contents("php://input"), $changes);

        $shutDownObj = new Power();
        $result = $shutDownObj->modifyState($changes);

        switch($result){
        case 'SUCCESS':
            syslog(LOG_NOTICE, "Powering off");
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
        header("Allow: PUT");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: PUT");
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
