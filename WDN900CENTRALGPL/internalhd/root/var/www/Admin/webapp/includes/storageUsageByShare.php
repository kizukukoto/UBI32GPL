<?php

class StorageUsageByShare{

    function StorageUsageByShare() {
    }
    
    function getStorageUsage(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

//return NULL;  //Error case
        return array(
            'share' => array(
                array('sharename' => "Share Name1", 'usage' => 100),
                array('sharename' => "Share Name2", 'usage' => 150),
                array('sharename' => "Share Name3", 'usage' => 1),
                )
            );
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