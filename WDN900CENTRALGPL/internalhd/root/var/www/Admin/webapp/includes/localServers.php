<?php

class LocalServers{

    var $local_server = array('local_server' => array());

    function LocalServers() {
    }
    
    function getList(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
        //Steve has prototyped this.

        array_push($this->local_server['local_server'], array( 'name' => "local1", 'ip_address' => "192.168.1.1"));
        array_push($this->local_server['local_server'], array( 'name' => "local2", 'ip_address' => "192.168.1.2"));
        array_push($this->local_server['local_server'], array( 'name' => "local3", 'ip_address' => "192.168.1.3"));

//return NULL;  //Error case
        return($this->local_server);
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