<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('storageUsage.php');
require_once('logMessages.php');

class Storage_usage{
	var $logObj;

    function Storage_usage(){
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
//        if(!authenticateAsOwner($queryParams))
//        {
//            header("HTTP/1.0 401 Unauthorized");
//            return;
//        }

        header("Content-Type: application/xml");
        
        $storageUsageObj = new StorageUsage();
        $result = $storageUsageObj->getStorageUsage();				

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('storage_usage');
            $xml->element('size', $result['size']);
            $xml->element('usage', $result['usage']);
            $xml->element('video', $result['video']);
            $xml->element('photos', $result['photos']);
            $xml->element('music', $result['music']);
            $xml->element('other', $result['other']);
            $xml->push('shares');
            foreach($result['share'] as $share){
                $xml->push('share');
                $xml->element('sharename', $share['sharename']);
                $xml->element('usage', $share['usage']);
                $xml->element('video', $share['video']);
                $xml->element('photos', $share['photos']);
                $xml->element('music', $share['music']);
                $xml->element('other', $share['other']);
                $xml->pop();
            }
            $xml->pop();
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
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function post($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
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
