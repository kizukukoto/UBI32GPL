<?php

class MediaServerBlockedList{

    var $device = array('device' => array());

    function MediaServerBlockedList() {
    }
    
    function getBlockList(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
        array_push($this->device['device'], array( 'mac_address' => "Mac123"));
        array_push($this->device['device'], array( 'mac_address' => "Mac124"));
        array_push($this->device['device'], array( 'mac_address' => "Mac125"));

//return NULL;  //Error case
        return($this->device);
    }

    function modifyBlockList($changes) {
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["device"]) ){
            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }

        //Actually do change

        return 'SUCCESS';
//        return 'SERVER_ERROR';

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