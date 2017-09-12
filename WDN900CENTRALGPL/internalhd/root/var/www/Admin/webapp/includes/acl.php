<?php
class Acl{
    var $shareName = '';
    var $remote_access = '';
    var $public_access = '';
    var $media_serving = '';
    var $fullaccess_user_arr = array(); //Need to push in array('user_name' => 'userName')
    var $readonly_user_arr = array(); //Need to push in array('user_name' => 'userName')
    
    function Acl($shareName, $remote_access, $public_access, $media_serving, $fullaccess_user_arr = array(), $readonly_user_arr = array() ) {
        $this->shareName = $shareName;
        $this->remote_access = $remote_access;
        $this->public_access = $public_access;
        $this->media_serving = $media_serving;
        $this->fullaccess_user_arr = $fullaccess_user_arr;
        $this->readonly_user_arr = $readonly_user_arr;
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