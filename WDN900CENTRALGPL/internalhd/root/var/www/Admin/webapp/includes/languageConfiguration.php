<?php

class LanguageConfiguration{

    var $language = '';
    
    function LanguageConfiguration() {
    }

    function getConfig(){

		if(!is_file("/etc/language.conf")){
			return NULL;
		}

        //!!!This where we gather up response  
        //!!!Return NULL on error
		$reader = new StringTableReader("fr","alertmessages.txt");

		//$this->language =
		/* 
		
		*/
		$useShellScriptForLanguage = false;
		$nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        if ($nasConfig === false || !isset($nasConfig['admin']['LANGUAGE_FILE']))
        {
            $useShellScriptForLanguage = true;
        }
		if (SKIP_SHELL_SCRIPT && !$useShellScriptForLanguage)
		{
		    $languageConf = @file_get_contents($nasConfig['admin']['LANGUAGE_FILE']);
		    if ($languageConf === false)
		    {
		        $useShellScriptForLanguage = true;
		    }
		    else 
		    {
        		$languageConfArr = explode(' ', $languageConf);
        		if ($languageConfArr[0] == 'language')
        		{
        		    $output = trim($languageConfArr[1]);
        		}
		    }
		}
		if (!SKIP_SHELL_SCRIPT || $useShellScriptForLanguage)
		{
    		unset($output);
		    exec('sudo sed -e \'/^language /s/language //\' /etc/language.conf', $output, $retVal);
    		if($retVal !== 0) {
    			return NULL;
    		}
    		$output = $output[0];
		}
		
		
		// return en_US by default
		$this->language = "en_US";
        if ($reader->isLocaleSupported($output)) {
			$this->language = $output;
		}
        return(array('language' => $this->language));
    }


    function modifyConfig($changes) {
  
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["language"]) ){
            return 'BAD_REQUEST';
        }
        //Verify changes are valid/Verify that resource was posted to before -RETURN NOT_FOUND
        if(FALSE){
            return 'BAD_REQUEST';
        }
		
		if(!is_file("/etc/language.conf")){
			return 'NOT_FOUND';
		}
	
        //Actually do change
		
		exec("sudo bash -c '(echo \"language {$changes["language"]}\">/etc/language.conf)'", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
        return 'SUCCESS';

    }

    
    function config($lang){
        //Require entire representation
        if( !isset($lang["language"]) ){
            return 'BAD_REQUEST';
        }
        //Verify values are valid 
        if(FALSE){
            return 'BAD_REQUEST';
        }
		
		//if the file is present then a language has been posted already, send error
		if(is_file("/etc/language.conf")){
			return 'BAD_REQUEST';
		}		
		
		exec("sudo bash -c '(echo \"language {$lang["language"]}\">/etc/language.conf)'", $output, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		
        return 'SUCCESS';
    }
  }

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>
