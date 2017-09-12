<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

//require_once('component_config.php');
require_once('componentconfig.inc');
require_once('errorcodes.inc');
require_once('statuscodes.inc');
require_once('security.inc');
require_once('globalconfig.inc');
require_once('device.inc');

//based on: http://nadeausoftware.com/articles/2007/09/php_tip_how_strip_html_tags_web_page

function htmlCodePatterns() {
	if (!isset($GLOBALS["_htmlcodepatterns"])) {
		$GLOBALS["_htmlcodepatterns"] =  array(
		// erase scripts, styles, embedded objects et al.
				'@<head[^>]*?>.*?</head>@siu',
				'@<style[^>]*?>.*?</style>@siu',
				'@<script[^>]*?.*?</script>@siu',
				'@<object[^>]*?.*?</object>@siu',
				'@<embed[^>]*?.*?</embed>@siu',
				'@<applet[^>]*?.*?</applet>@siu',
				'@<noframes[^>]*?.*?</noframes>@siu',
				'@<noscript[^>]*?.*?</noscript>@siu',
				'@<noembed[^>]*?.*?</noembed>@siu',
			  // add line breaks for each block
				'@</?((address)|(blockquote)|(center)|(del))@iu',
				'@</?((div)|(h[1-9])|(ins)|(isindex)|(p)|(pre))@iu',
				'@</?((dir)|(dl)|(dt)|(dd)|(li)|(menu)|(ol)|(ul))@iu',
				'@</?((table)|(th)|(td)|(caption))@iu',
				'@</?((form)|(button)|(fieldset)|(legend)|(input))@iu',
				'@</?((label)|(select)|(optgroup)|(option)|(textarea))@iu',
				'@</?((frameset)|(frame)|(iframe))@iu'
		 	);
	}
	return $GLOBALS["_htmlcodepatterns"];
}




function htmlCodeReplace() {
	if (!isset($GLOBALS["_htmlcodereplace"]) ) {
		$GLOBALS["_htmlcodereplace"] = array(
				 ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
				 "\n\$0", "\n\$0", "\n\$0", "\n\$0", "\n\$0", "\n\$0",
				 "\n\$0", "\n\$0",
		);
	}
	return $GLOBALS["_htmlcodereplace"];
}

function sqlCodePatterns() {
	if (!isset($GLOBALS["_sqlcodepatterns"]) ) {
		$GLOBALS["_sqlcodepatterns"] = array(
				  '@select.*$@siu',
				  '@insert.*$@siu',
				  '@delete.*$@siu',
				  '@replace.*$@siu',
				  '@truncate.*$@siu',
				  '@update.*$@siu',
				  '@call.*$@siu',
				  '@handle.*$@siu'
			);
	}
	return $GLOBALS["_sqlcodepatterns"];
}

