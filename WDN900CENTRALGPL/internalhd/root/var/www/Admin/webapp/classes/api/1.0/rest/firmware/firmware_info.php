<?php
require_once('NasXmlWriter.class.php');
require_once('authenticate.php');
require_once('firmwareInfo.php');
require_once('logMessages.php');

class Firmware_info{
	var $logObj;

    function Firmware_info() {
		$this->logObj = new LogMessages();
	}

    function get($urlPath, $queryParams=null, $ouputFormat='xml') {
//////////////////////////////////////////////////////
// DO NOT REQUIRE AUTHENTICATION FOR THIS METHOD
//////////////////////////////////////////////////////
//        if(!authenticateAsOwner($queryParams))
//        {
//            header("HTTP/1.0 401 Unauthorized");
//            return;
//        }

        header("Content-Type: application/xml");
        
        $fwUInfoObj = new FirmwareInfo();
    	$result = $fwUInfoObj->getAvailPackages();
        $currentVersion = "";
        if($result !== NULL){
    					
            $xml = new NasXmlWriter();
            $xml->push('firmware_info');
    		
    		// current firmware info
            $xml->push('current_firmware');
                foreach($result['current']['package'] as $package){
                    $xml->push('package');
                    $xml->element('name', $package['name']);
                    $xml->element('version', $package['version']);
                    $xml->element('description', $package['description']);
        			$xml->element('package_build_time', $package['package_build_time']);
        			$xml->element('last_upgrade_time', $package['last_upgrade_time']);
        			$xml->pop();
        			// store the current fw version for latter
        			$currentVersion = $package['version'];
                }
    		$xml->pop();
    
    		// update firmware info
    		$xml->push('firmware_update_available');
        		$xml->element('available', $result['update']['available']);
                foreach($result['update']['package'] as $updatePackage){
                    if (empty($updatePackage['version'])) continue;
                    $xml->push('package');
        			$xml->element('name', $updatePackage['name']);
        			$xml->element('version', $updatePackage['version']);
        			$xml->element('description', $updatePackage['description']);
                    $xml->pop();
                }
            $xml->pop();
    
            // upgrades firmware info
            $xml->push('upgrades');
            $available = $result['upgrades']['available'];
            $xml->element('available', $available);
            $xml->element('message', $result['upgrades']['message']);

            // get legacy update server info: delete this when new server has test cases ready.
//        	$imageLink="";
//           foreach($result['update']['package'] as $updatePackage){
//                if (empty($updatePackage['version'])) continue;
//    	        $xml->push('upgrade');
//    			//if ( $updatePackage['name'] == "apnc") {
//    			//    $imageLink = 'http://websupport.wdc.com/firmware/list.asp?type=AP1NC&#38;fw='.$currentVersion;
//    			//}
//    			$xml->element('version', $updatePackage['version']);
//                //$xml->element('image', $imageLink);
//                $xml->element('filesize', 'unknown');
//                $xml->element('releasenotes', 'http://www.wdc.com/wdproducts/updates/?family=wdfmb_live');
//                $xml->element('message', "");
//                $xml->pop();
//            }

            // upgrade info
    		foreach($result['upgrades']['upgrade'] as $upgradePackage){
                if (empty($upgradePackage['version'])) continue;
    	        $xml->push('upgrade');
                $xml->element('version', $upgradePackage['version']);
                $xml->element('image', $upgradePackage['image']);
                $xml->element('filesize', $upgradePackage['filesize']);
                $xml->element('releasenotes', $upgradePackage['releasenotes']);
                $xml->element('message', $upgradePackage['message']);
    	        $xml->pop();
            }
            $xml->pop(); // </upgrades>
    		$xml->pop(); // </firmware_info>
    		
            echo $xml->getXml();
            
    		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SUCCESS');
    	} else {
            //Failed to collect info
    		$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'SERVER_ERROR');
    		header("HTTP/1.0 500 Internal Server Error");
        }
    }


	function put($urlPath, $queryParams=null, $ouputFormat='xml'){
		if(!authenticateAsOwner($queryParams))
		{
			header("HTTP/1.0 401 Unauthorized");
			return;
		}
		
        $fwInfoObj = new FirmwareInfo();
		$result = $fwInfoObj->modifyUpdateAvailable();

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
        header("Allow: GET PUT");
        header("HTTP/1.0 405 Method Not Allowed");
    }

    function delete($urlPath, $queryParams=null, $ouputFormat='xml'){
        header("Allow: GET PUT");
        header("HTTP/1.0 405 Method Not Allowed");
    }

}

/*
 var $testFirmwareInfoXml = "
<firmware_info>
  <current_firmware>
    <package>
      <name>MyBookLive </name>
      <version>01.03.03</version>
      <description>Core F/W</description>
      <package_build_time>1304583882</package_build_time>
      <last_upgrade_time>1304631079</last_upgrade_time>
    </package>
  </current_firmware>
  <firmware_update_available>
    <available>true</available>
    <package>
      <name>apnc</name>
      <version>01.04.06</version>
      <description>MyBookLive core firmware</description>
    </package>
  </firmware_update_available>
  <upgrades>
    <available>true</available>
    <message></message>
     <upgrade>
     <version>01.04.06.00</version>
     <image>http://download.wdc.com/nas/sgsb01020600.msi</image>
     <filesize>635345</filesize>
     <releasenotes>http://www.wdc.com/wdproducts/updates/?family=wdfmb_live&#38;lang=eng</releasenotes>
     <message></message>
    </upgrade>
    <upgrade>
     <version>02.00.10</version>
     <image>http://download.wdc.com/nas/sgsb01020600.msi</image>
     <filesize>635345</filesize>
     <releasenotes>http://www.wdc.com/wdproducts/updates/?family=wdfmb_live</releasenotes>
     <message>http://www.wdc.com/wdproducts/updates/?family=wdfmb_live</message>
    </upgrade>
  </upgrades>
</firmware_info>
         ";
*/
/*
<SoftwareUpgrade xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
         xsi:noNamespaceSchemaLocation="WD_NAS_SoftwareUpgrade.xsd">
<Message>http://support.wdc.com/nas/msg.asp?f=sgsb01020000msg&lang=eng</Message>
<Upgrades>
  <Upgrade>
  <Version>01.02.06.00</Version>
  <Image>http://download.wdc.com/nas/sgsb01020600.msi</Image>
  <FileSize>635345</FileSize>
  <ReleaseNotes>http://support.wdc.com/nas/rel.asp?f=sgsb01020600rel&lang=eng</ReleaseNotes>
  </Upgrade>
  <Upgrade>
  <Version>01.03.00.00</Version>
  <Image>http://download.wdc.com/nas/sgsb01030000.msi</Image>
  <FileSize>715048</FileSize>
  <ReleaseNotes>http://support.wdc.com/nas/rel.asp?f=sgsb01030000rel&lang=eng</ReleaseNotes>
  </Upgrade>
</Upgrades>
</SoftwareUpgrade>
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
