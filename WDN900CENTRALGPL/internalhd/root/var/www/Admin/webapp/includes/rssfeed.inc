<?php

/**
 * Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
 * RSS feed generator class. This class can be used to create RSS 2.0 feeds and Media RSS feeds.
 * 
 * @author Sapsford_J
 *
 */
	
class RssFeed {
	
	var $xml;
	var $rssVersion;
	var $chanPushed = false;
	var $randCheck = array();
	
	/**
	 * Constructor - creates the XML writer and adds the root RSS element 
	 */
	
	function RssFeed()
	{
		require_once("NasXmlWriter.class.php");	
		
		$this->xml = new NasXmlWriter();
		$this->xml->push("rss", array("version" => "2.0", "xmlns:atom" => "http://www.w3.org/2005/Atom"));
	}
	
	/**
	 * 
	 * Fills in the contents of an RSS 2.0 element.
	 * 
	 * @param String $itemTitle item title 
	 * @param String $itemUrl url of item
	 * @param String $itemDescription description of item
	 * @param String $pubDate - RFC-822 format date and time (see getRssDateString() for conversion function)
	 * @param String $itemGuid - unique id, optional. If not supplied, this defaults to a random hex string unique to the feed
	 * @param boolean $isPermalink - true if guid is a url to the item, defaults to false
	 */
	
	private function fillItem(
				$itemTitle, 
				$itemUrl, 
				$itemDescription, 
				$pubDate,
				$itemGuid = NULL, 
				$isPermalink = false)
	{
		$this->xml->element("title", $itemTitle);
		$this->xml->element("link", $itemUrl );
		$this->xml->element("description", $itemDescription );
		$this->xml->element("pubDate", $pubDate);
		if ($itemGuid === NULL)
		{
			while (true) {
				$randid = mt_rand();
				if (!isset($this->randCheck[$randid])) {
					$this->randCheck[$randid] = true;
					break;
				}
			}
			$itemGuid = strpos($itemUrl,'?') === false ?  
									$itemUrl . "?zuid=" . dechex($randid) : 
									$itemUrl . "&zuid=" . dechex($randid) ;
			$isPermalink = true;		
		}
		
		$permaLink = $isPermalink ? "true" : "false";
		$this->xml->element("guid", $itemGuid, array("isPermaLink" => $permaLink) );
	}
	
	/**
	 * 
	 * Starts a new channel. A new channel element will be added to the feed. You should call this
	 * funciotn before adding any items
	 * 
	 * @param string $channelTitle - title for channel
	 * @param string $channelUrl - chanel url (usually the same as the feed url)
	 * @param string $channelDescription - desription of channel
	 * @param string $channelImageUrl - url for channel image
	 * @param $lastUpdatedDate - RFC-822 format date and time (see getRssDateString() for conversion function)
	 * @param string $languageCode - ISO 639-1 language code for channel 
	 */
	
	function startChannel($channelTitle, 
						  $channelUrl, 
						  $channelDescription, 
						  $channelImageUrl,
						  $lastUpdatedDate,
						  $updateFrequencyMinutes,
						  $languageCode = "en-us")
	{
		$this->xml->push("channel");
		$this->xml->element("title", $channelTitle );
		$this->xml->element("link", $channelUrl);
		$this->xml->emptyElement("atom:link", array("href"=>$channelUrl, "rel"=>"self", "type"=>"application/rss+xml"));
		$this->xml->element("description", $channelDescription);
		$this->xml->element("lastBuildDate", $lastUpdatedDate);
		$this->xml->element("language", $languageCode);
		$this->xml->element("ttl", $updateFrequencyMinutes);
		
		$this->chanPushed = true;
	}

