<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

require_once("util.inc");
require_once("globalconfig.inc");

$albumConfig = getGlobalConfig("albums");
$globalConfig = getGlobalConfig("global");


$MEDIAFILTERS = 
		array('image' => $albumConfig['IMAGE_FILTER'], 
			  'video' => $albumConfig['VIDEO_FILTER'], 
			  'flashvideo' => $albumConfig['FLASH_MOVIES_FILTER'], 
			  'audio' => $albumConfig['AUDIO_FILTER']);

$albumConfig['MEDIAFILTERS'] = $MEDIAFILTERS;
$serverName = isset($_SERVER['SERVER_NAME']) ? $_SERVER['SERVER_NAME'] : "localhost";
$albumConfig['URLPREFIX'] = "http://" . $serverName . "/";

$GLOBALS['albumConfig'] = $albumConfig;

$IPTCTAGS = 
    array('index' => 'APP13',
          'title' => '2#005',  
    	  'description' => '2#120',
          'created' => '2#055',
          'author' =>  '2#080',
          'bylinetitle' => '2#080',               
          'locationcode' => '2#026',
          'locationname' => '2#027',
          'datecreated' => '2#055',
          'timecreated' => '2#060',
          'digitalcreationdate'  => '2#062',
          'digitalcreationtime'  => '2#063',
          'imagetype'  => '2#130',
          'imageorientation' => '2#131'
    );
    
 $GLOBALS['iptcTags'] = IPTCTAGS;   

/**
  * Create a transcoding filter - the values will be used to select the trancoded files that match the desired params or are compatible
  * with them (e.g. if asked for 720 x 560 resolution, and only 640 x 480 and 1024 x 768 are availble, then next-smallest res will be returned, 
  * in this case 640 x 480.
  * 
  * returns an associative array of transcoding filter settings.
  *
  */

function makeTranscodingFilter($fileType, $width, $height, $cdepth, $vbrate, $abitrate, $afreq, $thumbWidth, $thumbHeight) 
{
	//obviously, there needs to be some validation here.	

	$transcodingFilter = 
			array("width" => $width, 
				  "height" => $height, 
				  "cdepth" => $cdepth, 
				  "vbrate" => $vbrate, 
				  "abitrate" => $abitrate, 
				  "afreq" => $afreq, 
				  "thumbwidth" => $thumbWidth,
				  "thumbheight"	=> $thumbHeight);
			
	return $transcodingFilter;
}

/**
  *
  * Set transcoding filter for the current user session - the filter will be used for all subsequent access calls until a new one is set 
  *
  */

function setTranscodingFilter($transcodingFilter ) 
{
	if (!($transcodingFilter === NULL))
	{
		$GLOBALS['transcodingFilter'] = $tfKey;
	}
}

/**
 * Check if file extension matches those in the file filter, returns TRUE if a match is found, else FALSE 
 * 
 */

function matchesFileFilter($file, $fileFilter)
{
	if (($fileFilter === NULL) || (sizeof($fileFilter)==0)) {
		return false; //no file filter == 'match none'
	}
	if (strcmp($fileFilter[0], "*") == 0)
		return true;
		
	$ext = end(explode(".", $file));
	if (isset($ext)) {
		return in_array($ext, $fileFilter);
	}
	return false;
}

/**
 * Check if file attribues match the given transcoding filter. Returns true if they match else false
 */

function matchesTranscodingFilter($content, $transcodingFilter)
{
	//TBA
	return TRUE;
}

/**
 * Directory access functions
 *
 **/

/**
 * Get a list of all albums under the given dir path, returns an empty list if none found, else
 * returns the following array for each album found:
 * 
 * array("title" => $title, "description" => $description, "owner" => "", "sharelist" => "", "albumid" => "");
 */

function getDirs($dirpath ) {
	$albums = array();
	if (file_exists($dirpath) && is_dir($dirpath)) {
		if ($dh = opendir($dirpath)) {
        	while (($file = readdir($dh)) !== false) {
				$filepath = $dirpath.$file;
				if ( file_exists($filepath) && is_dir($filepath) &&  ($file[0] != '.' ) ) {
					array_push($albums, $file);
				}
			}
        }
		else {
			//failed to open DIR - maybe no access rights
			return NULL;
		}
      	closedir($dh);
		return $albums;
	}
}

