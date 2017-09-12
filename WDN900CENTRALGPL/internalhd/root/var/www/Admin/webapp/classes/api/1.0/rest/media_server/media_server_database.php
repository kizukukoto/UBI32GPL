<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('mediaServerDatabase.php');
require_once('logMessages.php');

class Media_server_database{
	var $logObj;

    function Media_server_database(){
		$this->logObj = new LogMessages();

    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
/*
    	if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }
*/
        header("Content-Type: application/xml");
        
        $mediaServerDataBaseObj = new MediaServerDatabase();
        $result = $mediaServerDataBaseObj->getStatus();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('media_server_database');
            $xml->element('version', $result['version']);
			$xml->element('time_db_update', $result['time_db_update']);
            $xml->element('music_tracks', $result['music_tracks']);
            $xml->element('pictures', $result['pictures']);
            $xml->element('videos', $result['videos']);
			$xml->element('scan_in_progress', $result['scan_in_progress']);
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
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        parse_str(file_get_contents("php://input"), $changes);

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

        $mediaServerDataBaseObj = new MediaServerDatabase();
        $result = $mediaServerDataBaseObj->config($changes);

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
