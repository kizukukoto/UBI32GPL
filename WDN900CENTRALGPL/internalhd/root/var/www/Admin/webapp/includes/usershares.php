<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

require_once('usersdb.inc');
require_once('usersharesdb.php');
require_once('usersharesshell.php');
require_once ('shareaccessshell.php');
require_once('logMessages.php');
require_once('globalconfig.inc');

/**
 * Control class for User Samba Shares
 * 
 * @author sapsford_j
 *
 */

class UserShares {

	static $userSharesDB;
	static $userSharesShell;
	static $shareAccessShell;
	static $shareAccess;
	static $logObj;

	/**
	 * Constructor - create static instances of classes used by this class
	 */
	
	public function __construct() {
		if (!isset(self::$userSharesDB)) {
			self::$userSharesDB = new UserSharesDB(); 
		}
		if (!isset(self::$userSharesShell)) {
			self::$userSharesShell = new UserSharesShell();
		}
		if (!isset(self::$shareAccessShell)) {
			self::$shareAccessShell = new ShareAccessShell();
		}
		if (!isset(self::$logObj)) {
			self::$logObj = new LogMessages();
		}
		if (!isset(self::$shareAccess)) {
			self::$shareAccess = new ShareAccess();
		}
	}

	/**
	 * Get the details for all shares, or a single named share (optional)
	 *  
	 * @param string $shareName name of share. If this is omitted or null, then the details for all
	 * shares will be returned.
	 */
	
	public function getShares( $shareName=NULL){
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
		if (!isset($shareName)) {
			$shareList = self::$userSharesDB->getShares();
		}
		else {
			$shareList = self::$userSharesDB->getShareForName($shareName);
		}
		if (!isset($shareList) || sizeof($shareList) == 0) {
			return null; //TBA: return errot code 
		}

		// Get the size used by each share.  If the share is from a dynamic volume, get the
		// size using the disk free utility and convert the result from KiB to MB.

		for ($i=0; $i < sizeof($shareList); $i++) {
			$shareName = $shareList[$i]['share_name'];
			if (strcasecmp($shareList[$i]['dynamic_volume'],'true') != 0) {
				$shareSize = self::$userSharesShell->getShareSize($shareName);
			}
			else {
				unset($output);
				exec("df | grep \"/$shareName\$\" | awk '{printf(\"%.0f\",($3 * 1024)/1000000)}'", $output, $retVal);
				if(($retVal !== 0) || (!isset($output[0])) || ($output[0] == "")){
					$output[0] = "0";
				}
				$shareSize = trim($output[0]);
			}
			if ($shareSize) {
				$shareList[$i]['size'] = $shareSize;
			} else {
				//TBA: log error
				$shareList[$i]['size'] = 0;
			}
		}
		return $shareList;
	}
	
	/**
	 * Create a new Share
	 * 
	 * @param unknown_type $shareName name of share - must be unique
	 * @param unknown_type $shareDescription - descripion of share
	 * @param unknown_type $publicAccess whether public access is allowed (true/false)
	 * @param unknown_type $mediaServing type of media served by the share
	 * @param unknown_type $remoteAccess whether remote access is allowed (true/false)
     * @param unknown_type $dynamicVolume indicates if share is from removable storage (true/false)
     * @param unknown_type $capacity (only meaningful for dynamic volumes) maximum size of share in MB
     * @param unknown_type $readOnly (only meaningful for dynamic volumes) indicates if share can only be read (true/false)
     * @param unknown_type $usbhandle (only meaningful for dynamic volumes) handle of USB device associated with volume
	 */

