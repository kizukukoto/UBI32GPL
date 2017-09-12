<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('itunesConfiguration.php');
require_once('logMessages.php');

class Itunes_configuration{
	var $logObj;

    function Itunes_configuration(){
		$this->logObj = new LogMessages();
	}


    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $itunesConfigObj = new ItunesConfiguration();
        $result = $itunesConfigObj->getConfig();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('itunes_configuration');
            $xml->element('enable_itunes_server', $result['enable_itunes_server']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Failed to collect info
			header("HTTP/1.0 400 Bad Request");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'BAD_REQUEST');
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

        $itunesConfigObj = new ItunesConfiguration();
        $result = $itunesConfigObj->modifyConfig($changes);

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