//
// Basic Access functions
//

/**
 * Returns thumbnail path for given file path, thumbnail should be under ../thumbs
 * 
 */

function getThumbnailPath($file, $transcodingFilter = NULL) {
	$albumConfig = $GLOBALS["albumConfig"];
	
	$thumbpath = str_replace($albumConfig['CONTENTFOLDER'], $albumConfig['THUMBNAILFOLDER'], $file);
	$thumbpath = $thumbpath . ".jpg";
	//get all files that start with this path
	//TBA

	//select thumbnail file that matches transcoding filter
	//TBA
	
	return $thumbpath;
}

function getAlbumMetaData($album) {
	$albumConfig = $GLOBALS["albumConfig"];
	//$album = str_replace(" ", "_", $album);
	$dirpath = $albumConfig ['ALBUMSROOT'] . $album;
	
	$albumMetaData = array();
	$albumMetaData['title'] = "";
	$albumMetaData['description'] = "";
	$albumMetaData['image'] = "";
	
	if (file_exists($dirpath) && is_dir($dirpath)) {
		if ($dh = opendir($dirpath)) {
			$dirpath = $dirpath . "/";
			$albumTitleFile = $dirpath . $albumConfig['ALBUMTITLEFILE'];
			$albumDescriptionFile = $dirpath . $albumConfig['ALBUMDESCRIPTIONFILE'];
			$albumImageFile =   $dirpath . $albumConfig['ALBUMIMAGEFILE'];
			
			if (is_file($albumTitleFile) && file_exists($albumTitleFile)) {
				$fh = fopen($albumTitleFile, 'r');
				$albumMetaData['title'] =  stripLineEnd(fgets($fh));
				fclose($fh);
			}		
			if (is_file($albumDescriptionFile) && file_exists($albumDescriptionFile)) {
				$fh = fopen($albumDescriptionFile, 'r');
				while (!feof($fh)) {
					$description = $description . stripLineEnd(fgets($fh));
				}
				fclose($fh);
				$albumMetaData['description'] = $description;
			}		
			if (is_file($albumImageFile) && file_exists($albumImageFile)) {
				$albumMetaData['image'] = $albumConfig['URLPREFIX'] . $albumConfig['ALBUMSURIROOT'] . $album . "/" . $albumConfig['ALBUMIMAGEFILE'];	
			}
		}
	}
	return $albumMetaData;	
}

