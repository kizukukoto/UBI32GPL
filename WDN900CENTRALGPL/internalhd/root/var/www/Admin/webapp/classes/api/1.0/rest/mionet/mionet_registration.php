<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('mionetRegistration.php');
require_once('logMessages.php');

class Mionet_registration{
	var $logObj;

    function Mionet_registration(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
        header("HTTP/1.0 405 Method Not Allowed");
    }


    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

		$parameterChanges = $_POST;
		
		$parameterChanges['password'] = '********';
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $parameterChanges);

		$obj = new MionetRegistration();

        $result = $obj->register($_POST);
	
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
        default:
    		header("Content-Type: application/xml");
    		echo $result;
	        break;
        }
		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);
	}

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: POST");
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
