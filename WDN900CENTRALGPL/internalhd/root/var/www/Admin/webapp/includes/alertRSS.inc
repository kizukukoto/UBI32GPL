<?php

require_once("rssfeed.inc"); 
require_once("alert.inc");
require_once ("alertlogreader.inc");
require_once ("stringtablereader.inc");
require_once ("languageConfiguration.php");
require_once ("globalconfig.inc");
require_once ("messageFormatter.inc");

/**
 * 
 * Function to generate an rss feed for the last n alerts. 
 * 
 * @param int $alertLimit - number of alerts to include in feed
 * 
 * @author Sapsford_J
 */

function generateAlertRss($alertLimit)
{
	//get last n alerts from log
	$logReader = new AlertLogReader();
	$alertArray = $logReader->getLatestAlerts($alertLimit);
	
	if (isset($alertArray)) {
		
		$feed = new RssFeed();
		$urlPrefix = "http://" . $_SERVER["HTTP_HOST"];
		
/* This option is not used - not exposed in the WebUI
 * Hardcoding it for now siince 'getConfig()' 
 * no longer exists in globalconfig.inc
 * *
		//get update freq from conf file
		$updateFrequencyMins = getConfigSetting("rssalerts.conf","main","UPDATEFREQUENCYMINS");
		if ($updateFrequencyMins === null ) {
			$updateFrequencyMins = 1;
		}
*/
        $updateFrequencyMins = 1;

		$feed->startChannel("alerts",
  					  $urlPrefix . "/api/1.0/rest/alerts?format=rss", 
  					  "System Alerts", 
  					  $urlPrefix . "/images/alertchannel.png", 
  					  $feed->getRssDateString(time()),
  					  $updateFrequencyMins);
  					  
  		//get the current locale code 
  		$locale = "en_US"; //default to American english
  		$langConfigObj = new LanguageConfiguration();
        $result = $langConfigObj->getConfig();

        if($result !== NULL){
            if($result['language'] !== '' ) {
            	$locale = $result['language'];
            }
        }         

  		$reader = new StringTableReader($locale, "alertmessages.txt");
		foreach ($alertArray as $alert) {
            // read localized message text from string table for current locale
            $message = $reader->getString($alert->code);
            $description = $reader->getString($alert->code . "D");

            /** format the description */
            $description = formatMessage( $description, $alert->subst_values );

            $feed->addItem( $message, 
                            $urlPrefix . "/api/1.0/rest/displayalert?code=" . $alert->code . "&timestamp=" . $alert->timestamp, 
                            $description,
                            $feed->getRssDateString($alert->timestamp) );
		}
	}
	
	$feed->endChannel();
	return $feed->getRSS();
	
}

/*
 * Test code
 */

//$rss = generateAlertRss(20);
//var_dump($rss);


?>