<?php

class CopyStatus{

    var $files_copied = 0;
    var $size_copied = 0;
    var $total_files = 0;
    var $total_size = 0;
    var $file_name = '';
    var $in_progress = "false";

    function CopyStatus() {
    }
    
    function getStatus(){

        //!!!This where we gather up response  
        //!!!Return NULL on error

        $this->files_copied = 0;
        $this->size_copied = 0;
        $this->total_files = 0;
        $this->total_size = 0;
        $this->file_name = "Current.txt";
        $this->in_progress = "false";

//return NULL;  //Error case
        return( array(
                    'files_copied' => "$this->files_copied",
                    'size_copied' =>  "$this->size_copied",
                    'total_files' => "$this->total_files",
                    'total_size' => "$this->total_size",
                    'file_name' => "$this->file_name",
                    'in_progress' => "$this->in_progress"
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