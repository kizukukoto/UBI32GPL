<?php

class DiskStatus{

    var $disk = '';
    var $size = '';
    var $description = '';
    var $status = '';

    function DiskStatus() {
    }
    
    function getStatus(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

        $this->disk = "Disk";
        $this->size = 100;
        $this->description = "Big Disk";
        $this->status = "good";

//return NULL;  //Error case
        return array(
            'disk' => "$this->disk",
            'size' => "$this->size",
            'description' => "$this->description",
            'status' => "$this->status",
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