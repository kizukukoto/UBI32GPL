<?php
require_once("Mail.php");
require_once("Mail/mime.php");
require_once ("stringtablereader.inc");
require_once ("messageFormatter.inc");
require_once("languageConfiguration.php");

class MailClient{
	var $smtp_config_path = '/etc/smtp.conf';
	var $smtp_config = null;
	var $logObj;

	function MailClient(){
		if (file_exists($this->smtp_config_path)) {
			$this->smtp_config = parse_ini_file($this->smtp_config_path, true);
		}	

		$this->logObj = new LogMessages();
	}
	
	/**
	 * $alertConfig will have to and return addresses.
	 * $eventIds will have list os events.
	 */ 
	function sendEmail($alertConfig, $alerts){
		
		// Check email enabled.
		if(strcmp($alertConfig['email_enabled'],"on") != 0){
			return; // Email alerts is not enabled.
		}
		// Get device name and firmware version.
		$deviceName = exec("sudo hostname");
		$fwVersion = exec("cat /etc/version");

        $langConfigObj = new LanguageConfiguration();
        $result = $langConfigObj->getConfig();
		$langCode = "en_US";
		
        if($result !== NULL && isset($result['language'])){
        	$langCode = $result['language'];
        } 
				
		// Create localized string readers for email text and alert messages. 
		$labelReader = new StringTableReader($langCode, "alertemail.txt");
		$msgReader = new StringTableReader($langCode, "alertmessages.txt");
		
		$subject = $labelReader->getString("0001", array(0=>$deviceName));
		
		// Get localized subject
		$subject = "=?"."utf-8"."?B?".base64_encode($subject)."?=";
		
		//Build mail body.
		$body = $labelReader->getString("0002", array(0=>$deviceName)).".\n\n";
		
		foreach($alerts as $key=>$alert){
			$alert_str = $labelReader->getString("0007").":".$msgReader->getString("$alert->code")."\n\n";

			/** get the formatted message based on alert code & substitution values */
			$description = formatMessage( $msgReader->getString($alert->code."D"), $alert->subst_values ) ;
			$alert_str = $alert_str.$labelReader->getString("0003").":".$description."\n\n";

			$alert_str = $alert_str.$labelReader->getString("0004").":".$alert->severity."\n\n";
			$alert_str = $alert_str.$labelReader->getString("0005").":".$alert->code."\n\n";
			$alert_str = $alert_str.$labelReader->getString("0006").":".date('m-d-Y h:i:s A',$alert->timestamp)."\n\n\n";
			
			$body = $body.$alert_str."\n";

			//get firmware version and add to the body
			$fwVersion_string = $labelReader->getString("0008").":"."\t".$fwVersion; 
			$body = $body.$fwVersion_string."\n";

			// $this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  $alert_str);
		}
		
		//from
		$from = $alertConfig['email_returnpath'];
		
		// to
		if(isset($alertConfig['email_recipient_0']) && 
					strcmp($alertConfig['email_recipient_0'],"") != 0){
			$to = $alertConfig['email_recipient_0'];	
		}
		if(isset($alertConfig['email_recipient_1']) && 
					strcmp($alertConfig['email_recipient_1'],"") != 0){
			$to = $to.", ".$alertConfig['email_recipient_1'];	
		}
		if(isset($alertConfig['email_recipient_2']) && 
					strcmp($alertConfig['email_recipient_2'],"") != 0){
			$to = $to.", ".$alertConfig['email_recipient_2'];	
		}
		if(isset($alertConfig['email_recipient_3']) && 
					strcmp($alertConfig['email_recipient_3'],"") != 0){
			$to = $to.", ".$alertConfig['email_recipient_3'];	
		}
		if(isset($alertConfig['email_recipient_4']) && 
					strcmp($alertConfig['email_recipient_4'],"") != 0){
			$to = $to.", ".$alertConfig['email_recipient_4'];	
		}

		$headers = array ('From' => $from,
							'To' => $to,
							'Subject' => $subject);
		
		$smtp = Mail::factory('smtp',
							array ('host' => $this->smtp_config['host'],
									'port' => $this->smtp_config['port'],
									'auth' => true,
									'username' => $this->smtp_config['username'],
									'password' => $this->smtp_config['password']));
							
		$mime = new Mail_mime();
		$mime->setTxtBody($body);
		$body = $mime->get(array('text_charset' => 'utf-8'));
		$headers = $mime->headers($headers);
							
		$mail = $smtp->send($to, $headers, $body);
		if (PEAR::isError($mail)) {
			$mail = $smtp->send($to, $headers, $body);
			if (PEAR::isError($mail)) {
				return false;
			} else {
				return true;
			} 
		} else {
			return true;
		}	
	}

	
	function sendTestEmail($from, $to){
		// TODO : Replace subject and boday with localized strings.
		$subject = "Test Mail";
		$body = "This is a test email.";

		$headers = array ('From' => $from,
							'To' => $to,
							'Subject' => $subject);
		
		$smtp = Mail::factory('smtp',
							array ('host' => $this->smtp_config['host'],
									'port' => $this->smtp_config['port'],
									'auth' => true,
									'username' => $this->smtp_config['username'],
									'password' => $this->smtp_config['password']));
		$mail = $smtp->send($to, $headers, $body);
		if (PEAR::isError($mail)) {
			$mail = $smtp->send($to, $headers, $body);
			if (PEAR::isError($mail)) {
				return false;
			} else {
				return true;
			} 
		} else {
			return true;
		}	
		
	}
}
?>
