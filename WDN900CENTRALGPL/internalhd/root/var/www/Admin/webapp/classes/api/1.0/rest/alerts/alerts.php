<?php
require_once('logMessages.php');
require_once("alertRSS.inc");

/**
 * 
 * REST API class for alerts RSS feed
 * 
 * @author Sapsford_J
 *
 */
class Alerts
{
	static $logObj;
	
    //format of params : ?album=<albumname, _all>&media=<mediatype, _none>,
    //mediattype = 'images','video','audio','files', or a combination, e.g.: 'images+video'

	var $alertLimit = 20;
	
	/**
	 * Constructor - just includes necessary .inc files
	 */
	
	public function __construct()
	{
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
	}
	
    /**
     * Creates an returns the alert RSS feed
     */
    
    function createAlertRSSResult() {
		return generateAlertRss($this->alertLimit);
    }
    
    /**
     * Process REST GET
     * 
     * @param $urlPath - path used to invlike this REST API call
     * @param $queryParams - optional query params from HTTP query string
     * @param $output_format - required output format, this can only be rss, rss2 or xml
     */

    function get($urlPath, $queryParams=null, $output_format='xml')
    {
    	
	    self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (queryParams=$queryParams)");
    	    
        switch($output_format)
        {
            case 'xml'	:
            case 'rss2'	:
            case 'rss'	:
                $result =  $this->getXmlOutput($urlPath, $queryParams);
                break;
            case 'json' :
                $result =  $this->getJsonOutput($urlPath, $queryParams);
                break;
            case 'text' :
                $result =  $this->getTextOutput($urlPath, $queryParams);
                break;
        };

        if ( ($result  === NULL) || (sizeof($result) == 0)) {
            header("HTTP/1.0 404 Not Found");
        }
        echo($result);

        //return $result;
    }

    /**
     * Process HTTP POST
     * @param $urlPath
     * @param $queryParams
     * @param $output_format
     */
    
    function post($urlPath, $queryParams=null, $output_format='xml') {
    	header("HTTP/1.0 405 Method not allowed"); 
        echo("POST is not supported");
    }

    /**
     * Process HTTP PUT
     * @param $urlPath
     * @param $queryParams
     * @param $output_format
     */

    function put($urlPath, $queryParams=null, $output_format='xml') {
    	header("HTTP/1.0 405 Method not allowed"); 
    	echo("PUT is not supported");
    }

   /**
     * Process HTTP DELETE
     * @param $urlPath
     * @param $queryParams
     * @param $output_format
     */
  
    function delete($urlPath, $queryParams=null, $output_format='xml') {
    	header("HTTP/1.0 405 Method not allowed"); 
        echo("DELETE is not supported");
    }

   /**
     * Returns RSS formatted alerts
     * @param $urlPath
     * @param $queryParams
     * @param $output_format
     */
    
    function getXmlOutput($urlPath, $queryParams) 
    {
        return $this->createAlertRSSResult();
    }

    /**
     * Return JSON formatted output - not implemented as RSS is an xml-only
     * format.
     * 
     * @param $urlPath
     * @param $queryParams
     */
    
    function getJsonOutput($urlPath, $queryParams) {
    	header("HTTP/1.0 501 Not Implemented");
    	echo("JSON is not supported");
    }

    /**
     * Return plain text output - not implemented as RSS is an xml-only
     * format.
     * 
     * @param $urlPath
     * @param $queryParams
     */
    
    function getTextOutput($urlPath, $queryParams) {
    	header("HTTP/1.0 501 Not Implemented");
        echo("Text output is not supported");
    }


} //end class


?>