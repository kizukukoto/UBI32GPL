<?php
require_once('acl.php');
require_once('NasXmlWriter.class.php');
require_once('acllist.php');
require_once('authenticate.php');
require_once('logMessages.php');

class Share_access_configuration{
	var $logObj;

	function Share_access_configuration(){
		$this->logObj = new LogMessages();
	}

    function getACLs($share){
		$aclListobj = new Acllist();
		$aclList = $aclListobj->getAcls($share);
        return $aclList;
    }

    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $urlParts[5]);
		
        $urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));
        // Get specific ACL:  /share_access_configuration/{share Name}
        if(isset($urlParts[5]) && !(strcmp ( $urlParts[5], "") == 0) ){
			$aclList = $this->getACLs($urlParts[5]);

			if($aclList === NULL){
                header("HTTP/1.0 500 Internal Server Error");
                return;
            }

			$testAcl = $aclList[0];
			
            //if($testAcl->shareName === $urlParts[5]) {
			if($testAcl !== NULL) {
				$xml = new NasXmlWriter();
                $xml->push('share_access_configuration');
                $xml->element('remote_access', $testAcl->remote_access);
                $xml->element('public_access', $testAcl->public_access);
                $xml->element('media_serving', $testAcl->media_serving);
                $xml->push('fullaccess_user_arr');
                foreach($testAcl->fullaccess_user_arr as $fullaccess_user) {
                    $xml->element('user_name', $fullaccess_user['user_name']);
                }
                $xml->pop();
                $xml->push('readonly_user_arr');
                foreach($testAcl->readonly_user_arr as $readonly_user) {
                    $xml->element('user_name', $readonly_user['user_name']);
                }
                $xml->pop();
                $xml->pop();
                echo $xml->getXml();
                return;
				$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
			} else {
                // Specific ACL not found   
                header("HTTP/1.0 404 Not Found");
                return;
				$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'NOT_FOUND');
			}
        } else {
            // Get all is not defined in spec.
            header("HTTP/1.0 404 Not Found");
            return;
        }
    }

    function put($urlPath, $queryParams=null, $ouputFormat='xml'){
        if(!authenticateAsOwner($queryParams))
        {
            header("HTTP/1.0 401 Unauthorized");
            return;
        }

        $urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));
        if(isset($urlParts[5]) && !(strcmp ( $urlParts[5], "") == 0) ){
            $shareName = $urlParts[5];
			
			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $shareName);

            parse_str(file_get_contents("php://input"), $changes);
			
			$this->logObj->LogParameters(get_class($this), __FUNCTION__, $changes);

			
            $aclListobj = new Acllist();
            $result = $aclListobj->modifyAcl($shareName, $changes);

            switch($result){
            case 'SUCCESS':
                break;
            case 'BAD_REQUEST':
                header("HTTP/1.0 400 Bad Request");
                break;
            case 'ACL_NOT_FOUND':
                header("HTTP/1.0 404 Not Found");
                break;
            case 'SERVER_ERROR':
                header("HTTP/1.0 500 Internal Server Error");
                break;
            }
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $result);

        } else {
            header("HTTP/1.0 400 Bad Request");
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'BAD_REQUEST');
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
