<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('mediaServerBlockedList.php');

class Media_server_blocked_list{

    function Media_server_blocked_list(){
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $blockListObj = new MediaServerBlockedList();
        $result = $blockListObj->getBlockList();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('media_server_blocked_list');
            foreach($result['device'] as $device){
                $xml->push('device');
                $xml->element('mac_address', $device['mac_address']);
                $xml->pop();
            }
            $xml->pop();
            echo $xml->getXml();
        } else {
            //Failed to collect info
            header("HTTP/1.0 500 Internal Server Error");
        }
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        parse_str(file_get_contents("php://input"), $changes);

        $blockListObj = new MediaServerBlockedList();
        $result = $blockListObj->modifyBlockList($changes);

        switch($result){
        case 'SUCCESS':
            syslog(LOG_NOTICE, "Machine changed");
            break;
        case 'BAD_REQUEST':
            header("HTTP/1.0 400 Bad Request");
            break;

        case 'SERVER_ERROR':
            header("HTTP/1.0 500 Internal Server Error");
            break;
        }
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