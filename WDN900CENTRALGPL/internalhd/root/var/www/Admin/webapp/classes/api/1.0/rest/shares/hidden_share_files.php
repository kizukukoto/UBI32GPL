<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('hiddenShareFiles.php');
require_once('logMessages.php');

class Hidden_share_files{
	var $logObj;
	function Hidden_share_files(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $sharesListObj = new HiddenShareFiles();
        $result = $sharesListObj->getShares();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('hidden_share_files');
            foreach($result['share'] as $share){
                $xml->push('share');
                $xml->element('sharename', $share['sharename']);

                foreach($share['file'] as $file){
                    $xml->push('file');
                    $xml->element('backup_name', $file['backup_name']);
                    $xml->element('file_time_stamp', $file['file_time_stamp']);
                    $xml->element('size', $file['size']);
                    $xml->pop();
                }
                $xml->pop();
            }
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
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
		// !!!Need to add lock
	
		if(!authenticateAsOwner($queryParams))
		{
			header("HTTP/1.0 401 Unauthorized");
			return;
		}		

		$urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $urlParts[5]);
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $queryParams[backup_name]);

		if(isset($urlParts[5]) && !(strcmp ( $urlParts[5], "") == 0) && 
		   isset($queryParams[backup_name]) && !(strcmp ( $queryParams[backup_name], "") == 0)){
			$shareName = urldecode($urlParts[5]);
			$backupName = urldecode($queryParams[backup_name]);

			$slobj = new HiddenShareFiles();
				$result = $slobj->deleteBackup($shareName, $backupName);

			switch($result){
				case 'SUCCESS':
						syslog(LOG_NOTICE, "$backupName in $shareName deleted");
					break;
				case 'BACKUP_NOT_FOUND':
					header("HTTP/1.0 404 Not Found");
					break;
				case 'SERVER_ERROR':
					header("HTTP/1.0 500 Internal Server Error");					
					break;
			}
		} else {
			header("HTTP/1.0 400 Bad Request");
		}
		// !!!Need to remove lock
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
