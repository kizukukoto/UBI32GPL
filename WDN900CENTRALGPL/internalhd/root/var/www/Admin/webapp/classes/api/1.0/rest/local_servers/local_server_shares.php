<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('localServerShares.php');

class Local_server_shares{

    function Local_server_shares(){
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        $urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));

        header("Content-Type: application/xml");

        // Get specific local server:  /local_server_shares/{local server ip address}
        if(isset($urlParts[5]) && !(strcmp ( $urlParts[5], "") == 0) ){
            $localServerSharesObj = new LocalServerShares();
            $result = $localServerSharesObj->getLocalShares($urlParts[5], $error);

            if($result !== NULL){
                $xml = new NasXmlWriter();
                $xml->push('local_server_shares');
                foreach($result['local_share'] as $local_share){
                    $xml->push('local_share');
                    $xml->element('name', $local_share['name']);
                    $xml->element('public', $local_share['public']);
                    $xml->pop();
                }
                $xml->pop();
                echo $xml->getXml();
                return;
            } else {
                if($error === 'NOT_FOUND')
                {
                    // Local server not found
                    header("HTTP/1.0 404 Not Found");
                    return;
                } else {
                    //Failed to collect info
                    header("HTTP/1.0 500 Internal Server Error");
                    return;
                }
            }
        } else {
            // No plan to query all server
            header("HTTP/1.0 404 Not Found");
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