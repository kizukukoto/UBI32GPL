<?php

class NetworkConfiguration{

    var $proto = '';
    var $ip = '';
    var $netmask = '';
    var $gateway = '';
	var $dns = Array();
	var $dns0 ='';
	var $dns1 ='';
	var $dns2 ='';

    function NetworkConfiguration() {
        
    }
    
    function getNetworkConfig(){
        
        //!!!This where we gather up response  
        //!!!Return NULL on error

		// get the network configuration
		unset($output);
		exec("sudo /usr/local/sbin/getNetworkConfig.sh", $output, $retVal);
		if($retVal !== 0) {
			return NULL;
		}		
		
		$networkConfig = $output;

		$this->dns[0] = '';
		$this->dns[1] = '';
		$this->dns[2] = '';
		
		if (strcasecmp($networkConfig[0], "dhcp") ==0) {
			$this->proto = 'dhcp_client';
		}
		else {
			$index = 1;
			$this->proto = 'static';
			$ip = explode(" ", $networkConfig[$index]);
									
			$this->ip = $ip[1];
			$index++;
			$netmask = explode(" ", $networkConfig[$index]);
			$this->netmask = $netmask[1];
			
			$index++;
			if (isset($networkConfig[$index])){
				$nextArgs = explode(" ", $networkConfig[$index]);	
			}
			if (strcasecmp($nextArgs[0], "gateway") === 0){
				if (isset($nextArgs[1])) {
					$this->gateway = $nextArgs[1];
				}
				$index++;
			}			
			$dnsIndex = 0;
			do {
				if (isset($networkConfig[$index])){
					$nextArgs = explode(" ",$networkConfig[$index]);						
					$this->dns[$dnsIndex] = $nextArgs[1];						
				}
				$index++;
				$dnsIndex++;
			} while (isset($networkConfig[$index]));
			
		}
		
		$this->dns0 = $this->dns[0];
		$this->dns1 = $this->dns[1];
		$this->dns2 = $this->dns[2];
		
		
//return NULL;  //Error case
        return( array( 
                    'proto' => "$this->proto", 
                    'ip' => "$this->ip",
                    'netmask' => "$this->netmask",
                    'gateway' => "$this->gateway",
                    'dns0' => "$this->dns0",
                    'dns1' => "$this->dns1",
                    'dns2' => "$this->dns2"
                    ));
    }

    function modifyNetworkConfig($changes){
		
        //Require entire representation and not just a delta to ensure a consistant representation
		// elements can be set to blank
        if( !isset($changes["proto"]) ){

            return 'BAD_REQUEST';
			
        }
		
		if  ((strcasecmp($changes["proto"], "dhcp_client") !=0) &&
			((!isset($changes["ip"])      || 
						!isset($changes["netmask"]) ||
						!isset($changes["gateway"]) ||
						!isset($changes["dns0"])    ||
						!isset($changes["dns1"])    ||
						!isset($changes["dns2"]) ))) {

            return 'BAD_REQUEST';

		}
			
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }	
        
			if (strcasecmp($changes["proto"], "dhcp_client") ==0) {
			
			// set the network configuration to DHCP
			unset($output);
			exec("sudo /usr/local/sbin/setNetworkDhcp.sh", $output, $retVal);
			if($retVal !== 0) {
				return 'SERVER_ERROR';
			}	
		}
		else {
			    
			// set the network configuration to static 
			
				exec("sudo /usr/local/sbin/setNetworkStatic.sh \"{$changes["ip"]}\" \"{$changes["netmask"]}\" \"{$changes["gateway"]}\" \"{$changes["dns0"]}\" \"{$changes["dns1"]}\" \"{$changes["dns2"]}\""
				      , $output, $retVal);
							
		}

		return 'SUCCESS';
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
