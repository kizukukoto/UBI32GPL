<?php

require_once('userlist.php');
require_once('usersdb.inc');


class User{
    
    var $username = '';
    var $fullname = '';
    var $userid = '';
	static $logObj;
	static $lockObj;

	public function __construct($userName = '', $userFullName = '', $userId = '')
	{
		if (!isset(self::$lockObj)) {
			self::$lockObj = new Lock("Userlist_lock");
		}
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
        $this->username = $userName;
        $this->fullname = $userFullName;
        $this->userid = $userId;
	}
	
	
//    function User($userName = '', $userId = '') {
//        $this->username = $userName;
//        $this->fullname = $userId;
//    }

    function getXmlOutput(){
        $xmloutput = '<user>';
        if(strcmp ( $this->user_id, "") == 0){
            $xmloutput = $xmloutput.'<fullname/>';
        } else {
            $xmloutput = $xmloutput.'<fullname>'.$this->user_id.'</fullname>';
        }

        $xmloutput = $xmloutput.'</user>';
        
        return $xmloutput;
    }

/*
 * $userId = $UsersDB->createUser($username, $password, $fullname, $isAdmin);
 * 
 * usersdb.inc
 * 
 * 	function createUser($username, $password=null, $fullname=null, $isAdmin=null)
	{
		$isPassword = !empty($password) ? '1' : '0';
		$bindVarNVTArray = array(
			array(':local_username', getSafeDatabaseText((string)$username), PDO::PARAM_STR),
			array(':fullname', getSafeDatabaseText((string)$fullname), PDO::PARAM_STR),
			array(':is_admin', $isAdmin, PDO::PARAM_INT),
			array(':is_password', $isPassword, PDO::PARAM_INT),
		);
		$userId = $this->executeInsert(self::$queries['INSERT_USER'], 'INSERT_USER', $bindVarNVTArray);
		if ($userId < 1) $userId = false;
		
		return $userId;
	}
 */
    // configure users in the 'Xternal' db from current OS settings
    function configUserXdb () {
        
        self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "");
    
        $ulist = array();
        
        $ul = new Userlist();
    	$ulist = $ul->getUsers();
    	
    	//var_dump($ulist);
    	    	
    	if($ulist === NULL) {
    		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "WARN: getUsers() call failed");
    	}

    	for( $i = 0; $i<sizeof($ulist); $i++ ) {
    	    
    	    //var_dump($ulist[$i]['username']);
    	    
   		    // username
  		    $username = $ulist[$i]['username'];
  		    
    		if($username !== "") {
    		    
                // 
                $password = 'true';
                
                // fullname
    			unset($outputFullName);
    			exec("sudo /usr/local/sbin/getUserInfo.sh $username 'fullname'", $outputFullName, $retVal);
    			if($retVal !== 0) {
    			    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "Error: getUserInfo.sh call failed");
    				return NULL;
    			}
    			$fullname = $outputFullName[0];
    			
    			
    		    // isAdmin
    			unset($ownerName);
	            exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
	            if($retVal !== 0) {
		            $logObj->LogData('OUTPUT', NULL,  __FUNCTION__,  'getOwner.sh failed');
		            return NULL;
	            }
	            $ownername = "$ownerName[0]";
	            
	            // initialize users db
    			//$isAdmin = $username == "$ownerName[0]" ? '1' : '0';
				$userId = 0;
				$isAdmin = 'false';
				$usersDB = new UsersDB();
				
				unset($isPassword);
				exec("sudo /usr/local/sbin/usrPwdExists.sh $username", $isPassword, $retVal);
				$password = "$isPassword[0]";
				
    			if ($username == $ownername) {
	   				$isAdmin = 'true';
	   				$userId = 1;
	   				$hasPassword = ($password == 'yes') ? true : false;
    				$usersDB->updateUser($userId, $username, $fullname, null, null, $isAdmin, $hasPassword);
    			}
    			else {
    				$hasPassword = ($password == 'yes') ? 'true' : null;
    	        	$userId = $usersDB->createUser($username, $hasPassword, $fullname, $isAdmin);
    			}
    			
    	        //var_dump($userId);
    	        //var_dump($username);
    	        
    	        if ($userId > 0) {
        	        unset($output);
        	        // set userId in password comment field for upgraded FW version
        	        exec("sudo chfn -f \"$fullname\" -r \"$userId\" \"$username\"", $output, $retVal);
           			if($retVal !== 0) {
        			    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "Error: useradd call failed");
        			    return NULL;
        			}
    	        	// user should already exist in htpasswd file if there was a password
		        	$ul->createApacheUser($username, $userId);
        		}
    	    }
        }
    }
  
  
}

  

  
class UserGroupXdb {

  	var $userId = '';
    var $userName = '';
	var $userPwd = '';
    
//	static $logObj;	

	public function __construct() {
//		if (!isset(self::$logObj)) {
//			self::$logObj = new LogMessages();
//		}
	}
    
    function apacheDirective( ){
    	
        $xmloutput = "\n".'<user>'."\n";
        if(strcmp ( $this->fullName, "") == 0){
            $xmloutput = $xmloutput."  ".'<fullName/>';
        } else {
            $xmloutput = $xmloutput."  ".'<userName>'."\n"."  "."  ".$this->userName."\n"."  ".'</userName>'."\n";
            $xmloutput = $xmloutput."  ".'<fullName>'."\n"."  "."  ".$this->fullName."\n"."  ".'</fullName>'."\n";
        }

        $xmloutput = $xmloutput.'</user>'."\n";
        
        return $xmloutput;
    }

