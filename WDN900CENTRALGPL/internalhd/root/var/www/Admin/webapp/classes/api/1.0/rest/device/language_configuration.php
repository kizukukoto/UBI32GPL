<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('languageConfiguration.php');
require_once('stringtablereader.inc');
require_once('logMessages.php');

class Language_configuration{
	var $logObj;

    function Language_configuration(){
		$this->logObj = new LogMessages();
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Content-Type: application/xml");
        
        $langConfigObj = new LanguageConfiguration();
        $result = $langConfigObj->getConfig();

        if($result !== NULL){
            if($result['language'] === '' ) {
                // Language not configured
                header("HTTP/1.0 404 Not Found");            
            } else {
                $xml = new NasXmlWriter();
                $xml->push('language_configuration');
                $xml->element('language', $result['language']);
                $xml->pop();
                echo $xml->getXml();
				$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
			}
        } else {
            //Failed to collect info
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'NOT_FOUND');
			header("HTTP/1.0 404 Not Found");
        }
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){

        parse_str(file_get_contents("php://input"), $changes);

        $langConfigObj = new LanguageConfiguration();
        $result = $langConfigObj->modifyConfig($changes);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        switch($result){
        case 'SUCCESS':
            break;
        case 'BAD_REQUEST':
            header("HTTP/1.0 400 Bad Request");
            break;
        case 'NOT_FOUND':
            header("HTTP/1.0 404 Not Found");            
            break;
        case 'SERVER_ERROR':
            header("HTTP/1.0 500 Internal Server Error");
            break;
        }
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
	}

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){

    	$this->logObj->LogData('>>><<<', get_class($this),  __FUNCTION__,  $result);
    	// var_dump($queryParams);
    	
		parse_str(file_get_contents("php://input"), $changes);

		$langConfigObj = new LanguageConfiguration();
		$result = $langConfigObj->config($changes);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

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

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET, PUT, POST");
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
