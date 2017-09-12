<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('hddStandbyTime.php');
require_once('logMessages.php');

class Hdd_standby_time{
	var $logObj;

    function Hdd_standby_time(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $hddStanbyConfigObj = new HddStandbyTime();
        $result = $hddStanbyConfigObj->getConfig();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('hdd_standby_time');
            $xml->element('enable_hdd_standby', $result['enable_hdd_standby']);
            $xml->element('hdd_standby_time_minutes', $result['hdd_standby_time_minutes']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
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

        parse_str(file_get_contents("php://input"), $changes);
		
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        $hddStanbyConfigObj = new HddStandbyTime();
        $result = $hddStanbyConfigObj->modifyConfig($changes);

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
