<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('copyStatus.php');

class Copy_status{

    function Copy_status(){
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $statusObj = new CopyStatus();
        $result = $statusObj->getStatus();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('copy_status');
            $xml->element('files_copied', $result['files_copied']);
            $xml->element('size_copied', $result['size_copied']);
            $xml->element('total_files', $result['total_files']);
            $xml->element('total_size',  $result['total_size']);
            $xml->element('file_name',  $result['file_name']);
            $xml->element('in_progress',  $result['in_progress']);
            $xml->pop();
            echo $xml->getXml();
        } else {
            //Failed to collect info
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