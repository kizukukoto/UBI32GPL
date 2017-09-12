<?php

class MediaServerDatabase {

    var $version = '';
	var $time_db_update = '';
    var $music_tracks = '';
    var $pictures = '';
    var $videos = '';
	var $scanInProgress = '';

    function MediaServerDatabase() {
    }
    
    function getStatus(){
        //!!!This where we gather up response  
        //!!!Return NULL on error

		// get the network configuration
		unset($MediaDBoutput);
		exec("sudo /usr/local/sbin/getMediaServerDbInfo.sh", $MediaDBoutput, $retVal);
		if($retVal !== 0) {
			return NULL;
		}			
				
		$version = explode("=", $MediaDBoutput[0]);		
		$this->version = trim($version[1], '"');

		$musicTracks = explode("=", $MediaDBoutput[1]);
		$this->music_tracks = trim($musicTracks[1], '"');

		$pictures = explode("=", $MediaDBoutput[2]);
		$this->pictures = trim($pictures[1], '"');

		$videos = explode("=", $MediaDBoutput[3]);
		$this->videos = trim($videos[1], '"');

		$timeDBUpdate = explode("=", $MediaDBoutput[4]);
		$this->time_db_update = trim($timeDBUpdate[1], '"');

		$scanInProgress = explode("=", $MediaDBoutput[5]);
		$this->scanInProgress = trim($scanInProgress[1], '"');

		return( array( 
			'version' => "$this->version",
			'time_db_update' => "$this->time_db_update",
			'music_tracks' => "$this->music_tracks",
			'pictures' => "$this->pictures",
			'videos' => "$this->videos",
			'scan_in_progress' => "$this->scanInProgress",
			));
    }

    function config($changes) {
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["database"]) ){
            return 'BAD_REQUEST';
        }
        //Verify changes are valid
        if(FALSE){
            return 'BAD_REQUEST';
        }
		
        //Actually do change
		unset($mediaDBOutput);
		exec("sudo /usr/local/sbin/cmdMediaServer.sh \"{$changes["database"]}\"", $mediaDBOutput, $retVal);
		if($retVal !== 0) {
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