	/*
	 * Ends channel. Calling this funcoitn is optinal for a feed with a single channel as the last channel
	 * element will be automatically ended when  call is made to getRSS(). If there is more than one
	 * channel in a feed, then this funciotn must be called before calling startChannel() again.
	 */
	function endChannel() {
		if ($this->chanPushed) {
			$this->chanPushed = false;
			$this->xml->pop();
		}
	}
	
/**
	 * 
	 * Adds an RSS 2.0 item. For an explanation of 
	 * RSS 2.0 elements and attributes, refer to: 
	 * http://validator.w3.org/feed/docs/rss2.html
	 * 
	 * @param String $itemTitle item title 
	 * @param String $itemUrl url of item
	 * @param String $itemDescription description of item
	 * @param String $pubDate - RFC-822 format date and time (see getRssDateString() for conversion function)
	 * @param String $itemGuid - unique id, optional. If not supplied, this defaults to a random hex string unique to the feed
	 * @param boolean $isPermalink - true if guid is a url to the item, defaults to false
	 */
		
	function addItem($itemTitle, 
					 $itemUrl, 
					 $itemDescription, 
					 $pubDate,
					 $itemGuid = NULL, 
					 $isPermalink = false)
	{
		$this->xml->push("item");
		$this->fillItem($itemTitle, $itemUrl, $itemDescription, $pubDate, $itemGuid, $isPermalink);
		$this->xml->pop();
	}
	
	/**
	 * 
	 * Adds a Media RSS item. For an explanation of 
	 * RSS 2.0 elements and attributes, refer to: 
	 * http://validator.w3.org/feed/docs/rss2.html
	 * 
	 * @param String $itemTitle item title 
	 * @param String $itemUrl url of item
	 * @param String $itemDescription description of item
	 * @param String $pubDate - RFC-822 format date and time (see getRssDateString() for conversion function)
	 * @param $mediatitle - title for media item
	 * @param $mediaDescription - description for media item
	 * @param $mediaContentArray - contents of a media:content element, stored in an associative array
	 * @param $mediaThumbnailArray - contents of a media:thumbnail element, stored in an associative array
	 * @param String $itemGuid - unique id, optional. If not supplied, this defaults to a random hex string unique to the feed
	 * @param boolean $isPermalink - true if guid is a url to the item, defaults to false
	 */
	
	function addMediaItem($itemTitle, 
						  $itemUrl, 
						  $itemDescription,
						  $pubDate, 
						  $mediatitle, 
						  $mediaDescription, 
						  $mediaContentArray, 
						  $mediaThumbnailArray,
						  $itemGuid = NULL,
						  $isPermalink = FALSE)
	{
		$this->xml->push("item");
		$this->fillItem($itemTitle, $itemUrl, $itemDescription, $pubDate, $itemGuid, $isPermalink);
			
		$this->xml->element("media:title", "[CDATA[ " . $mediatitle . " ]]");
		$this->xml->element("media:description", "[CDATA[ " . $mediaDescription . " ]]" );
		$this->xml->emptyElement("media:content",  $mediaContentArray);	
		$this->xml->emptyElement("media:thumbnail",  $mediaThumbnailArray);	
		
		$this->xml->pop();
	}
	
	/**
	 * Utility function to create an RFC-822 format date and time string from a unix time 
	 * 
	 * @param long $unixTime - unix time given as # seconds since January 1, 1970
	 * @param boolean $tzOffset - if false, timezone is given as a 3-character string, if false,
	 * timesonze is given as offset number fo hours from GMT. Both timesonze formats are supported 
	 * according to the W3C RSS 2.0 standard, but some RSS clients may only work with one and not the other
	 *
	 * @return RFC-822 format date and time string 
	 */
	
	function getRssDateString($unixTime, $tzOffset = false)
	{
		//rss date string format confirms to RFC-822, 
		//examples: Wed, 23 Feb 2005 16:12:56 PST, or:  Wed, 23 Feb 2005 16:12:56 -0800
		//PHP date Format string: D, d M y H:i:s O/T  ;
		
		if ($tzOffset) {
			return date("D, d M Y H:i:s O", $unixTime);
		}
		else {
			return date("D, d M Y H:i:s T", $unixTime);
		}
		
	}
	
	/**
	 * Create the contents of a media:content element, stored as an associative array
	 * 
	 * @param string $mediaFileUrl - url for the media file (image file,audio file, video file etc).
	 * @param long $mediaFileSize - size of media file in bytes
	 * @param string $mediaMimeType - MIME tyope of media file
	 * @param int $mediaHeight - height of image of video frame in pixels
	 * @param omt $mediaWidth - width of image or video frame in pixels
	 * 
	 * @return associative array with correct field names for media:content element attributes
	 */
	
