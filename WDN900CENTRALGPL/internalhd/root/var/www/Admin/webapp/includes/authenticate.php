<?php
require_once('logMessages.php');

function authenticateAsOwner($queryParams){
	$logObj = new LogMessages();

    if( !isset($queryParams["pw"]) ){
			$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'owner password not supplied');
		return FALSE;
    }

    //Verify owner name
//    unset($ownerName);
//    exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
//    if($retVal !== 0) {
//		$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'getOwner.sh failed');
//        return NULL;
//    }
//    if($ownerName[0] !== $queryParams["owner"]){
//
//		$logObj->LogParameters(NULL, __FUNCTION__, $ownerName[0]);
//		$logObj->LogParameters(NULL, __FUNCTION__, "{$queryParams["owner"]}");
//		$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'owner do not match');
//        return FALSE;
//    }
//
    //Verify password

    unset($ownerUid);
    unset($ownerName);
    $useShellScriptForOwnerName = false;
    $useShellScriptForHash = false;
    $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
    if ($nasConfig === false)
    {
        $useShellScriptForOwnerName = true;
        $useShellScriptForHash = true;
    }
    else 
    {
        if (!isset($nasConfig['admin']['SH_SETUP_PARAM_FILE']) || !isset($nasConfig['admin']['PASSWD_FILE']))
        {
            $useShellScriptForOwnerName = true;
        }
        if (!isset($nasConfig['admin']['SHADOW_FILE']))
        {
            $useShellScriptForHash = true;
        }
    }
    if (SKIP_SHELL_SCRIPT && !$useShellScriptForOwnerName)
    {
        $handle = @fopen($nasConfig['admin']['SH_SETUP_PARAM_FILE'], 'r');
        if ($handle === false)
        {
            $useShellScriptForOwnerName = true;
        }
        else
        {
            while (($buffer = fgets($handle)) !== false)
            {
                if ($buffer[0] != '#')
                {
                    $data = explode('=', trim($buffer));
                    if ($data[0] == 'ownerUid')
                    {
                        $ownerUid = str_replace('"','', $data[1]);
                        break;
                    }
                }
            }
            fclose($handle);
            if (isset($ownerUid))
            {
                $handle = @fopen($nasConfig['admin']['PASSWD_FILE'], 'r');
                if ($handle === false)
                {
                    $useShellScriptForOwnerName = true;
                }
                else
                {
                    while (($buffer = fgets($handle)) !== false)
                    {
                        if ($buffer[0] != '#')
                        {
                            $data = explode(':', trim($buffer));
                            if ($data[3] == '1000' && $data[2] == $ownerUid)
                            {
                                $ownerName = $data[0];
                                break;
                            }
                        }
                    }
                    fclose($handle);
                    if (!isset($ownerName))
                    {
                        $logObj->LogData('OUTPUT', NULL, __FUNCTION__, 'getOwner failed');
                        return NULL;
                    }
                }
            }
           
        }
    }
    if (!SKIP_SHELL_SCRIPT || $useShellScriptForOwnerName)
    {
    	unset($ownerName);
        exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
    	if($retVal !== 0) {
    		$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'getOwner.sh failed');
    		return NULL;
    	}
    	$ownerName = $ownerName[0];
    }

    unset($hash);
    if (SKIP_SHELL_SCRIPT && !$useShellScriptForHash)
    {
        $handle = @fopen($nasConfig['admin']['SHADOW_FILE'], 'r');
        if ($handle === false)
        {
            $useShellScriptForHash = true;
        }
        else
        {
            while (($buffer = fgets($handle)) !== false)
            {
                if ($buffer[0] != '#')
                {
                    $data = explode(':', trim($buffer));
                    if ($data[0] == $ownerName)
                    {
                        $hash = $data[1];
                        break;
                    }
                }
            }
            fclose($handle);
            if (!isset($hash))
            {
                $logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'invalid username or password');
                return NULL;
            }
        }
    }
    if (!SKIP_SHELL_SCRIPT || $useShellScriptForHash) 
    {
    	unset($hash);
        exec("sudo awk -v usr=$ownerName -F: '{if ($1 == usr) print $2}' /etc/shadow", $hash, $retVal);
        if($retVal !== 0)
        {
            $logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'invalid username or password');
            return NULL;
        }
        $hash = $hash[0];
    }


    $decodedPw = base64_decode($queryParams["pw"], false);
    if("$decodedPw" === FALSE){
			$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'invalid username or password');
        return FALSE;
    }

    if($hash === ''){
        if($queryParams["pw"] === ""){
            return TRUE;
        } else {
				$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'invalid username or password');
				return FALSE;
        }
    }

    $salt = substr($hash, 0, 12);
    unset($challange);
    $challange = crypt($decodedPw, "$salt");		


    if($challange === $hash){
        return TRUE;
    } else {
			$logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'invalid password');
			return FALSE;
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
