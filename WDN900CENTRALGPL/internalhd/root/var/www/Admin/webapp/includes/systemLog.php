<?php

class SystemLog{

    function SystemLog() {
    }
    
    function getLog(){
        //Create a file of all logs and return it/put it someplace where upper layer can get to it

		unset($output);
		exec("sudo /usr/local/sbin/getSystemLog.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return NULL;
		}		
		return (array('path_to_log' => $output[0] ));


    }
	 
	function sendLog(){
		//Create a file of all logs and return it/put it someplace where upper layer can get to it

		unset($output);
		exec("sudo /usr/local/sbin/sendLogToSupport.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return NULL;
		}		
		if ($output[0] === 'server_connection_failed')
		{
			return (array('transfer_success' => "failed", 'logfilename' => '' ));			
		}
		else
		{
			return (array('transfer_success' => "succeeded", 'logfilename' => $output[0] ));
		}

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
