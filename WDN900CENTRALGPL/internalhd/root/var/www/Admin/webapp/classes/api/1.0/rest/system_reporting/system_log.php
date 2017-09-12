<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('systemLog.php');
require_once('logMessages.php');

class System_log{
	var $logObj;

    function System_log(){
		$this->logObj = new LogMessages();
	}

	function get($urlPath, $queryParams=null, $ouputFormat='application/x-download'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $sysLogObj = new SystemLog();
        $result = $sysLogObj->getLog();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('system_log');
            $xml->element('path_to_log', $result['path_to_log']);
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

		$sysLogObj = new SystemLog();
		$result = $sysLogObj->sendLog();
		
		if($result !== NULL){
			$xml = new NasXmlWriter();
			$xml->push('system_log');
			$xml->element('transfer_success', $result['transfer_success']);
			$xml->element('logfilename', $result['logfilename']);
			$xml->pop();
			echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
			//Failed to collect info
			header("HTTP/1.0 500 Internal Server Error");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SERVER_ERROR');
		}
		
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET POST");
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
