<?php

class System_state extends RestComponent {

	var $logObj;

    function __construct() {
        require_once('restcomponent.inc');
        require_once('systemState.php');
        require_once('logMessages.php');
        $this->logObj = new LogMessages();
    }

    function get($urlPath, $queryParams=null, $outputFormat='xml'){
        $infoObj = new SystemState();
        $result = $infoObj->getState();
        if($result !== NULL){
            $this->generateSuccessOutput(200, 'system_state', $result, $outputFormat);
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Failed to collect info
            header("HTTP/1.0 500 Internal Server Error");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SERVER_ERROR');
		}
    }

    function put($urlPath, $queryParams=null, $outputFormat='xml'){
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function post($urlPath, $queryParams=null, $outputFormat='xml'){
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $outputFormat='xml'){
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