function getFileInfo($filePath)
{
	$fstat = stat($filepath);
	
	$mimetype = "";
	
	$extplusdot = strrchr($filepath,'.');
	if ($extplusdot) {
		$ext = substr($extplusdot,1);
		$prefix = substr($filepath, 0, strrpos($filepath, $extplusdot));
		$urlPrefix = substr($fileURI, 0, strrpos($fileURI, $extplusdot));
	} 
	else {
		$ext = NULL;
	}
	
	if (($ext != NULL) && in_array($ext, $dirConfig['IMAGE_FILTER'])) {
	    $iptcTags = $GLOBALS['iptcTags'];
		$imageinfo = array();
		$image = array(640,480);
		$imagesize = getimagesize($filepath, $imageinfo);
		if (isset($imageinfo[$iptcTags($iptcTags['index'])])) {
	    	$iptc = iptcparse($iptcTags($iptcTags['index']));
	    	if (is_array($iptc)) {
	        	$description = $iptc[$iptcTags['description']][0];
	    	}
	     	$created = $iptc[$iptcTags['created']][0];
	     	$author = $iptc[$iptcTags['author']][0];  //photog
	     	if ($author === NULL) {
	     		$author = $iptc[$iptcTags['bylinetitle']][0]; //credit_byline_title
	     	}
		}
		$mimetype = $imagesize['mime'];
	}
	else if ($ext != NULL ) {   
		if ( in_array($ext, $dirConfig['VIDEO_FILTER'])) {
			//it's a movie file, so set the size to the transoding params
			$imagesize[0] = $dirConfig['RESOLUTION'][0];
			$imagesize[1] = $dirConfig['RESOLUTION'][1];
			$mimetype = $dirConfig ['VIDEO_MIMETYPES'][$ext];
		}
		else if (in_array($ext, $dirConfig['AUDIO_FILTER'])) {
			$mimetype = $dirConfig['AUDIO_MIMETYPES'][$ext];
		}
	}
	$thumbPath = getThumbnailPath($prefix);
	
	if (is_file($thumbPath) && file_exists($thumbPath)) { 
		$thumbURI = getThumbnailPath($urlPrefix);
	}
	else { //supply a default thumbnail for the media type
		//if (strpos($mimetype, "audio/") !== false )
		{
			$thumbPath = getWebAppPath . "/htdocs/images/audiothumb.jpg" ;
			$thumbURI = $dirConfig['URLPREFIX'] . "images/audiothumb.jpg";
		}
	}
	$thumbinfo = array();
	
	//$thumbsize = getimagesize($thumbPath, $thumbinfo);
	$thumbsize = array(80,60);
	
	if ($created === NULL)
	    {
	    $created = $fstat["ctime"];
	    }
	    //get permissions
	    $perms = fileperms($filepath);
	    $owneraccess = ($perms & 0x0080) ? "w" : (($perms & 0x0100) ? "r" : n);   //r,w,n = none
	    $otheraccess = ($perms & 0x0002) ? "w" : (($perms & 0x0004) ? "r" : n);   //r,w,n = none
	    $readonly = !is_writable($filepath);
	    
	$info = 
		array("file_path" => $filepath, 
			  "file_uri" => $fileURI, 	
			  "file_id" => $fstat["ino"],
			  "file_size" => $fstat["size"], 
			  "file_created" => $created, 
			  "file_modified" => $fstat["mtime"], 
			  "file_readonly" => $readonly, 
			  "file_owner" => $fstat["uid"],	
			  "file_owneraccess" => $owneraccess,
			  "file_otheraccess" =>	$otheraccess,
			  "file_isdir" => (($perms & 0x4000) == 0x4000), 
	          "file_width" => $imagesize[0], 
	          "file_height" => $imagesize[1], 
	          "file_encoding" => "", 
			  "thumb_path" => $thumbPath,
			  "thumb_uri"  => $thumbURI,
			  "thumb_width"	=> $thumbsize[0],
		      "thumb_height" => $thumbsize[1],
			  "title" => "", 
	          "description" => $description, 
	          "author" => $author, 
	          "mime_type" => $mimetype, 
	          "duration" => NULL, 
	          "frequency" => NULL, 
	          "bitrate" => NULL );
	return $info;	
}

