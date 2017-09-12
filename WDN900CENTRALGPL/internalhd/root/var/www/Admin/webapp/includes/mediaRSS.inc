<?php
	// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

	require_once("util.inc");
	require_once("albumsdb.inc");
	require_once("albumitemsdb.inc");
	require_once("rssfeed.inc");
	require_once("globalconfig.inc");

	
	if (!isset($GLOBALS["albumConfig"])) {
		$albumConfig = getGlobalConfig("albums");
		if ($albumConfig) {
			$MEDIAFILTERS = 
			array('image' => $albumConfig['IMAGE_FILTER'], 
				  'video' => $albumConfig['VIDEO_FILTER'], 
				  'flashvideo' => $albumConfig['FLASH_MOVIES_FILTER'], 
				  'audio' => $albumConfig['AUDIO_FILTER']);

			$albumConfig['MEDIAFILTERS'] = $MEDIAFILTERS;
			$GLOBALS["albumConfig"] = $albumConfig;
		}  
	}
	
	function getMimeType($fileName) {
		$mimeType = "application/force-download";
		$extplusdot = strrchr($fileName,'.');
		if ($extplusdot) {
			$ext = substr($extplusdot,1);
			$albumConfig = $GLOBALS["albumConfig"];
			if ($albumConfig && $ext) {
				if ( in_array($ext, $albumConfig['VIDEO_FILTER'])) {
						$mimeType = $albumConfig ['VIDEO_MIMETYPES'][$ext];
				}
				else if (in_array($ext, $albumConfig['AUDIO_FILTER'])) {
						$mimeType = $albumConfig['AUDIO_MIMETYPES'][$ext];
				}
				else if (in_array($ext, $albumConfig['IMAGE_FILTER'])){
						$mimeType = "image/" . $ext;
				}
			}
		}
		return $mimeType;
	}
	
	function genAlbumItemsMediaRSS($userId, $albumId, $media)
	{
		$albumConfig = $GLOBALS["albumConfig"];
		$albumsDB = new AlbumsDB();
		$albumItemsDB = new AlbumItemsDB();
		
		$mediaFilter = array();
		if (strcmp($media, $albumConfig["ALLCONTENT"] ) != 0 )
		{
			$mediaTypes = explode("+", $media);
			
			foreach ($mediaTypes as $mediatype)
			{
				$mediatype = trim($mediatype);
				$filterArray = $albumConfig["MEDIAFILTERS"][$mediatype]; 
				if (isset($filterArray))
				{
					$mediaFilter = array_merge($mediaFilter,  $filterArray);
				}
			}
		}
		
		$albumPath = $albumConfig['ALBUMSROOT'] . $album . "/" . $albumConfig['CONTENTFOLDER'];
		
		$album = $albumsDB->getAlbum($userId, $albumId);
		$albumItems = $albumItemsDB->getAlbumItems($albumId);
				
		if ( $album !== NULL && $albumItems !== NULL )
		{
			$rss = new RssFeed();
			
			$rss->startChannel($album[0]['name'], 
						  	   "/api/1.0/rest/albumdemo?format=rss2&albumid=" . $albumId . "&media=" . $media,
						  		$album[0]['description'], 
						  		"",
						  		$rss->getRssDateString(time()),
						  		60,
						  		"en-us");
						  		
			foreach ($albumItems as $albumItem)
			{
				$idx++;
				$filePath = $albumItem['name'];
				
				$itemTitle = $albumItem['name'];
				$itemUri = $albumConfig['URLPREFIX'] . 'api/1.0/rest/albumitemcontents/' .  $albumItem['album_item_id'];
				
				if(isset($albumItem['cover_art_path']) && $albumItem['cover_art_path'] != ''){
					$itemThumbUri = $albumConfig['URLPREFIX'].$albumItem['cover_art_path'];
					$thumbImageSize = array(640, 480);	
				}else{
					$itemThumbUri =  $itemUri."?tnType=tn96s1";
					$thumbImageSize = array(96, 96);	
				}
				$imageSize = array($albumItem['width'],$albumItem['height']);
				$mimeType = getMimeType($filePath);
				$mediaContentArray = 
						$rss->createMediaContent($itemUri,
											     $albumItem['size'],
												 $mimeType,
												 $imageSize[1],
												 $imaheSize[0]);
				
				$mediaThumbnailArray = 
						$rss->createThumbNailContent($itemThumbUri,
							    					 $thumbImageSize[1],
							    					 $thumbImageSize[0]);
				
				$rss->addMediaItem($itemTitle, 
					  			   $itemUri, 
					  			   "",
					  			   $rss->getRssDateString(time()), 
					  			   $itemTitle, 
					  			   "", 
					  			   $mediaContentArray, 
					  			   $mediaThumbnailArray);
			}
			return $rss->getRSS();
			
		}
		
		return NULL;
	}
	
	function genAlbumsMediaRSS($userId)
	{
		$albumConfig = $GLOBALS["albumConfig"];
		$albumIdLookup = $GLOBALS['_mrssalbumidlookup'];
		$albumsDB = new AlbumsDB();
		
		$albums = $albumsDB->getAlbums($userId);
		
		if (isset($albums) && sizeof($albums) > 0)
		{
			$rss = new RssFeed();
			 
			$rss->startChannel("Albums", 
						  "/api/1.0/rest/albumdemo?format=rss2&albumid=_all", 
						  "rss feed of albums for current user", 
						  "",
						  $rss->getRssDateString(time()),
						  60,
						  "en-us");
			
			foreach ($albums as $album)
			{
				$albumTitle = $album['name'];
				$retPos = strpos($albumTitle, "/r");
				if ($retPos)
				{
					$albumTitle = substr($albumTitle, 0, $retPos);
				}
				$albumUri = "/api/1.0/rest/albumdemo?format=rss2&albumid=". $album['album_id'] . "&media=image+audio+video";
				$itemUri = $album['preview_image'];
				$mimeType = getMimeType($itemUri);
				
				$mediaContentArray = 
						$rss->createMediaContent($itemUri,
											     0,
												 $mimeType,
												 480,
												 640);
				
				$mediaThumbnailArray = 
						$rss->createThumbNailContent($itemUri,
							    					 480,
							    					 640);
				
				$rss->addMediaItem($albumTitle, 
					  			   $albumUri, 
					  			   "",
					  			   $rss->getRssDateString(time()), 
					  			   $albumTitle, 
					  			   "", 
					  			   $mediaContentArray, 
					  			   $mediaThumbnailArray);		 
			}
			
			return $rss->getRss();
		}
		return NULL;
	}
	
	//Test code
	
	//$xmlstring = genXML("Monument Valley", "image");
	//var_dump( $xmlstring );
	
	//$xmlchanstring = genAlbumsMediaRSS(24);
	//var_dump( $xmlchanstring );

	//$xmlchanstring = genAlbumItemsMediaRSS(24,5,"image+audio+video");
	//var_dump( $xmlchanstring );
	
	
?>