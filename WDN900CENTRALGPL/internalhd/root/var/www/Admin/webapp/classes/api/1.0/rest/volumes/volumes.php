<?php
/**
 * Class Volumes
 *
 * PHP version 5.2
 *
 * @category  Classes
 * @package   Orion
 * @author    WDMV Software Engineering
 * @copyright 2011 (c) Western Digital Technologies, Inc. - All rights reserved
 * @license   http://support.wdc.com/download/netcenter/gpl.txt GNU License
 * @link      http://www.wdc.com/
 *
 */
require_once('restcomponent.inc');

/**
 * Volumes Class extends the Rest Component and accesses the Volumes database table.
 *
 * @version Release: @package_version@
 *
 */
class Volumes extends RestComponent {

	function __construct() {
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('usersharesdb.php');
		require_once('util.inc');
		require_once('volumesdb.inc');
	}

	/**
	 * Get volume information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	function get($urlPath, $queryParams=null, $output_format='xml') {
		$uuid = !empty($urlPath[0]) ? $urlPath[0] : '';
		$volumesDB = new VolumesDB();
		if (!empty($uuid)) {
			$volume = $volumesDB->getVolume($uuid);
			if (empty($volume)) {
				$this->generateErrorOutput(404, 'volumes', 'VOLUME_NOT_FOUND', $output_format);
				return;
			}
			$this->generateItemOutput(200, 'volumes', $volume, $output_format);
			return;
		}
		$volumes = $volumesDB->getVolume();
		if (empty($volumes)) {
			$this->generateErrorOutput(404, 'volumes', 'NO_VOLUMES_FOUND', $output_format);
			return;
		}
		$this->generateCollectionOutput(200, 'volumes', 'volume', $volumes, $output_format);
	}

	/**
	 * Create a new volume.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function post($urlPath, $queryParams=null, $output_format='xml') {
		$uuid        = isset($queryParams['uuid'])         ? trim($queryParams['uuid'])         : null;
		$label       = isset($queryParams['label'])        ? trim($queryParams['label'])        : null;
		$basePath    = isset($queryParams['base_path'])    ? trim($queryParams['base_path'])    : null;
		$devPath     = isset($queryParams['dev_path'])     ? trim($queryParams['dev_path'])     : null;
		$drivePath   = isset($queryParams['drive_path'])   ? trim($queryParams['drive_path'])   : null;
		$fsType      = isset($queryParams['fs_type'])      ? trim($queryParams['fs_type'])      : null;
		$isConnected = isset($queryParams['is_connected']) ? trim($queryParams['is_connected']) : null;

		if (empty($uuid) || empty($label) || empty($basePath) || empty($drivePath)) {
			$this->generateErrorOutput(400, 'volumes', 'PARAMETER_MISSING', $output_format);
			return;
		}

		if (!is_dir($basePath) || !is_dir($drivePath)) {
			$this->generateErrorOutput(403, 'volumes', 'VOLUME_CREATE_FAILED', $output_format);
			return;
		}	
		
		$volumesDB = new VolumesDB();

		$volume = $volumesDB->getVolume($uuid);
		if (!empty($volume)) {
			$this->generateErrorOutput(400, 'volumes', 'VOLUME_EXISTS', $output_format);
			return;
		}

		try {
			$status = $volumesDB->createVolume($uuid, $label, $basePath, $devPath, $drivePath, $fsType, $isConnected);
			if (!$status) {
				$this->generateErrorOutput(403, 'volumes', 'VOLUME_CREATE_FAILED', $output_format);
				return;
			}

			### Create an initial share (share name same as volume label)
			$shareName    = $label;
			$shareDesc    = 'Volume: '.$label;
			$publicAccess = 'true';
			$mediaServing = 'all';
			$remoteAccess = 'true';
			$UserSharesDB = new UserSharesDB();
			$share_id = $UserSharesDB->createUserShare($shareName, $shareDesc, $publicAccess, $mediaServing, $remoteAccess);
			if ($share_id == -1) {
				$this->generateErrorOutput(403, 'volumes', 'VOLUME_CREATE_SHARE_FAILED', $output_format);
				return;
			}

			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'volumes', $results, $output_format);
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'volumes', $e->getMessage(), $output_format);
		}
	}

	/**
	 * Update existing volume information.
	 *
	 * @param array  $urlPath
	 * @param array  $queryParams
	 * @param string $output_format
	 */
	function put($urlPath, $queryParams=null, $output_format='xml') {
		$uuid        = !empty($urlPath[0])                 ? $urlPath[0]                        : null;
		$label       = isset($queryParams['label'])        ? trim($queryParams['label'])        : null;
		$basePath    = isset($queryParams['base_path'])    ? trim($queryParams['base_path'])    : null;
		$devPath     = isset($queryParams['dev_path'])     ? trim($queryParams['dev_path'])     : null;
		$drivePath   = isset($queryParams['drive_path'])   ? trim($queryParams['drive_path'])   : null;
		$fsType      = isset($queryParams['fs_type'])      ? trim($queryParams['fs_type'])      : null;
		$isConnected = isset($queryParams['is_connected']) ? trim($queryParams['is_connected']) : null;

		if (empty($uuid)) {
			$this->generateErrorOutput(400, 'volumes', 'PARAMETER_MISSING', $output_format);
			return;
		}

		$volumesDB = new VolumesDB();

		$volume = $volumesDB->getVolume($uuid);
		if (empty($volume)) {
			$this->generateErrorOutput(404, 'volumes', 'VOLUME_NOT_FOUND', $output_format);
			return;
		}

		try {
			$status = $volumesDB->updateVolume($uuid, $label, $basePath, $devPath, $drivePath, $fsType, $isConnected);

			if (!empty($label)) {
				### Update Share Name and Share Description
				$shareName    = $volume['label'];
				$newShareName = $label;
				$newShareDesc = 'Volume: '.$label;
				$UserSharesDB = new UserSharesDB();
				$status = $UserSharesDB->updateUserShare($shareName, $newShareName, $newShareDesc);
				if ($status === false) {
					$this->generateErrorOutput(500, 'volumes', 'VOLUME_UPDATE_SHARE_FAILED', $output_format);
					return;
				}
			}

			if ($isConnected == 'true') {
				### TODO: Mark associated shares as connected
			} else {
				### TODO: Mark associated shares as disconnected
			}

			if (!empty($drivePath)) {
				### TODO: Add associated shares
			} else {
				### TODO: Remove associated shares
			}

			$results = array('status' => 'success');
			$this->generateSuccessOutput(200, 'volumes', $results, $output_format);
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'volumes', $e->getMessage(), $output_format);
		}
	}

	/**
	 * Delete volume information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 */
	function delete($urlPath, $queryParams=null, $output_format='xml') {
		$uuid = !empty($urlPath[0]) ? $urlPath[0] : '';
		try {
			$volumesDB = new VolumesDB();
			if (!empty($uuid)) {
				$volume = $volumesDB->getVolume($uuid);
				if (empty($volume)) {
					$this->generateErrorOutput(404, 'volumes', 'VOLUME_NOT_FOUND', $output_format);
					return;
				}

				### Delete associated shares
				$shareName = $volume['label'];
				$UserSharesDB = new UserSharesDB();
				$status = $UserSharesDB->deleteUserShare($shareName);
				if (!$status) {
					$this->generateErrorOutput(500, 'volumes', 'VOLUME_DELETE_SHARE_FAILED', $output_format);
					return;
				}

				$status = $volumesDB->deleteVolume($uuid);
				$results = array('status' => 'success');
				$this->generateSuccessOutput(200, 'volumes', $results, $output_format);
			} else {
				$volumes = $volumesDB->getVolume();
				foreach($volumes as $volume) {
					$uuid = $volume['uuid'];
					$status = $volumesDB->deleteVolume($uuid);

					### Delete associated shares
					$shareName = $volume['label'];
					$UserSharesDB = new UserSharesDB();
					$status = $UserSharesDB->deleteUserShare($shareName);
					if (!$status) {
						$this->generateErrorOutput(500, 'volumes', 'VOLUME_DELETE_SHARE_FAILED', $output_format);
						return;
					}

				}
				$results = array('status' => 'success');
				$this->generateSuccessOutput(200, 'volumes', $results, $output_format);
			}
		} catch (Exception $e) {
			$this->generateErrorOutput($e->getCode(), 'volumes', $e->getMessage(), $output_format);
		}
	}
}
?>