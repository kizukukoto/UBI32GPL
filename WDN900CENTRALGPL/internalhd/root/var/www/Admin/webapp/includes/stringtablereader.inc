<?php

	/**
	 * 
	 * Class for managing Localized string tables. The string table files are stored 
	 * under /etc/nas/strings in linux and C:\SVNBranches\Nas3G\REST\webapp\htdocs\strings
	 * on Windows. 
	 * 
	 * The localized strong table files are stored in directories named using the ISO 639-1
	 * code for the locale/language.
	 * 
	 * Example:
	 * 
	 * 		../strings/
	 * 				en_us/
	 *              fr/
	 *              es/
	 *              de/
	 * 
	 * The format of each line of the file is: nnnn = mmmmmm... where nnnn
	 * is the four digit code and mmmmmmmmm is the text message in the given locale/language
	 * 
	 * @author Sapsford_J
	 *
	 */

	class StringTableReader {
		
		private static $stDirPath;

		private $stFileName;
		private $locale;
		private $locales; //supported locales
		private $stringTable = array();
		
		/**
		 * fn to read locale directories and add directory names to locales array
		 */
		
		private function readLocales() 
		{
		  $this->locales = array();	
	      if ($dh = opendir(self::$stDirPath)) {
   				while ( ($file = readdir($dh)) !== false ) {
				      if ( $file != "." && $file != "..") {
         					if (is_dir(self::$stDirPath . '/' . $file) ) {
								$this->locales[$file] = true;
					         }
      				  }
				}
			}
		}
		
		/**
		 * fn to load the string table for the current locale and populate
		 * the $stringTable array 
		 * 
		 */
		private function readStringTable()
		{
			$filePath = self::$stDirPath . "/" . $this->locale . "/" .  $this->stFileName;
			if (!file_exists($filePath )) {
				//log error
				return false;			
			}
			
			$file = fopen($filePath, "r");
			if ($file === false) {
				//log error
				return false;
			}
	
			//read lines from file
			$this->stringTable = array();
			while ($fileLine = fgets($file)) {
				$messageFields = explode("=", $fileLine ); 
				if (sizeof($messageFields >= 2)) {
					$messageFields[0] = isset($messageFields[0]) ? $messageFields[0] : NULL;
					$messageFields[1] = isset($messageFields[1]) ? $messageFields[1] : NULL;
					$this->stringTable[ (trim($messageFields[0])) ] = trim($messageFields[1]); 
				}
				else {
					//log error - malformed string table
				}	
			}
		}
		
		/**
		 * Constructor - sets the initial local and reads in the string table for that
		 * locale
		 * 
		 * @param string $locale ISO 639-1 locale/language code
		 * @param string $fileName name of string table file in each locale dir, do not include the path
		 */
		
		public function StringTableReader($locale, $fileName)
		{
		   if ( (isset($_SERVER["WINDIR"]) && (stripos($_SERVER["WINDIR"], "windows") !== false)) ||
					   (isset($_SERVER["windir"]) && (stripos($_SERVER["windir"], "windows") !== false)) ) {
				  //we are running on windows
				  self::$stDirPath = "C:\\SVNBranches\\Nas3G\\etc\\nas\\strings";
			}
			else {
			  	  self::$stDirPath = "/etc/nas/strings";
			}
 			$this->setMessageLocale($locale);
 			$this->stFileName = $fileName;
		}
		
		/**
		 * fn to get an array of ISO locale codes for supported locales. Only the locales
		 * in this list have associated strong tables.
		 * 
		 * @return array of locale codes
		 */
		
		public function getSupportedLocales()
		{
			if (!isset($this->locales) )  {
				$this->readLocales();
			}
			return $this->locales;
		}
		
		/**
		 * fn to test if a given locale is supported
		 * @param unknown_type $locale
		 */
		
		public function isLocaleSupported($locale)
		{
			$locales = $this->getSupportedLocales();
			return isset($locales[$locale]);
		}
		
		/**
		 * fn to get the localized string matching the given token.
		 * If there is no string mathcing the token, or an un0supporte locale
		 * has been set, then "????????" will be returned.
		 *  
		 * @param unknown_type $token
		 * @param unknown_type $args
		 */
		
		public function getString($token, $args=null ) {
			$msgStr = "????????";
			if ( sizeof($this->stringTable) == 0 ) {
				//read the string table
				$this->readStringTable();
			}
			
			if (isset($this->stringTable[$token])) {
				$msgStr = $this->stringTable[$token];
			}
			if ($msgStr === null)
			{
				//log error
			}
			else if ($msgStr !== null && $args !== null) {
				$argNum = 1;
				foreach ($args as $arg) {
					$msgStr = str_replace("%" . $argNum, $arg, $msgStr);
					++$argNum;
				}
			}
			return $msgStr;
		}
		
		/**
		 * fn to set the current String table locale
		 * 
		 * @param string $localeToLoad ISO 639-1 locale/language code
		 */
		
		public function setMessageLocale($localeToLoad) {
			if ($this->locale != $localeToLoad) {
				if ($this->isLocaleSupported($localeToLoad)) {
					$this->locale = $localeToLoad;
				}
				else {
					//log error message here - default to en-us
					$this->locale = "en_US";
				}
				$this->stringTable = array();
			}
		}
				
		
	}

	/**
	 * Test code
	 */
	
	/*
	$reader = new StringTableReader("fr","alertmessages.txt");
	$locales = $reader->getSupportedLocales();
	var_dump($locales);
	$message = $reader->getString(1001);
	var_dump($message);
	$message = $reader->getString(2);
	var_dump($message);
	$message = $reader->getString(4, array("C"));
	var_dump($message);
	$issupported = $reader->isLocaleSupported("DE");
	var_dump($issupported);
	$issupported = $reader->isLocaleSupported("BG");
	var_dump($issupported);
	$reader->setMessageLocale("DE");
	$message = $reader->getString(1001);
	var_dump($message);
	$message = $reader->getString(2);
	var_dump($message);
	$message = $reader->getString(4, array("C"));
	var_dump($message);
	$reader->setMessageLocale("BG");
	$message = $reader->getString(1001);
	var_dump($message);
	$message = $reader->getString(2);
	var_dump($message);
	*/
	
	
	

?>