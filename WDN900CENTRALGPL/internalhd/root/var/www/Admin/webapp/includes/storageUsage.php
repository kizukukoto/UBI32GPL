<?php

class StorageUsage{

    function StorageUsage() {
    }

    function getStorageUsage(){
        //!!!This where we gather up response  
        //!!!Return NULL on error
        //NOTE: $useShellScriptForTotalSpace is set purposely to true because disk_total_space() below
        //will return the size of the mount point even if user data mount failed.  PHP does not have a
        //mount function to see if user data has been mounted.  Since we'd have to shell out to do 'mount', we
        //might as well use the shell script to do all the work.
        $useShellScriptForTotalSpace = true;
        $useShellScriptForNasVideoSize = false;
        $useShellScriptForNasPhotoSize = false;
        $useShellScriptForNasMusicSize = false;
        $useShellScriptForShareNames = false;
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false)
        {
            $useShellScriptForTotalSpace = true;
            $useShellScriptForNasVideoSize = true;
            $useShellScriptForNasPhotoSize = true;
            $useShellScriptForNasMusicSize = true;
            $useShellScriptForShareNames = true;
        }
        else 
        {
            if (!isset($nasConfig['admin']['DATA_VOLUME']))
            {
                $useShellScriptForTotalSpace = true;
            }
            if (!isset($nasConfig['admin']['VIDEO_SIZE_FILE']))
            {
                $useShellScriptForNasVideoSize = true;
            }
            if (!isset($nasConfig['admin']['PHOTO_SIZE_FILE']))
            {
                $useShellScriptForNasPhotoSize = true;
            }
            if (!isset($nasConfig['admin']['MUSIC_SIZE_FILE']))
            {
                $useShellScriptForNasMusicSize = true;
            }
            if (!isset($nasConfig['admin']['SHARE_NAME_FILE']))
            {
                $useShellScriptForShareNames = true;
            }
        }
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForTotalSpace)
        {
            $diskTotalSpace = disk_total_space($nasConfig['admin']['DATA_VOLUME']);
            $diskFreeSpace = disk_free_space($nasConfig['admin']['DATA_VOLUME']);
            $diskUsedSpace = $diskTotalSpace - $diskFreeSpace;
            $size = ceil((float)$diskTotalSpace/1000/1000/1000);
            $used = ceil((float)$diskUsedSpace/1000/1000/1000);
        }
        else
        {
    		unset($output);
    		exec("sudo getDataVolumeUsage.sh", $output, $retVal);
    		if($retVal !== 0) 
    		{
    			return NULL;
    		}		
            if((!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0GB 0GB";
            }
    		$space = explode(" ", $output[0]);
    			
    		$size = trim($space[1]);
    		
    		$size = trim($size, "GB");
    		
    		$used = trim($space[0]);
    			
    		$used = trim($used, "GB");
        }

        if (SKIP_SHELL_SCRIPT && !$useShellScriptForNasVideoSize)
        {
            unset($output);
            $output = @file_get_contents($nasConfig['admin']['VIDEO_SIZE_FILE']);
            if ($output === false){
                $useShellScriptForNasVideoSize = true;
            }
            else if (empty($output))
            {
                $output = "0";
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForNasVideoSize)
        {
		    unset($output);
    		exec("sudo cat /var/local/nas_file_tally/video_size", $output, $retVal);	
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output = "0";
            }
            else
            {
                $output = $output[0];
            }
        }
		$videoSize = trim($output);

		if (SKIP_SHELL_SCRIPT && !$useShellScriptForNasPhotoSize)
		{
            unset($output);
		    $output = @file_get_contents($nasConfig['admin']['PHOTO_SIZE_FILE']);
            if ($output === false)
            {
                $useShellScriptForNasPhotoSize = true;
            }
            else if (empty($output))
            {
                $output = "0";
            }
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForNasPhotoSize)
		{
		    unset($output);
            exec("sudo cat /var/local/nas_file_tally/photos_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output = "0";
            }
            else
            {
                $output = $output[0];
            }
		}
		$photosSize = trim($output);

		if (SKIP_SHELL_SCRIPT && !$useShellScriptForNasMusicSize)
		{
		    unset($output);
            $output = @file_get_contents($nasConfig['admin']['MUSIC_SIZE_FILE']);
            if ($output === false)
            {
                $useShellScriptForNasMusicSize = true;
            }
            else if (empty($output))
            {
                $output = "0";
            }
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForNasMusicSize)
		{
            unset($output);
            exec("sudo cat /var/local/nas_file_tally/music_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output = "0";
            }
            else
            {
                $output = $output[0];
            }
        }
		$musicSize = trim($output);

//		unset($output);
//		exec("sudo cat /var/local/nas_file_tally/documents_size", $output, $retVal);
//      if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
//          $output[0] = "0";
//      }
//		$documentsSize = trim($output[0]);		
//
//		unset($output);
//		exec("sudo cat /var/local/nas_file_tally/unknown_size", $output, $retVal);
//      if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
//          $output[0] = "0";
//      }
//		$unknownSize = trim($output[0]);		

		$otherSize = $used - $videoSize - $photosSize - $musicSize;
		
		$storageUsage = array(  'size' => $size,
			'usage' => $used,
			'video' => $videoSize,
			'photos' => $photosSize,
			'music' => $musicSize,
			'other' => $otherSize,
			'share' => array());

		// first get the share names
        $shareNames = array();
        if (SKIP_SHELL_SCRIPT || $useShellScriptForShareNames)
        {
            $handle = @fopen($nasConfig['admin']['SHARE_NAME_FILE'], 'r');
            if ($handle === false)
            {
                $useShellScriptForShareNames = true;
            }
            else
            {
            	while (($buffer = fgets($handle)) !== false)
            	{
            		if ($buffer[0] != '#')
            		{
            			$data = explode(':', substr($buffer, strpos($buffer, ']/')+2));
            			$values = explode('/', $data[0]);
            			if ($values[0] == 'shares' && isset($values[1]) && $data[1] == '*' && $data[2] == 'RWBEX')
            			{
            				$shareNames[] = $values[1];
            			}
            		}
            	}
            	fclose($handle);
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForShareNames) 
        {
            unset($output);
        	exec("sudo /usr/local/sbin/getShares.sh \"all\"", $output, $retVal);
        	if($retVal !== 0) {
        	    return 'SERVER_ERROR';
        	}
        	$shareNames = $output;
        }
//		$shareNames = $output;
				
		$shares = array();
		
		// loop through each share name and get description and create share instance
		foreach($shareNames as $testShare){			
//NOTE: Share directories aren't give x permission, so this script can't access it directly even though the files are marked as having read access. ask about this later.
			$testShare = trim($testShare);
			
			unset($output);
			exec("sudo cat /var/local/nas_file_tally/\"$testShare\"/total_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0";
            }
			$shareTotalSize = trim($output[0]);		

/*
        unset($output);
        $output = @file_get_contents('/var/local/nas_file_tally/'.$testShare.'/total_size');
        if ($output === false){
            $output = "0";
        }
        $shareTotalSize = trim($output);
*/


			unset($output);
			exec("sudo cat /var/local/nas_file_tally/\"$testShare\"/video_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0";
            }
			$shareVideoSize = trim($output[0]);		

/*
        unset($output);
        $output = @file_get_contents('/var/local/nas_file_tally/'.$testShare.'/video_size');
        if ($output === false){
            $output = "0";
        }
        $shareVideoSize = trim($output);
*/

			unset($output);
			exec("sudo cat /var/local/nas_file_tally/\"$testShare\"/photos_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0";
            }
			$sharePhotosSize = trim($output[0]);

/*
        unset($output);
        $output = @file_get_contents('/var/local/nas_file_tally/'.$testShare.'/photos_size');
        if ($output === false){
            $output = "0";
        }
        $sharePhotosSize = trim($output);
*/
			unset($output);
			exec("sudo cat /var/local/nas_file_tally/\"$testShare\"/music_size", $output, $retVal);
            if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
                $output[0] = "0";
            }
			$shareMusicSize = trim($output[0]);		

/*
        unset($output);
        $output = @file_get_contents('/var/local/nas_file_tally/'.$testShare.'/music_size');
        if ($output === false){
            $output = "0";
        }
        $shareMusicSize = trim($output);
*/

			$shareOtherSize = $shareTotalSize - $shareVideoSize - $sharePhotosSize - $shareMusicSize;
			
			array_push($shares, 
				array('sharename' => "$testShare", 'usage' => $shareTotalSize, 
				'video' => $shareVideoSize, 'photos' => $sharePhotosSize, 'music' => $shareMusicSize, 'other' => $shareOtherSize));
							
		}
		return array(
			'size' => $size,
			'usage' => $used,
            'video' => $videoSize,
            'photos' => $photosSize,
            'music' => $musicSize,
			'other' => $otherSize,
            'share' => $shares
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
