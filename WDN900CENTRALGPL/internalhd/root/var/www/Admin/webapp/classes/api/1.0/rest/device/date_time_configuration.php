<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('DateTimeConfiguration.php');
require_once('logMessages.php');

class Date_time_configuration{
	var $logObj;

    function Date_time_configuration(){
		$this->logObj = new LogMessages();

    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $dateTimeConfObj = new DateTimeConfiguration();
        $result = $dateTimeConfObj->getConfig();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('date_time_configuration');
            $xml->element('datetime', $result['datetime']);
            $xml->element('ntpservice', $result['ntpservice']);
            $xml->element('ntpsrv0', $result['ntpsrv0']);
            $xml->element('ntpsrv1', $result['ntpsrv1']);
            $xml->element('ntpsrv_user', $result['ntpsrv_user']);
            $xml->element('time_zone_name', $result['time_zone_name']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Failed to collect info
            header("HTTP/1.0 500 Internal Server Error");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'INTERNAL SERVER ERROR');
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

        $dateTimeConfObj = new DateTimeConfiguration();
        $result = $dateTimeConfObj->modifyConf($changes);
		
        switch($result){
        case 'SUCCESS':
            syslog(LOG_NOTICE, "SUCCESS Date Time Configured");
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
