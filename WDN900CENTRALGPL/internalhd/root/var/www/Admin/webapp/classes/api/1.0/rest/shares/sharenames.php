<?php

require_once('logMessages.php');
require_once('outputwriter.inc');
require_once('restcomponent.inc');
require_once('security.inc');
require_once('usershares.php');

class ShareNames extends RestComponent {

	static $userShares;
	var $logObj;
	
	/**
	 * Constructor, creates UserShares and ShareAccess instances
	 */
	function __construct() {
		$this->logObj = new LogMessages();
		if (!isset(self::$userShares)) {
			self::$userShares = new UserShares();
		}
	}
	
	public function get($urlPath, $queryParams=null, $output_format='xml') {
		// Don't allow in WAN
		if (!isLanRequest()) {
			$this->generateErrorOutput(401, 'share_names', 'NOT_ALLOWED_IN_WAN', $output_format);
			return;
		}
		
		
		$includeWdSyncFolder = isset($queryParams['include_wd_sync_folder'])  ? trim($queryParams['include_wd_sync_folder'])  : false;
		$includeWdSyncFolder = ($includeWdSyncFolder === 'true' || $includeWdSyncFolder === '1'  || $includeWdSyncFolder === 1) ? true : false;
		
		$shares = self::$userShares->getAllShareNames();
		
		// Generate response
		setHttpStatusCode(200);
		$output = new OutputWriter(strtoupper($output_format));
		$output->pushElement('share_names');
		foreach($shares as $share){
			$output->pushElement('share');
			$output->element('share_name', $share['share_name']);
			$output->element('usb_handle', $share['usb_handle']);
			if($includeWdSyncFolder){
				$syncFolderPath = getSharePath().'/'.$share['share_name'].'/'.'WD Sync';
				if(is_dir($syncFolderPath)) {
					$output->element('wd_sync_folder', 'true');
				}else{
					$output->element('wd_sync_folder', 'false');
				}
			}
			$output->popElement();
		}
		$output->popElement();
		$output->close();
	}
	
}
?>
