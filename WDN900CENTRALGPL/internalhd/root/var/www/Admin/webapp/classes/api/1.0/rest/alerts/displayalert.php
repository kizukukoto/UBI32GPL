<?php

	//NOTE: this is just a Test place-holder so that the Alert RSS links work
	
	class DisplayAlert
{

	
	/**
	 * Constructor - just includes necessary .inc files
	 */
	
	function DisplayAlert() {
		require_once("alert.inc");
		require_once ("stringtablereader.inc");
		require_once ("languageConfiguration.php");
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
        switch($output_format)
        {
            case 'xhtml':
        	case 'xml'	:
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
    	//get alert code and read alert message string from localized string table
		$wd_code = $queryParams["code"];	
		$wd_timestamp = $queryParams["timestamp"];	
		
		if (isset($wd_code)) { 
		
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
			
			$message = $reader->getString($wd_code);
			if (isset($wd_timestamp)) {
				$displaydate = date("D, d M Y H:i:s T", $wd_timestamp);
				
			}
		}

?>
<html>
<head>
	<title>WD NAS System Alert</title>
</head>
<body>
	<p></p>
	<center>
	<table border="0" cellspacing= "10" cellpadding = "8" width="85%">
	<tr>
		<td style="border-top:1px;border-bottom:solid 2px;border-top:solid 2px"><font face="arial" size="5" color="blue">WD NAS</font></td>
	</tr>
	<tr>
		<td style="border-top:1px;border-bottom:solid 1px"><font face="arial" size="5" color="red"> Alert: </font><font face="arial" size="5" color="black"><?php echo($message);?></font></td>
	</tr>
	<tr>
		<td><font face="arial" size="2" color="black"><?php echo($displaydate) ?></font></td>
	</tr>
	<tr>
		<td><font face="arial" size="4" color="black">Description</font></td>
	</tr>
	<tr>
		<td><font face="arial" size="3" color="black">Alert description goes here</font></td>
	</tr>
	</table>
	</center>
</body>
</html>
    	
<?php   	
    	
    	
        return "";
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

/*
//test code
$qp = array();
$qp["format"]="json";
$qp["code"]=1001;
$urlp=array();

$displayAlert = new DisplayAlert();

$result = $displayAlert->get($urlp, $qp, "json");

//var_dump($result);

*/

?>