function getDirContents($dirpath, $dirname, $fileFilter = NULL, $transcodingFilter = NULL, $recurse = FALSE   ) 
{
	$dirConfig = $GLOBALS["albumConfig"];
	
	if ($transcodingFilter === NULL) {
		//get transcoding filter for session
		$transcodingFilter = $GLOBALS['transcodingFilter'];
	}
	
	//cycle through the files in the folder, get the file and image attributes
	
	$dirContents = array();
	
	if (file_exists($dirpath) && is_dir($dirpath)) {
		if ($dh = opendir($dirpath)) {
        	while ( !(($file = readdir($dh)) === false)) {
				$filepath = $dirpath . "/". $file;
				//echo "Dir file: " . $filepath. "\n";

				if ( file_exists($filepath) && matchesFileFilter($file, $fileFilter)) {
					$fileURI = $dirConfig['URLPREFIX'] . $dirConfig['ALBUMSURIROOT'] . $dirname . "/" . $dirConfig ['CONTENTFOLDER'] . "/" .$file;
					$fstat = stat($filepath);
					
					$mimetype = "";
					
					$extplusdot = strrchr($filepath,'.');
					if ($extplusdot) {
						$ext = substr($extplusdot,1);
						$prefix = substr($filepath, 0, strrpos($filepath, $extplusdot));
						$urlPrefix = substr($fileURI, 0, strrpos($fileURI, $extplusdot));
					} 
					else {
						$ext = NULL;
					}
					
					if (($ext != NULL) && in_array($ext, $dirConfig['IMAGE_FILTER'])) {
						$imageinfo = array();
						$image = array(640,480);
						$imagesize = getimagesize($filepath, $imageinfo);
						if (isset($imageinfo["APP13"])) {
    						$iptc = iptcparse($info["APP13"]);
    						if (is_array($iptc)) {
        						$description = $iptc["2#120"][0];
    						}
    					 	$created = $iptc["2#055"][0];
    					 	$author = $iptc["2#080"][0];  //photog
    					 	if ($author === NULL) {
    					 		$author = $iptc["2#085"][0]; //credit_byline_title
    					 	}
						}
						$mimetype = $imagesize['mime'];
					}
					else if ($ext != NULL ) {   
						if ( in_array($ext, $dirConfig['VIDEO_FILTER'])) {
							//it's a movie file, so set the size to the transoding params
							$imagesize[0] = $dirConfig['RESOLUTION'][0];
							$imagesize[1] = $dirConfig['RESOLUTION'][1];
							$mimetype = $dirConfig ['VIDEO_MIMETYPES'][$ext];
						}
						else if (in_array($ext, $dirConfig['AUDIO_FILTER'])) {
							$mimetype = $dirConfig['AUDIO_MIMETYPES'][$ext];
						}
					}
					$thumbPath = getThumbnailPath($prefix);
					
					if (is_file($thumbPath) && file_exists($thumbPath)) { 
						$thumbURI = getThumbnailPath($urlPrefix);
					}
					else { //supply a default thumbnail for the media type
						//if (strpos($mimetype, "audio/") !== false )
	                    {
	                    	$thumbPath = getWebAppPath . "/htdocs/images/audiothumb.jpg" ;
							$thumbURI = $dirConfig['URLPREFIX'] . "images/audiothumb.jpg";
						}
					}
					$thumbinfo = array();
					
					//$thumbsize = getimagesize($thumbPath, $thumbinfo);
					$thumbsize = array(80,60);
					
					if ($created === NULL) {
    					$created = $fstat["ctime"];
    				}
    				//get permissions
    				$perms = fileperms($filepath);
    				$owneraccess = ($perms & 0x0080) ? "w" : (($perms & 0x0100) ? "r" : n);   //r,w,n = none
    				$otheraccess = ($perms & 0x0002) ? "w" : (($perms & 0x0004) ? "r" : n);   //r,w,n = none
    				$readonly = !is_writable($filepath);
    				
					$content = 
						array("file_path" => $filepath, 
							  "file_uri" => $fileURI, 	
							  "file_id" => $fstat["ino"],
							  "file_size" => $fstat["size"], 
							  "file_created" => $created, 
							  "file_modified" => $fstat["mtime"], 
							  "file_readonly" => $readonly, 
							  "file_owner" => $fstat["uid"],	
							  "file_owneraccess" => $owneraccess,
							  "file_otheraccess" =>	$otheraccess,
							  "file_isdir" => (($perms & 0x4000) == 0x4000), 
			         		  "file_width" => $imagesize[0], 
			         		  "file_height" => $imagesize[1], 
			         		  "file_encoding" => "", 
							  "thumb_path" => $thumbPath,
							  "thumb_uri"  => $thumbURI,
							  "thumb_width"	=> $thumbsize[0],
						      "thumb_height" => $thumbsize[1],
							  "title" => "", 
			         		  "description" => $description, 
			         		  "author" => $author, 
			         		  "mime_type" => $mimetype, 
			         		  "duration" => "", 
			         		  "frequency" => "", 
			         		  "bitrate" => "" );
						
					//var_dump($content);	 
					if (matchesTranscodingFilter($content, $transcodingFilter)) {		
						array_push($dirContents, $content);
					}	
				}
			}
        }
		else  {
			//failed to open DIR - maybe no access rights
			return NULL;
		}
	}
	else {
		//echo ("Unable to open path or path is NOT a directory: " . $dirpath);
	}
	return $dirContents;
}

