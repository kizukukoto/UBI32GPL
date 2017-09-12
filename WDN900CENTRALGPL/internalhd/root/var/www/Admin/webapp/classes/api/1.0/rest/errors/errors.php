<?php
/**
 * Class Errors
 *
 * PHP version 5.2.2
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
 * Users Class extends the Rest Component and accesses the Users DB table.
 *
 * @version Release: @package_version@
 *
 */
class Errors extends RestComponent {

	var $errors;

	function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get error information.
	 *
	 * @param array $urlPath
	 * @param array $queryParams
	 * @param string $output_format
	 * @return array $results
	 */
	function get($urlPath, $queryParams=null, $output_format='xml')
	{
		$errorCode = isset($queryParams['error_code']) ? trim($queryParams['error_code']) : '';
		$errorName = isset($queryParams['error_name']) ? trim($queryParams['error_name']) : '';
		$language  = isset($queryParams['language'])   ? trim($queryParams['language'])   : 'en';

		if (!getSessionUserId()) {
			$this->generateErrorOutput(401, 'errors', 'USER_LOGIN_REQUIRED', $output_format, $language);
			return;
		}

		if (!isAdmin(getSessionUserId())) {
			$this->generateErrorOutput(401, 'errors', 'USER_NOT_AUTHORIZED', $output_format, $language);
			return;
		}

		if (!empty($errorCode)) {

			$error = getErrorCodes($errorCode, '', $language);
			if (!isset($error['error_name'])) {
				$this->generateErrorOutput(404, 'errors', 'ERROR_CODE_NOT_FOUND', $output_format, $language);
				return;
			}

		} else if (!empty($errorName)) {

			$error = getErrorCodes('', $errorName, $language);
			if (!isset($error['error_code'])) {
				$this->generateErrorOutput(404, 'errors', 'ERROR_NAME_NOT_FOUND', $output_format, $language);
				return;
			}

		} else {

			$errors = getErrorsCodes('', '', $language);
			$this->generateCollectionOutput(200, 'errors', 'error', $errors, $output_format);
			return;

		}

		$results = array('error_code' => $errorCode, 'error_name' => $errorName, 'error_desc' => $errorDesc);
		$this->generateItemOutput(200, 'errors', $results, $output_format);
		return;
	}
}
?>