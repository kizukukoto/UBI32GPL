<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('ownerConfiguration.php');
require_once('logMessages.php');

class Owner_configuration{
	var $logObj;

    function Owner_configuration(){
		$this->logObj = new LogMessages();
	}

	function getOwnerConfigOnStingray(){
		$fp = fopen("/etc/passwd", "r");
		if (!$fp)
		return NULL;
		$result = array();
		while (($line = fgets($fp, 512)) !== false) {
			list($user, $pass, $uid, $gid, $info, $home) = split(":", $line, 6);
			if($user === 'admin'){
				$infoList = split(',', $info);
				$fullName = $infoList[0];
				$result['owner_name']= $user;
				$result['fullname']= $fullName;
				breeak;
			}
		}
		fclose($fp);
		 
		$fp1 = fopen("/etc/shadow", "r");
		if (!$fp1)
		return NULL;
		while (($line = fgets($fp1, 512)) !== false) {
			$passInfo = split(":", $line);
			if($passInfo[0] === 'admin'){
				$emptyPasswd = "false";
				if(empty($passInfo[1])){
					$emptyPasswd = "true";
				}
				$result['empty_passwd']= $emptyPasswd;
				break;
			}
		}
		fclose($fp1);
		return $result;
	}
	
    function get($urlPath, $queryParams=null, $ouputFormat='xml'){
        //Get owner is not password protected so UI can pre-populate owner name in login
        header("Content-Type: application/xml");
        if(getDeviceType() === '5'){
        	$result = $this->getOwnerConfigOnStingray();
        }else{
	        $ownerConfigObj = new OwnerConfiguration();
	        $result = $ownerConfigObj->getConfig();
        }
        if($result !== NULL){
            $xml = new NasXmlWriter();
            $xml->push('owner_configuration');
            $xml->element('owner_name', $result['owner_name']);
            $xml->element('fullname', $result['fullname']);
            $xml->element('empty_passwd', $result['empty_passwd']);
            $xml->element('status', 'success');
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
		
		$ownerInfo = $changes;
		$ownerInfo["empty_passwd"] ="************";
		
		$ownerInfo["change_passwd"] ="************";
		$ownerInfo["owner_passwd"] = "************";
		
		$this->logObj->LogParameters(get_class($this), __FUNCTION__, $ownerInfo);

        $ownerConfigObj = new OwnerConfiguration();
        $result = $ownerConfigObj->modifyConfig($changes);

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