	public function createShare($shareName, $shareDescription, $publicAccess, $mediaServing, $remoteAccess, $dynamicVolume='false', $capacity=null, $readOnly=null, $usbHandle=null, $fileSystemType='')  {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, shareDescription=$shareDescription, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess, dynamicVolume=$dynamicVolume, capacity=$capacity, readOnly=$readOnly, usbHandle=$usbHandle, fileSystemType=$fileSystemType)");
		$status = self::$userSharesShell->createShare($shareName, $shareDescription); 	
		if ($status === true) {
			$status = self::$shareAccessShell->updateShareAccess($shareName, $publicAccess, $mediaServing, $remoteAccess);
			if ($status === true) {
				$status = self::$userSharesDB->createUserShare($shareName, $shareDescription, $publicAccess, $mediaServing, $remoteAccess, $dynamicVolume, $capacity, $readOnly, $usbHandle, $fileSystemType);
				$this->updateMediaCrawlerConfigFile();
			}
		}
		return $status;
	}
	
	/**
	 * Update a Share
	 * 
	 * @param unknown_type $shareName name of share - must be unique
	 * @param unknown_type $newShareName new share name if renaming share, if not renaming must be set to exisitng na  me of share
	 * @param unknown_type $shareDescription - descripion of share
	 * @param unknown_type $publicAccess whether public access is allowed (true/false)
	 * @param unknown_type $mediaServing type of media served by the share
	 * @param unknown_type $remoteAccess whether remote access is allowed (true/false)
	 */

	public function updateShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName, newShareName=$newShareName, newDescription=$newDescription, publicAccess=$publicAccess, mediaServing=$mediaServing, remoteAccess=$remoteAccess)");

		$status = self::$userSharesShell->updateShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess); 	
		if ($status === true) {
			$status = self::$userSharesDB->updateUserShare($shareName, $newShareName, $newDescription, $publicAccess, $mediaServing, $remoteAccess);
			$this->updateMediaCrawlerConfigFile();
		}
		return $status;
	}
	
	/**
	 * Deletes a given share
	 * 
	 * @param unknown_type $shareName name of share - must match an existig share
	 */

	public function deleteUserShare($shareName) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (shareName=$shareName)");
		$status = self::$userSharesShell->deleteShare($shareName); 	
		if ($status === true) {
			//delete share access
			$status = self::$shareAccess->deleteAllAccessForShare($shareName);
			if ($status == false) {
				//log error - there is no log messages function as of now in LogMessages class
			}
			//delete share - even if there is no ACL, we still should delete the share from the DB, if it exists
			$status = self::$userSharesDB->deleteUserShare($shareName);
			//update share entry in mediacrawler config file
			$this->updateMediaCrawlerConfigFile();
		}
		return $status;
	}

	/**
	 * Deletes all of the shares belonging to a specific user (for 3.5G this will wlways be teh Admin user)
	 * 
	 * @param unknown_type $userId - Id of user
	 */
	
	public function deleteAllSharesForUser($userId) {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: (userId=$userId)");
		$shares = $this->getShares($userId);
		if (!$shares) {
			self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__, "ERROR: userId=$userId owns no shares");
			return false;
		}
		$status = false;
		foreach ($shares as $share) {
			$status = self::$userSharesShell->deleteShare($share['share_name']);
			if (!$status) {
				break;
			}
		}
		if ($status) {
			 $status = self::$userSharesDB->deleteAllSharesForUser($userId);
		} else {
			//TBA: log error
		}
		return $status;
	}

	/**
	 * Deletes all user shares
	 */
	
	public function deleteAllUserShares() {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
		$status = self::$userSharesShell->deleteAllShares();
		if ($status) {
			 $status = self::$userSharesDB->deleteAllShares();
		}
		return $status;
	}
	
	/**
	 * Saves the current state of all shares
	 */
	public function saveShareState() {
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "PARAMS: ");
		$status = self::$userSharesShell->saveUserShareState();
		return $status;
	}
	
	
	/**
	 * Writes the file paths of each share to the given file name 
	 * @param $tempFile name of file - must be open for write prior to calling this function and closed
	 * after calling this function.
	 * 
	 * FIX ME: this code should be moved into updateMediaCrawlerConfigFile(), there
	 * doesn't seem to be any reason to put this  code in a seperate function.
	 */
	function writeCurrentPaths($tempFile) {
		$dbAccess = new DBAccess();
		$shares = self::$userSharesDB->getActiveShares();
		foreach ($shares as $share) {
			fprintf($tempFile, "\t\t<Path>/".$share[share]."</Path>\n");
		}
	}

	/**
	 * Updates the Media Crawler confguration XML file with the details of each share. This function should be called
	 * after creatiing, updating or deleting shares
	 * 
	 */
	function updateMediaCrawlerConfigFile() {
		/*
		$currentFilePath = getMediaCrawlerHostFilePath();;
		$tempFilePath = '/tmp/temp.xml';
		$currentFile = fopen($currentFilePath, "r") or exit("Unable to open file!");
		$tempFile = fopen($tempFilePath, "w") or exit("Unable to open file!");
		$inPaths = false;
		while (!feof($currentFile)) {
			$inputStr = fgets($currentFile);
			if(strpos($inputStr, '<CrawlPaths>') !== false ) {
				$inPaths = true;
				fwrite($tempFile, $inputStr);
				$this->writeCurrentPaths($tempFile);
			}
			if(strpos($inputStr, '</CrawlPaths>') !== false ) {
				$inPaths = false;
			}
			if(! $inPaths) {
				fwrite($tempFile, $inputStr);
			}
		}
		fclose($currentFile);
		fclose($tempFile);
		@unlink($currentFilePath);
		//Move hosts file.
		$cmd = "sudo mv ".$tempFilePath." ".$currentFilePath;
		exec($cmd, $output, $retVal);
		// Change hosts file permissions 
		$cmd = "sudo chmod 0644 ".$currentFilePath;
		exec($cmd, $output, $retVal);
		//rename($tempFilePath, $currentFilePath);
		// restart media crawler
		*/
		$mediaCrawlerpath = getMediaCrawlerPath();
		$cmd = "sudo ".$mediaCrawlerpath." restart";
		exec($cmd, $output, $retVal);
	}
	
	/**
	 * Returns list of all share names both public and private.
	 */
	public function getAllShareNames(){
		self::$logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  "");
		$shareList = self::$userSharesDB->getAllShareNames();
		return $shareList;
	}
	
}

/** TEST CODE **/
/*
$us = new UserShares();
//$us->deleteAllUserShares(); //WARNING do not run this test against a live DB as ALL shares will be deleted !!!


$us->createShare("burt1", "Burt's share", "false", "all", "true","false",'1234000000','false','','');
$us->createShare("burt2", "Burt's Other share", "false", "music", "true","true",'1010101010','true','A12FF13BC2E','NTFS');
$us->createShare("ralph", "Ralph's share", "true", "videos", "false","true",'4567808989','true','F45EFFF1212CD44','EXT4');
$shares = $us->getShares();
var_dump($shares);

$shares = $us->getShares(1);
var_dump($shares);
$shares = $us->getShares(1,"burt2");
var_dump($shares);
$us->updateShare(1, "burt2", "burt2_update", "Burt's updated share", "endabled", "photos", "enabled");
$shares = $us->getShares(1,"burt2_update");
var_dump($shares);
$us->deleteUserShare(2, "ralph");
$shares = $us->getShares();
var_dump($shares);
$us->deleteAllSharesForUser(1);
$shares = $us->getShares();
var_dump($shares);
$us->deleteAllUserShares();
$shares = $us->getShares();
var_dump($shares);
*/
?>