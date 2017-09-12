<?php
 
class CopyLocalServerShare{
    
    var $logObj;
    
    function CopyLocalServerShare() {
        $this->logObj = new LogMessages();
    }

    function copy($source_des){
        
        $this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId, username=$username, password=***rd)");
        
        //Require entire representation
        if( !isset($source_des['destination_share']) ||
            !isset($source_des['abort_copy']) ||
            !isset($source_des['source_share']['server_ip_address']) ||
            !isset($source_des['source_share']['name']) ||
            !isset($source_des['source_share']['username']) ||
            !isset($source_des['source_share']['password']) ){

            $this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "Return: BAD_REQUEST");
            return 'BAD_REQUEST';
        }
        //Verify values are valid --Only allow one copy at a time.
        if(FALSE){
            return 'BAD_REQUEST';
        }

        //Actually manage copy

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