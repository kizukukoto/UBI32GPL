<?php

require_once('logMessages.php');

class HiddenShareFiles{

	var $logObj;
	function HiddenShareFiles(){
		$this->logObj = new LogMessages();
	}
    function getShares(){
		
		$share = array('share' => array());

		unset($outputBackupShares);
		exec("sudo /usr/local/sbin/getBackupShares.sh", $outputBackupShares, $retVal);
		if($retVal !== 0) {
			$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'server error getBackupShares');
			return 'SERVER_ERROR';
		}

		foreach($outputBackupShares as $testShare){				

			$share['share']["$testShare"] = array('sharename' => "$testShare", 'file' => array());
						
			unset($outputBackupList);
			exec("sudo /usr/local/sbin/getBackupShareList.sh \"$testShare\"", $outputBackupList, $retVal);
			if($retVal !== 0) {
				$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'server error getting backup share list');
				
				return 'SERVER_ERROR';
			}
			
			foreach($outputBackupList as $testBackup)
			{

				setlocale(LC_ALL, "en_US.UTF-8");
				unset($outputBackupModTime);
				exec("sudo /usr/local/sbin/getBackupModTime.sh \"$testShare\" " . escapeshellarg($testBackup), $outputBackupModTime, $retVal);
				if($retVal !== 0) {
					$this->logObj->LogParameters(get_class($this), __FUNCTION__, $testShare);
					$this->logObj->LogParameters(get_class($this), __FUNCTION__, $testBackup);
					$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'server error getBackupModTime');
					return NULL;
				}
				$backupModTime = $outputBackupModTime[0];
				
				unset($outputBackupSize);
				exec("sudo /usr/local/sbin/getBackupSize.sh \"$testShare\" " . escapeshellarg($testBackup), $outputBackupSize, $retVal);
				if($retVal !== 0) {
					$this->logObj->LogData('OUTPUT', get_class($this),  __FUNCTION__,  'server error getBackupSize');
					return NULL;
				}
				
				$backupSize = $outputBackupSize[0];
				
				array_push($share['share']["$testShare"]['file'], 
					array('backup_name' => $testBackup, 'file_time_stamp' => $backupModTime, 'size' => $backupSize));
				
			}

		}
		return($share);
    }
	
	function deleteBackup($shareName, $backupName){
		
		unset($outputBackupShares);
		exec("sudo /usr/local/sbin/getBackupShares.sh", $outputBackupShares, $retVal);
		if($retVal !== 0) {
			return 'SERVER_ERROR';
		}
		
		//find the backup share and the backup in it, if found then delete it
		foreach($outputBackupShares as $testShare){				

			// if found the share
			if (strcasecmp($testShare, $shareName) ==0) {			

				unset($outputBackupList);
				exec("sudo /usr/local/sbin/getBackupShareList.sh \"$shareName\"", $outputBackupList, $retVal);
				if($retVal !== 0) {
					return 'SERVER_ERROR';
				}

				foreach($outputBackupList as $testBackup)
				{
					// if found the backup, delete the backup
					if (strcasecmp($testBackup, $backupName) == 0) {
						
						unset($output);
                        setlocale(LC_ALL, "en_US.UTF-8");
                        exec("sudo /usr/local/sbin/deleteBackup.sh \"$shareName\" " . escapeshellarg($backupName), $output, $retVal);
						if($retVal !== 0) {
							return 'SERVER_ERROR';
						}
						return 'SUCCESS';	

					}

				}
			}		

		}
		
		// if we've gotten here then the backup could not be found, return BACKUP_NOT_FOUND
		
		return 'BACKUP_NOT_FOUND';	
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
