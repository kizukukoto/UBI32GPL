<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('eulaAcceptance.php');
require_once('logMessages.php');
require_once('restcomponent.inc');

class Eula_acceptance extends RestComponent {

	var $logObj;
	
    function Eula_acceptance(){
		$this->logObj = new LogMessages();
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Content-Type: application/xml");
        
        $eulaObj = new EulaAcceptance();
        $result = $eulaObj->getAcceptance();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('eula_acceptance');
            $xml->element('accepted', $result['accepted']);
            $xml->pop();
            echo $xml->getXml();
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
		} else {
            //Resource (EULA Acceptance) not created yet
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'NOT_FOUND');
			header("HTTP/1.0 404 Not Found");
        }
    }


    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        $eulaObj = new EulaAcceptance();
        $result = $eulaObj->accept($queryParams);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $_POST);

		
        switch($result){
        case 'SUCCESS':
            header("HTTP/1.0 201 Created");
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