function getAllDirContents($fileFilter, $dirRoot)
{
	$albumConfig = $GLOBALS["albumConfig"];
	$allDirContents = array();
	
	if (file_exists($dirRoot) && is_dir($dirRoot)) {
		if ($dh = opendir($dirRoot)) {
        	while ( !(($file = readdir($dh)) === false) ) {
				$dirContents = getDirContents($file, end(explode("/",$file)), $fileFilter);
				if ( sizeof($dirContents) > 0 ) {
					$allDirContents = array_merge($allDirContents, $dirContents);
				}			
			}
		}
	}
	return $allDirContents;
}

function getDirAttributes($dirpath)
{
	if (file_exists($dirpath) && is_dir($dirpath)) {
		if ($dh = opendir($dirpath)) {
			//get parent dir
			$parentDir = dirname($dirpath);
			$fstat = stat($dirpath);

			$attr = array(
				"dir_name" => $dirpath,
				"dir_volume" => $fstat["dev"],
				"dir_id" =>  $fstat["ino"],
				"dir_created" => $fstat["ctime"],
				"dir_modified" => $fstat["modified"],
				"dir_readonly" => !is_writeable($dirpath),
				"dir_owner" => $fstat["uid"]
			);		 
			return $attr;
		}
	}
	return NULL;
}


//
// Convenience functions for different Media types
//


function getImages($album) 
{
	$albumConfig = $GLOBALS["albumConfig"];
	
	if (strcmp($album, "_all") == 0) { //return contents of all albums
		return getAllDirContents($albumConfig['IMAGE_FILTER']);
	}
	else {
		return getDirContents($album, $albumConfig['IMAGE_FILTER']);
	}
}

function getImageThumbnails($album) {
	$albumConfig = $GLOBALS["albumConfig"];
	
	return getThumbnails($album, $albumConfig["IMAGE_FILTER"]);
}

function getMovies($dir, $album) {
	$albumConfig = $GLOBALS["albumConfig"];
	
	if (strcmp($album, "_all") == 0) { 
	    //return contents of all albums
		return getAllDirContents($albumConfig["VIDEO_FILTER"]);
	}
	else {
		return getDirContents($dir, $album, $albumConfig["VIDEO_FILTER"]);
	}
}

function getFlashMovies($dir, $album) 
{
	$albumConfig = $GLOBALS["albumConfig"];
		
	if (strcmp($album, "_all") == 0) { 
	    //return contents of all albums
		return getAllDirContents($albumConfig['FLASH_MOVIES_FILTER']);
	}
	else {
		return getDirContents($dir, $album, $albumConfig['FLASH_MOVIES_FILTER']);
	}
}


function getMusic($dir, $album) {
	$albumConfig = $GLOBALS["albumConfig"];
		
	if (strcmp($album, "_all") == 0) { 
	    //return contents of all albums
		return getAllDirContents($albumConfig["AUDIO_FILTER"]);
	}
	else {
		return getDirContents($dir, $album, $albumConfig["AUDIO_FILTER"]);
	}
}

function getImagesAndMovies($dir, $album) {
	$albumConfig = $GLOBALS["albumConfig"];
	
	if (strcmp($album, $albumConfig["ALLCONTENT"]) == 0) { 
	    //return contents of all albums
		return getAllDirContents(array_merge($albumConfig["IMAGE_FILTER"], $albumConfig["VIDEO_FILTER"]));
	}
	else {
		return getDirContents($dir, $album, array_merge($albumConfig["IMAGE_FILTER"], $albumConfig["VIDEO_FILTER"]));
	}
}

function getFiles($dir, $album, $fileFilter)  {
	return getDirContents($dir, $album, $fileFilter);
}


///Test code

	//$transFilter = makeTranscodingFilter("image", 800, 600, 24, $VIDEOBITRATE, $AUDIOBITRATE, $AUDIOFREQUENCY, 80, 60);
	 
	//$contents = getDirContents( "C:/SVNBranches/orion/Device/trunk/webapp/htdocs/albums/videos/content", "videos", array_merge($albumConfig['IMAGE_FILTER'], $albumConfig['VIDEO_FILTER']));  //
	//var_dump($contents);

?>