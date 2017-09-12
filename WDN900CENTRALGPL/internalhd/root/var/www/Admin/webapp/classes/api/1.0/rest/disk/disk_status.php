<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('diskStatus.php');

class Disk_status{

    function Disk_status(){
    }


    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $diskStatusObj = new DiskStatus();
        $result = $diskStatusObj->getStatus();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('disk_status');
            $xml->element('disk', $result['disk']);
            $xml->element('size', $result['size']);
            $xml->element('description', $result['description']);
            $xml->element('status', $result['status']);
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