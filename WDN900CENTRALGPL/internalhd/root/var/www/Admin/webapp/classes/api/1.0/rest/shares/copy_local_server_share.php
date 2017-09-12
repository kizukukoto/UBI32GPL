<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('copyLocalServerShare.php');

class Copy_local_server_share{

    function Copy_local_server_share(){
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

        $copyObj = new CopyLocalServerShare();
        $result = $copyObj->copy($_POST);

        switch($result){
        case 'SUCCESS':
            header("HTTP/1.0 201 Created");
            syslog(LOG_NOTICE, "Accepted EULA");
            break;
        case 'BAD_REQUEST':
            header("HTTP/1.0 400 Bad Request");
            break;
        case 'SERVER_ERROR':
            header("HTTP/1.0 500 Internal Server Error");
            break;
        }
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