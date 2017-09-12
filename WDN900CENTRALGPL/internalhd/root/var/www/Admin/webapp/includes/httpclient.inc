<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
class HttpClient {
	
	var $name = "HttpClient";

	private $server;
	private $requestUrl;
		private $curlOptions = array(CURLOPT_SSL_VERIFYPEER => false);
	
	public function __construct ( $args = array() ) {
		$this->curlOptions[ CURLOPT_RETURNTRANSFER ] = TRUE;
				
		if (!empty ( $args['requestUrl'] ) ) {
			$this->requestUrl = $args['requestUrl'];
		}else {
			$this->requestUrl = "";
		}
		
		if (!empty ($args['curlOptions'])) {
			$this->curlOptions = $args['curlOptions'];
		}
	}
	
	public function post($args = array()) {
		$output = null;
		if ( !empty ( $args['requestUrl'] ) ) {
			$this->requestUrl = $args['requestUrl'];
		}
		$curlClient = $this->__getCurl();
		curl_setopt ( $curlClient, CURLOPT_POST, TRUE );
		if ( !empty ( $args['data'] ) ) {
			curl_setopt( $curlClient, CURLOPT_POSTFIELDS, getUrlString( $args['data'] ) );
		}
		
		$output = curl_exec( $curlClient );
		$info = curl_getinfo($curlClient);
		$statusCode = $info['http_code'];
		curl_close($curlClient);
		return array("status_code" => $statusCode, "response_text" => $output);
	}

	public function get($requestUrl) {
		$output = null;
		// URL can't be enpty for get
		if (empty($requestUrl)) {
			return;	
		}
		$this->requestUrl = $requestUrl;
		$curlClient = $this->__getCurl();
		
		$output = curl_exec($curlClient);
		
		$info = curl_getinfo($curlClient);
		$statusCode = $info['http_code'];
		
		curl_close($curlClient);
		
		return array("status_code" => $statusCode, "response_text" => $output);
	}
	
	private function getLocale() {
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
	
	
	private function generateLocaleHeader() {
		$locale = $this->getLocale();
		$locale = str_replace("_", "-", $locale);
		return "Accept-Language: $locale";	
	}

	private function __getCurl () {
		$curl = curl_init();
		$curlOptions = $this->curlOptions;
		$curlOptions[CURLOPT_URL] = $this->requestUrl;
        $headers = isset($curlOptions[CURLOPT_HTTPHEADER]) ? $curlOptions[CURLOPT_HTTPHEADER] : NULL ;
        
        if (is_array($headers) && !empty($headers)) {
        	array_push($headers, $this->generateLocaleHeader());
        } else {
        	$headers = array($this->generateLocaleHeader());
        }

        $curlOptions[CURLOPT_HTTPHEADER] = $headers;

        curl_setopt_array( $curl, $curlOptions );
        return $curl;
	}
}
?>