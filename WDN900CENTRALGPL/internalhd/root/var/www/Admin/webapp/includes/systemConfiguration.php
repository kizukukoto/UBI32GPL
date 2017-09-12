<?php

class systemConfiguration{

    function systemConfiguration() {
    }
    
    function getConfig(){
        //Create a file of all the config and return it/put it someplace where upper layer can get to it

		unset($output);
		exec("sudo /usr/local/sbin/saveConfigFile.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return NULL;
		}		

		$pathToFile = $output[0];
		
		return (array('path_to_config' => $pathToFile));                     //$output[0] ));
	}

    function modifyConfig($changes){

		if( !isset($changes["filepath"]) ){
			return 'BAD_REQUEST';			
		}
			
		unset($output);
		exec("sudo /usr/local/sbin/restoreConfig.sh \"{$changes["filepath"]}\"", $output, $retVal);
		if($retVal !== 0) 
		{	
			return 'SERVER_ERROR';
		}		

		$statusInfo = explode(":", $output[0]);
		
		$xml = new NasXmlWriter();
		$xml->push('restore_status');
		$xml->element('status_code', $statusInfo[0]);
		$xml->element('status_description', $statusInfo[1]);
		$xml->pop();
		return $xml->getXml();
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