	function createMediaContent($mediaFileUrl,
								$mediaFileSize,
								$mediaMimeType,
								$mediaHeight = 0,
								$mediaWidth  = 0)
	{
		$contentArray = array("url" => $mediaFileUrl, 
							  "fileSize" => $mediaFileSize, 
							  "type" => $mediaMimeType, 
							  "height" => $mediaHeight,
							  "width" => $mediaWidth);

		return $contentArray;
	}
	
	/**
	 * Create the contents of a media:thumbnail element, stored as an associative array
	 * 
	 * @param string $thumbFileUrl - url for the thumbnail image file
	 * @param int $thumbHeight - height of thumbnail image in pixels
	 * @param omt $thumbWidth - width of thumbnail image in pixels

	 * @return associative array with correct field names for media:thumbnail element attributes
	 */
	
	function createThumbNailContent($thumbFileUrl,
								    $thumbHeight,
								    $thumbWidth)
	{
		$contentArray = array("url" => $thumbFileUrl, 
							  "height" => $thumbHeight,
							  "width" => $thumbWidth);

		return $contentArray;
	}
	
	/**
	 * Get the RSS XML as a string
	 * 
	 * @return RSS 2.0 formatted XML 
	 */
	
	function getRSS()
	{
		$this->endChannel();
		$this->xml->pop();
		$this->xml->pop();
		return $this->xml->getXml();
	}	
}

/*
 * test code
 * 
 */
/*

  $feed = new RssFeed();
  
  $datestr = $feed->getRssDateString(time());
  
  $feed->startChannel("TestChannel",
  					  "http://channel1.wdc.com", 
  					  "Just a test channel", 
  					  "http://images.wdc.com/channel1.png", 
  					  $datestr);

  $feed->addItem("Item One", 
				 "http://channel1.wdc.com/api/1.0/rest?format=xml&imtemid=1001", 
				 "A test item for the RSS feed",
				  $feed->getRssDateString(time()) );

  $feed->addItem("Item Two", 
				 "http://channel1.wdc.com/api/1.0/rest?format=xml&imtemid=1002", 
				 "Another test item for the RSS feed",
				  $feed->getRssDateString(time()) );
				  
				  
  $feed->addItem("Item Three", 
				 "http://channel1.wdc.com/api/1.0/rest?format=xml&imtemid=1003", 
				 "One more test item for the RSS feed",
				  $feed->getRssDateString(time()) );
				  
				  

  $mediaContent1 = 	$feed->createMediaContent("http://channel1.wdc.com/api/1.0/rest?format=xml&media=audio&itemid=4001",
										102034,
										"audio/mp3",
										0,
										0 );

  $thumbNailContent1 = $feed->createThumbNailContent("http://channel1.wdc.com/thumbs/item4001.png",
								    		36,
								    		36);								

  $feed->addMediaItem("Media Item 1", 
					  "http://channel1.wdc.com/api/1.0/rest?format=xml&mediaitemid=4001", 
					  "One more song",
					   $feed->getRssDateString(time()), 
					  "One More Song by A. N. Other", 
				      "Media description goes here", 
					  $mediaContent1, 
					  $thumbNailContent1);
					  
   $mediaContent2 = 	$feed->createMediaContent("http://channel1.wdc.com/api/1.0/rest?format=xml&media=audio&itemid=4002",
										102034,
										"audio/mp3",
										0,
										0 );

  $thumbNailContent2 = $feed->createThumbNailContent("http://channel1.wdc.com/thumbs/item4002.png",
								    		36,
								    		36);								

  $feed->addMediaItem("Media Item 1", 
					  "http://channel1.wdc.com/api/1.0/rest?format=xml&mediaitemid=4002", 
					  "Yest Another Song",
					   $feed->getRssDateString(time()), 
					  "Yet Another Sonmg by A. N. Other", 
				      "Media description goes here", 
					  $mediaContent2, 
					  $thumbNailContent2);					  
						  
  $feed->endChannel();

  $rss = $feed->getRSS();
  var_dump($rss);

*/

?>