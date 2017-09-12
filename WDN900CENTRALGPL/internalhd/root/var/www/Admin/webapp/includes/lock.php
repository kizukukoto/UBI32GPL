<?php

define ('__LOCK_DIR__', '/var/lock/');

require_once('logMessages.php');

class Lock{

    var $name;
	var $logObj;
    
    function Lock($name){
        $this->name = $name;
		$this->logObj = new LogMessages();
    }

    function Get() {
        while (TRUE) {
            if (($fd = @fopen(__LOCK_DIR__.$this->name, 'x')) !== FALSE) {
                //Got lock
                @fwrite($fd, getmypid());
                @fclose($fd);								
                return;
            } else {
                //Locking failed check for stale process
                $lockingPID = trim(file_get_contents(__LOCK_DIR__.$this->name)); 
                $pids = explode( "\n", trim( `ps -e | awk '{print $1}'` ) ); 

			//	$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "lock is set");
				//Check to see that process holding lock is still running.
                if( in_array( $lockingPID, $pids ) ) {
                    //Process is still running
                    usleep(250000);  // Wait 1/4 second
                } else {
                    $this->Release();
                }
            }
        }
    }

    function Release() {
        if (file_exists(__LOCK_DIR__.$this->name)) {
            @unlink(__LOCK_DIR__.$this->name);
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
