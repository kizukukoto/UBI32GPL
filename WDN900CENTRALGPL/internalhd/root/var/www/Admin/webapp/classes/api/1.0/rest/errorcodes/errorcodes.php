<?php
/**
 * Class ErrorCodes
 *
 * PHP version 5
 *
 * @category  Classes
 * @package   Orion
 * @author    WDMV Software Engineering
 * @copyright 2011 Western Digital Technologies, Inc. - All rights reserved
 * @license   http://support.wdc.com/download/netcenter/gpl.txt GNU License
 * @link      http://www.wdc.com/
 *
 */
require_once('restcomponent.inc');


/**
 * ErrorCodes Class extends the Rest Component and reads the error codes array.
 *
 * @version Release: @package_version@
 *
 */
class ErrorCodes extends RestComponent {

	function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get error codes information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 * @return array $results
	 */
	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		if (isset($urlPath[0])) {
			if (is_numeric($urlPath[0])) {
				$errorCode = trim($urlPath[0]);
			} else {
				$errorName = trim($urlPath[0]);
			}
		} else {
			$errorCode = isset($queryParams['error_code']) ? trim($queryParams['error_code']) : '';
			$errorName = isset($queryParams['error_name']) ? trim($queryParams['error_name']) : '';
		}

		if (!getSessionUserId()) {
			$this->generateErrorOutput(401, 'error_codes', 'USER_LOGIN_REQUIRED', $output_format);
			return;
		}

		if (!isAdmin(getSessionUserId())) {
			$this->generateErrorOutput(401, 'error_codes', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		if (!empty($errorName)) {
			$errorCodes = getErrorCodes($errorName);
			if (!isset($errorCodes['error_code'])) {
				$this->generateErrorOutput(404, 'error_codes', 'ERROR_NAME_NOT_FOUND', $output_format);
				return;
			}

		} else if (!empty($errorCode)) {
			$errorCodes = getErrorCodes(null, $errorCode);
			if (!isset($errorCodes['error_name'])) {
				$this->generateErrorOutput(404, 'error_codes', 'ERROR_CODE_NOT_FOUND', $output_format);
				return;
			}

		} else {
			$errorCodes = getErrorCodes('', '');
			$this->generateCollectionOutput(200, 'error_codes', 'error_code', $errorCodes, $output_format);
			return;

		}
		$this->generateItemOutput(200, 'error_codes', $errorCodes, $output_format);
		return;
	}
}
?>