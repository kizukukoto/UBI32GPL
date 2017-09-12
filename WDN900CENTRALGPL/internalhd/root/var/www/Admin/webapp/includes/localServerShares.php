<?php

class LocalServerShares{

    var $local_share = array('local_share' => array());

    function LocalServerShares() {
    }
    
    function getLocalShares($ip, &$error){
        //!!!This where we gather up response  
        //!!!Return NULL on error
        //    If $ip doesn't respond, set &$error = NOT_FOUND
/*
 //Steve has prototyped this.  SAMBA has support to get list of local shares

 array_push($this->local_share['local_share'], array( 'name' => "share1", 'public' => "true"));
 array_push($this->local_share['local_share'], array( 'name' => "share2", 'public' => "false"));
 array_push($this->local_share['local_share'], array( 'name' => "share3", 'public' => "false"));

//return NULL;  //Error case Also set &$error
return($this->local_share);
*/
        $error = NULL; //'NOT_FOUND';
        $error = 'NOT_FOUND';
        return NULL;
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