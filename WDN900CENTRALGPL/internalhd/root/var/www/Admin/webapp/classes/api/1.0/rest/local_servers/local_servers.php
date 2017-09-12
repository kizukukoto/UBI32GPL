<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('localServers.php');

class Local_servers{

    function Local_servers(){
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        header("Content-Type: application/xml");
        
        $localServersObj = new LocalServers();
        $result = $localServersObj->getList();

        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('local_servers');
            foreach($result['local_server'] as $local_server){
                $xml->push('local_server');
                $xml->element('name', $local_server['name']);
                $xml->element('ip_address', $local_server['ip_address']);
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