function sqlCodeReplace() {
	if (!isset($GLOBAL["_sqlcodereplace"]) ) {
		$GLOBAL["_sqlcodereplace"] = array(
				' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		);
	}
	return $GLOBAL["_sqlcodereplace"];
}

function strip_all_html($htmlText )
{
	$text = preg_replace( htmlCodePatterns(),  htmlCodeReplace(), strtolower($htmlText) );
	return strip_tags( $text );
}

//erase all text from beginning sql statemnet to end of string: anyone trying to inject SQL
//is NOT trying to use the web interface properly, so we will not attempt to extract valid
//input from the string.

function strip_sql($sqlText )
{
	$text = preg_replace( sqlCodePatterns(), sqlCodeReplace(), strtolower($sqlText) );

	return $text;
}

function strip_all_code($text)
{
	$strippedText = strip_all_html($text);
	$strippedText = strip_sql($text);
	return strippedText;
}

function containsCode($patternArray, $text) {
	foreach($patternArray as $pattern) {
		if (preg_match($pattern, strtolower($text)) > 0) {
			return true;
		}
	}
	return FALSE;
}

function containsHTML($text) {
	return containsCode(htmlCodePatterns(), $text);
}

function containsSQL($text) {
	return containsCode(sqlCodePatterns(), $text);
}

function containsAnyCode($text) {
	return ( containsCode(htmlCodePatterns(), $text) ||
			 containsCode(sqlCodePatterns(), $text) );
}

function stripLineEnd($text)
{
	$rpos = strpos($text, '\r');
	$npos = strpos($text, '\n');
	if (($rpos && !$npos) || ($rpos < $npos))
	{
		$text = substr($text,0,$rpos);
	}
	else if ( ($npos && !$rpos) || ($rpos > $npos) )
	{
		$text = substr($text,0, $npos);
	}
	return $text;
}

function deleteTrailingCommas($text)
{
		$text=preg_replace('/,\s*([\]}])/m', '$1', $text);
		return $text;
}

function validUrl($str)
{
	return ( ! preg_match('/^(http|https|ftp):\/\/([A-Z0-9][A-Z0-9_-]*(?:\.[A-Z0-9][A-Z0-9_-]*)+):?(\d+)?\/?/i', $str)) ? FALSE : TRUE;
}

function urlReplaceQueryParams($url, $queryParams) {
	foreach ( $queryParams as $paramName=>$paramVal )
	{
		$match = "%" . $paramName . "%";
		$url = str_replace( $match, urlencode($paramVal),  $url );
	}
	return $url;
}

function jsonDecodeUTF8($jsontext)
{
	 return json_decode(deleteTrailingCommas(utf8_encode($jsontext)));
}

function jsonEncodeforJavascript($value)
{
	 return '(' . json_encode($value) . ');';
}

/**
 * Returns HTTP request's base url.
 *
 * @return string| request base url
 */
function getBaseUrl(){
	$scriptUri = $_SERVER['SCRIPT_URI'];
	$scriptUrl = $_SERVER['SCRIPT_URL'];
	$baseUrl = str_replace($scriptUrl, "/",$scriptUri);
	return baseUrl;
}

/**
 * Sets the HTTP status in response
 *
 * @param int $statusCode
 */

function setHttpStatusCode($status_code, $msg=null, $comp_code=null, $error_code=null){
	global $httpStatusCodes;
	$hdrMsg = 'HTTP/1.1 '.$status_code.' ';
	if (!empty($msg)) {
		$hdrMsg = $hdrMsg.$msg;
	} else {
		//if (!empty($comp_code) && !empty($error_code)) {
		//	$hdrMsg = $hdrMsg.$comp_code.'.'.$error_code.' ';
		//}
		if (isset($httpStatusCodes[$status_code])) {
			$hdrMsg = $hdrMsg.$httpStatusCodes[$status_code];
		} else {
			$hdrMsg = $hdrMsg.'UNSET CODE';
		}
	}
	//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'hdrMsg', $hdrMsg);
	header($hdrMsg);
	if (!empty($comp_code) && !empty($error_code)) {
		header('Comp Code: '.$comp_code);
		header('Error Code: '.$error_code);
	}
	return $httpStatusCodes[$status_code];
}

/**
 * Sets the page cache control
 *
 */
function setCacheControl(){
//	header("Cache-Control: no-cache, must-revalidate");
}

/**
 * Sets the mime type
 * 	Ex: application/pdf
 *
 * @param string $mimeType
 */
function setMimeType($mimeType){
	header("Content-type: ".$mimeType);
}

/**
 * Gets the full path to the webapp directory
 */

function getWebAppPath()
{
	$currentPath = __FILE__;
	//strip up to 'webapp'
  	return substr($currentPath, 0, strpos($currentPath,"webapp") + 6);
}

/**
 * Create a url string from an associative array
 * @param associative array
 * @return string
 */
function getUrlString( $args ) {
	$retString = "";
	foreach ( $args as $key => $value ) {
		if (strlen ( $retString ) ) {
			$retString .= "&";
		}
		$retString .= urldecode( $key ). "=" . urlencode( $value );
	}
	return $retString;
}


/**
 * Get the component information from the component config array.
 *
 * @param string $comp_name
 * @param integer $comp_code
 * @return array $comp
 */
function getComponentCodes($comp_name=null, $comp_code=null){
	global $componentConfig;
	//printf("<PRE>getComponentCodes.comp_name=[%s]<PRE>\n",$comp_name);
	$code = 0;
	$comps = array();
	foreach ($componentConfig as $name => $info) {
		$comp = array(
			'code'  => $code,
			'name'  => $name,
			'file'  => $info[0],
			'title' => $info[1],
			'auth'  => $info[2],
		);
		if ($name == $comp_name || ($code == $comp_code && $comp_code !== null && $comp_code !== '')) {
			//printf("<PRE>getComponentCodes.code=[%s]<PRE>\n",$code);
			//printf("<PRE>getComponentCodes.comp=[%s]<PRE>\n",print_r($comp,true));
			return $comp;
		}
		$comps[] = $comp;
		$code++;
	}
	if (!empty($comp_name)) {
		return false;
	} else {
		return $comps;
	}
}

/**
 * Get the error codes information from the error codes array.
 *
 * @param string $error_name
 * @param interger $error_code
 * @return array $error
 * @return array $errors
 */
function getErrorCodes($error_name=null, $error_code=null){
	global $errorCodes;
	//printf("<PRE>getErrorCodes.error_name=[%s]<PRE>\n",$error_name);
	//printf("<PRE>getErrorCodes.error_code=[%s]<PRE>\n",$error_code);
	$code = 0;
	$errors = array();
	foreach ($errorCodes as $name => $desc) {
		$error = array(
			'error_code' => $code,
			'error_name' => $name,
			'error_desc' => $desc,
		);
		if ($name == $error_name || ($code == $error_code && $error_code !== null && $error_code !== '')) {
			//printf("<PRE>getErrorCodes.error=[%s]<PRE>\n",print_r($error,true));
			return $error;
		}
		$errors[] = $error;
		$code++;
	}
	if (!empty($error_name) || !empty($error_code)) {
		return false;
	} else {
		return $errors;
	}
}


/**
 * Get the error codes information from the error codes array.
 *
 * @param interger $errorCode
 * @param string $errorName
 * @param string $language
 * @return array $error
 */
function getErrorCodes_TEST($errorCode=null, $errorName=null, $language='en'){
	global $errorCodes;
	//printf("<PRE>getErrorCodes.errorCode=[%s]<PRE>\n",$errorCode);
	//printf("<PRE>getErrorCodes.errorName=[%s]<PRE>\n",$errorName);
	//printf("<PRE>getErrorCodes.language=[%s]<PRE>\n",$language);
	$code = 0;
	$errors = array();
	foreach ($errorCodes as $name => $languages) {
		$lang = strtolower($language);
		$lang = (isset($languages[$lang]) && !empty($languages[$lang])) ? $lang : 'en';
		$desc = $languages[$lang];
		$error = array(
			'error_code' => $code,
			'error_name' => $name,
			'error_desc' => htmlentities($desc),
			'error_lang' => $lang,
		);
		//printf("<PRE>getErrorCodes.error=%s<PRE>\n",print_r($error,true));
		if (($code == $errorCode && $errorCode !== null && $errorCode !== '') || $name == $errorName) {
			//printf("<PRE>FOUND - getErrorCodes.code = [%s]<PRE>\n",$code);
			//printf("<PRE>FOUND - getErrorCodes.name = [%s]<PRE>\n",$name);
			//printf("<PRE>getErrorCodes.error=%s<PRE>\n",print_r($error,true));
			return $error;
		}
		$errors[] = $error;
		$code++;
	}
	if (!empty($errorCode) || !empty($errorName)) {
		return false;
	} else {
		//printf("<PRE>getErrorCodes.errors=%s<PRE>\n",print_r($errors,true));
		return $errors;
	}
}

/**
 * This function is responsible for enabling, disabling, starting-up or shutting down
 * Orion system services. The scipt path for each service is defined in glolbalconfig.ini
 * If the remoteaccess setting in globalconfig.ini is false, services are disabled, of it
 * is set to true then services willbe enabled and started if they are not already running and
 * if there are one or more device users defined in DeviceUsers table of the database.
 *
 * @return true if successful, otherwise false
 *
 */

function updateRemoteServices() {
	$remoteAccess = getRemoteAccess();	
	
	//get number of remote users
	$deviceUsesDB = new DeviceUsersDB();
	$deviceUserCount = $deviceUsesDB->getNumberOfDeviceUsers();
	//load script paths into array
	$servicesConfig = getGlobalConfig("services");
	if (empty($servicesConfig)) {
		//log error
		return false;
	}
	$scriptList = $servicesConfig["SERVICESCRIPTS"];
	if (empty($scriptList)) {
		//log error
		return false;
	}
	$scriptPathTokens = explode(",",$scriptList);
	if (empty($scriptPathTokens)) {
		//log error
		return false;
	}
	
	$servicesStopped = false;
	
	foreach($scriptPathTokens as $scriptPathToken) {
		//get current status of service
		$scriptPath = $servicesConfig[$scriptPathToken. "_STATUS"];
		//echo($scriptPath . "<br/>");flush();
		unset($output);
		exec($scriptPath, $output, $retVal );
		//echo("OUTPUT: " . $output . "<br/>");flush();
		if (empty($output)) {
			//log error
			return FALSE;
		}
				
		foreach($output as $outputLine) {
			$status = trim(strtolower($outputLine));
			if(strpos($status,"not running") !== false ) {
				$running = false;
				break;
			}
			else if ( strpos($status,"running") !== false ) {
				$running = true;
				break;
			}
		}
		if (!isset($running)) {
			//unknown status from service
			//LOG ERROR
			continue;
		}

		//echo("STATUS" . $status. "<br/>");
		
		if ( strcasecmp($remoteAccess,"FALSE") == 0) {
			if ($running) {
				//if running, disable service
				unset($output);
				$scriptPath = $servicesConfig[$scriptPathToken. "_DISABLE"];
				exec($scriptPath, $output, $retVal);
				$servicesStopped = true;
			}
		}
		else {
			//remote access is enabled
			if (strcasecmp($scriptPathToken,"COMMMANAGERSCRIPT") == 0) {
				//only start comm mgr is there are device users				
				if ( !$running && $deviceUserCount > 0 ) {
					$scriptPath = $servicesConfig[$scriptPathToken. "_ENABLE"];
					unset($output);
					exec($scriptPath, $output, $retVal);
				}
				else if ( $running && ($deviceUserCount == 0) ) {
					//there are no device users, but remote access is enabled so
					//disable comm_mgr if it is running
					unset($output);
					$scriptPath = $servicesConfig[$scriptPathToken. "_DISABLE"];
					exec($scriptPath, $output, $retVal);
					$servicesStopped = true;
				}
			}
			else if (!$running) {
				//start sll other services if remote access is enabled and not running
				$scriptPath = $servicesConfig[$scriptPathToken. "_ENABLE"];
				//echo($scriptPath . "<br/>");flush();
				unset($output);
				exec($scriptPath, $output, $retVal);
				//echo("OUTPUT: " . $output . "<br/>");flush();
			}
		}
	}
	// Services stopped. Clean up old journal file.
	
	if($servicesStopped && file_exists("/internalhd/etc/orion/orion.db-journal")){
		$journalTimeChanged = false;
		$fstat = @stat("/internalhd/etc/orion/orion.db-journal");
		$journalFileMtime = $fstat['mtime'];
		for($i=0;$i<5;$i++){
			sleep(1);
			$fstat = @stat("/internalhd/etc/orion/orion.db-journal");
			if($journalFileMtime != $fstat['mtime']){
				$journalTimeChanged = true;
				break;
			}
		}
		if(!$journalTimeChanged){
			exec("rm -rf /internalhd/etc/orion/orion.db-journal", $output, $retVal);
		}
	}
	
	return true;
}

function encode_utf8($x){
	if(mb_detect_encoding($x) == 'UTF-8'){
		return $x;
	} else {
		return utf8_encode($x);
	}
}

function escape_string($s) {
	//$s = str_replace("'", "\\'", $s);
	$s = str_replace("`", "\`", $s);
	$s = str_replace('"', '\\"', $s);
	$s = str_replace("\r", " ", $s);
	$s = str_replace("\n", " ", $s);
	return $s;
}

?>