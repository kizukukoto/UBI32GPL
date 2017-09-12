<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('itunesScan.php');
require_once('logMessages.php');

class Itunes_scan{

    function Itunes_scan(){
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

		parse_str(file_get_contents("php://input"), $changes);

		$logObj = new LogMessages();
		$logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        $itunesScanObj = new ItunesScan();
        $result = $itunesScanObj->scan($changes);

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
		
		$logObj->LogData('OUTPUT', get_class($this), __FUNCTION__, $result);
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
