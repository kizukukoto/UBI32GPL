<?php
define('WEBDAV_PREFIX', 'shares'); 
define('ADMIN_API_ROOT', $_SERVER["__ADMIN_API_ROOT"] . '/');

ini_set('include_path',
    '.' . ':' . ADMIN_API_ROOT . '/webapp/includes/' . ':'
    . ADMIN_API_ROOT . '/webapp/classes/api/' . ':'
    . ini_get('include_path')
    ); 

require_once("shareaccess.php");
require_once("deviceusersdb.inc");
require_once("usersdb.inc");
require_once("security.inc");
require_once("../includes/globalconfig.inc");

$locale = getLocale();

setlocale(LC_ALL, $locale.'.utf8');
bindtextdomain("messages", ADMIN_API_ROOT . "/webapp/locale/");
//bindtextdomain("default", "UTF-8");
textdomain("messages");


// Get host name/IP and port number

$globalConfig = getGlobalConfig("global");


$portalProtocol = getCentralServerProtocol();
$portalHostName= getCentralServerHost();
//XXX: remove me on production
//$portalHostName="stage9web.remotewd4.com";


$portalUrl = $portalProtocol . $portalHostName;


session_start();

function getDeviceUserDisplayName() {
	$displayName = $_SESSION['username'];
	
	if (!empty($displayName)) {
		if (!empty($_SESSION['fullname'])) {
			$displayName = $_SESSION['fullname'];
		}

		$displayName = " - " . $displayName;
	} else {
		$displayName = " ";
	}
	
	return $displayName;
}

function getLocale() {
	static $locale = null;
	
	if ($locale === null) {
			
		if (file_exists('/etc/language.conf')) {
			$content = file_get_contents('/etc/language.conf');
			$strarr = explode(' ', $content);
			$locale = trim($strarr[1]);
		} else {
			$out = exec('xmldbc -w /device/features/language');
			switch($out) {
				case 'de':
					$locale = 'de_DE';
					break;
				case 'en':
					$locale = 'en_US';
					break;
				case 'es':
					$locale = 'es_ES';
					break;
				case 'fr':
					$locale = 'fr_FR';
					break;
				case 'it':
					$locale = 'it_IT';
					break;
				case 'ja':
					$locale = 'ja_JP';
					break;
				case 'ko':
					$locale = 'ko_KR';
					break;
				case 'ptbr':
					$locale = 'pt_BR';
					break;
				case 'ru':
					$locale = 'ru_RU';
					break;
				case 'zhcn':
					$locale = 'zh_CN';
					break;
				case 'zhtw':
					$locale = 'zh_TW';
					break;
					
				default:
					$locale = 'en_US';
			}
			
		}
	}
	
	return $locale;	
}

function gettext2 ($key) {
	static $stringTable = null;
	$locale = getLocale();
	if ($stringTable === null) {
		//build string table
		$stringTable = array();
		
		$localePath = ADMIN_API_ROOT . "/webapp/locale";
		$defaultLocaleFile = "$localePath/en_US.utf8/LC_MESSAGES/messages.po";
		$localeFile = "$localePath/$locale.utf8/LC_MESSAGES/messages.po";
		$fileContent = file_get_contents($defaultLocaleFile);
		$fileContent .= file_get_contents($localeFile);
		if (is_string($fileContent)) {
			$lines = explode("\r\n", $fileContent);
			$curkey = null;
			foreach ($lines as $line) {
				
				if (preg_match_all("/(msgid|msgstr)\s+\"(.*?)\"/", $line, $matches, PREG_SET_ORDER)) {
					//var_dump($matches[0][1]);
					//var_dump($matches[1]);
					$prop = $matches[0][1];
					
					$trimmed = $matches[0][2];

					if (strcmp($prop, "msgid") == 0) {

						$curkey = $trimmed;
					} else {
						$stringTable[$curkey] = $trimmed;
						$curkey = null;
					}
					
				}
			}
		}
	}
	
	//var_dump($stringTable);
	return $stringTable[$key];
}


?>
