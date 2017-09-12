<?php

//include_once("HttpClient.php");
class MionetRegistered{

    var $registered = '';
    var $mionet_username = '';
	var $mionet_conf_file="/etc/mionet/senvid_install.conf";
	var $mionet_prop_file="/usr/mionet/Senvid.txt";
	var $URL = "";
	var $stagingURL= "stagingmionet.senvid.net";
	var $port=80; //This is default port
	var $prodURL="www.senvid.net";

    function DeviceDescripton() {
    }

    function getRegistered(){
        //!!!This where we gather up response
        //!!!Return NULL on error
        return NULL;
/*
		$ini = parse_ini_file($this->mionet_prop_file);
		$nodeID = $ini["nodeID"];

        if($nodeID ==-1){
        	$this->registered = "false";
        	$this->mionet_username = "";

        	return( array( 'registered' => "$this->registered", 'mionet_username' => "$this->mionet_username"));
        }
        $output = @exec('sudo chmod o+r /etc/mionet/senvid_install.conf');
        if(!is_null($output) && strlen($output) >0) {
        	return NULL;  //Error case
        }

		$ini = parse_ini_file($this->mionet_conf_file);
		$username = $ini["Username"];
		@exec('sudo chmod o-r /etc/mionet/senvid_install.conf');

        $this->registered = "true";
        $this->mionet_username = $username;

        //return NULL;  //Error case
        return( array( 'registered' => "$this->registered", 'mionet_username' => "$this->mionet_username"));
*/
    }

    function modifyRegistered($changes){
		return 'BAD_REQUEST';
/*
    	//Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["registered"]) ||
            !isset($changes["mionet_username"]) ){

            return 'BAD_REQUEST';
        }

		$ini = parse_ini_file($this->mionet_prop_file);
		$nodeID = $ini["nodeID"];

        if($nodeID ==-1){
        	//This means the user never registered before. So unregistration makes no sense
        	 return 'BAD_REQUEST';
        }

        //Now get the user name registered for this box
        $output = @exec('sudo chmod o+r /etc/mionet/senvid_install.conf');
		if(!is_null($output) && strlen($output) >0) {
			return 'BAD_REQUEST';  //Error case
		}

		$ini = parse_ini_file($this->mionet_conf_file);
		$username = $ini["Username"];

		@exec('sudo chmod o-r /etc/mionet/senvid_install.conf');

        //compare it with the one sent by user
        if(strcasecmp ($username,$changes["mionet_username"]) !=0){
        	 return 'BAD_REQUEST';
        }

        $ini = parse_ini_file($this->mionet_prop_file);

		$ssURL = $ini["ssUrl"];
		if(isset($ssURL)){
			$pos = strpos($ssURL,$this->prodURL);

			if($pos === false) {
				$this->URL=	$this->stagingURL;
			}
			else{
				//This means we are hitting production URL
				$this->URL=	$this->prodURL;
			}

		}
		else{
			$this->URL=	$this->stagingURL;
		}

		//Now unregister it
		exec("sudo /usr/mionet/mionetd reset");
		$removenode =true;
		try
		{
			$client = new HttpClient($this->URL,$this->port);
			$client->post("/servlet/senvid.webTop.server.registration.NodeCreationPHPServlet"
				, array(
					"removenode" => $removenode
					,"nodeId" => $nodeID
				));
			$result = $client->getContent();

			if($result ==-1) {
				$errmsg="Unable to delete the Node from the Database";
				syslog(LOG_ERR, $errmsg);
			}

		}catch(Exception $e)
		{
			syslog(LOG_ERR, $e->getMessage());
			return 'BAD_REQUEST';
		}

        return 'SUCCESS';
*/
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