  	function genGroupFile()
	{
		// self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "");
		
		$ulistObj = new Userlist();
		$deviceUserDb = new DeviceUsersDB();	
		$ulist = array();
		$devusrlist = array();
		$userIdList = array();
		
		//$ulist = $ulistObj->getUsers();  // test
		unset($userIdList);
		$k = 0;
	    //for($i = 0; $i < sizeof($ulist); $i++ ) { // test
	    for($i = 3; $i < 7; $i++ ) {  
	    //
    		//$userId = $ulist[$i]['user_id']; // test
    		$userId = $i;
    		unset($devusrlist);
			$devusrlist = $deviceUserDb->getDeviceUsersForUserId($userId);

			$userIdList[$k]['user_id'] = $userId;
    		$userIdList[$k]['user_name'] = 'device_id_';
    		$k++;

			//
			if ($devusrlist !== NULL) {
				//
				for($j = 0; $j < sizeof($devusrlist); $j++ ) {
					//
    				$userIdList[$k]['user_id'] = $userId;
    				$userIdList[$k]['user_name'] = $devusrlist[$j]['device_user_id'];
    				$k++;
				}
    		}
    		else {
				self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "ERROR: userIdList is empty.");
				return false;
    		}
    	}

		foreach($userIdList as $userEntry){
				print 'deviceUser for userId '.$userEntry['user_id'].' is '.$userEntry['user_name']."\n";
		}
		return true;
	}
	
  	function build() {
  		
  		// build user group file
  		
  		// generate apache group file
  		
  		// generate apache 'include' directives
  		
  	}

  }

/*

require_once('acllist.php');
require_once('user.php');


$aclObj = new Acllist();
$userObj = new User();

$userObj->configUserXdb();
$aclObj->setSharesConfigXdb();

*/
  
/*
//test code
require_once('deviceusersdb.inc');
require_once('globalconfig.inc');
require_once('outputwriter.inc');
require_once('security.inc');
require_once('util.inc');

$_SERVER["REMOTE_ADDR"] = 'local';  //Clean up warning

$deviceUserDb = new DeviceUsersDB();

$userId=3;
$deviceUserId=123;
$deviceUserAuth='ikOgm2m0TWgvY2goH3LKKw==';
$deviceUserDb->createDeviceUser($userId, $deviceUserId,	$deviceUserAuth,'', '', '', false, true, null , null);

$userId=4;
$deviceUserId=124;
$deviceUserAuth='ikOgm2m0TWgvY2goH3LKKw==';
$deviceUserDb->createDeviceUser($userId, $deviceUserId,	$deviceUserAuth,'', '', '', false, true, null , null);

$userId=5;
$deviceUserId=125;
$deviceUserAuth='ikOgm2m0TWgvY2goH3LKKw==';
$deviceUserDb->createDeviceUser($userId, $deviceUserId,	$deviceUserAuth,'', '', '', false, true, null , null);

$userId=6;
$deviceUserId=126;
$deviceUserAuth='ikOgm2m0TWgvY2goH3LKKw==';
$deviceUserDb->createDeviceUser($userId, $deviceUserId,	$deviceUserAuth,'', '', '', false, true, null , null);

$userId=7;
$deviceUserId=127;
$deviceUserAuth='ikOgm2m0TWgvY2goH3LKKw==';
$deviceUserDb->createDeviceUser($userId, $deviceUserId,	$deviceUserAuth,'', '', '', false, true, null , null);

//unset($deviceUsers);
//$deviceUsers = $deviceUserDb->getDeviceUsersForUserId(4);
//print 'devicUsers for userId 4:' . "\n";
//var_dump($deviceUsers);

//$userGrpObj = new UserGroup();
//$userGrpObj->buildUserGroupFile();

//$userArray=$userGrpObj->getXmlOutput();
*/
  
/*
 * comment out these lines '//' in security.inc
 *
 	$hc = new HttpClient();
	$response = $hc->get($serverUrlDeviceUser);
	if($response['status_code'] != 200){
//		return false;
	}
	$deviceXml = $response['response_text'];
	//check response
	$deviceUser = simplexml_load_string($deviceXml);
	if (stristr($deviceUser->status, 'success') == false) {
		//log error
//		return false;
	}
*/

/*
 [1]=>
  array(13) {
    ["device_user_id"]=>
    string(3) "123"
    ["user_id"]=>
    string(1) "4"
    ["auth"]=>
    string(24) "ikOgm2m0TWgvY2goH3LKKw=="
    ["email"]=>
    string(0) ""
    ["type"]=>
    string(0) ""
    ["name"]=>
    string(0) ""
    ["is_active"]=>
    string(0) ""
    ["created_date"]=>
    string(10) "1301786305"
    ["dac"]=>
    NULL
    ["dac_expiration"]=>
    NULL
    ["enable_wan_access"]=>
    string(1) "1"
    ["type_name"]=>
    NULL
    ["application"]=>
    NULL
  }
*/
/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */

?>