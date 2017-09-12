<?php

class SmartTest{

    var $percent_complete = 0;
    var $status = '';

    function SmartTest() {
    }
    
    function getResults(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
		
		unset($output);
		exec("sudo /usr/local/sbin/getSmartTestStatus.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return NULL;
		}		
				

		$status = explode(" ", $output[0]);
		$this->status = $status[0];
		
		
		if (isset($status[1]))
		{
			$this->percent_complete = $status[1];			
		}
		else
		{
			$this->percent_complete = '';			
		}
			
        return( array( 
                    'percent_complete' => $this->percent_complete,
                    'status' => $this->status
                    ));
    }

    function start($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["test"]) ){
            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }

		if (strcasecmp($changes["test"], "stop") ===0 )
		{
			unset($output);
			exec("sudo /usr/local/sbin/cmdSmartTest.sh abort", $output, $retVal);
			if($retVal !== 0) 
			{
				return 'SERVER_ERROR';
			}		

			return 'SUCCESS';
		}

		// if we have gotten here then the user wants to start a test, 
		// must verify there is not already a test going
		unset($output);
		exec("sudo /usr/local/sbin/getSmartTestStatus.sh", $output, $retVal);
		if($retVal !== 0) 
		{
			return 'SERVER_ERROR';
		}		
			
		$status = explode(" ", $output[0]);

			
		if (strcasecmp(trim($status[0]), "inprogress") ===0 )
		{
			return 'BAD_REQUEST';			
		}

		if (strcasecmp($changes["test"], "start_short") ===0 )
		{
			$testCommand = "short";						
		}
		else
		{
			$testCommand = "long";			
		}
		
		
		
		unset($output);
		exec("sudo /usr/local/sbin/cmdSmartTest.sh \"$testCommand\"", $output, $retVal);
		if($retVal !== 0) 
		{
			return 'SERVER_ERROR';
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
