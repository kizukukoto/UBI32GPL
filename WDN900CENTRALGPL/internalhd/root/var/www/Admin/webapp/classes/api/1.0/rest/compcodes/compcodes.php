<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('restcomponent.inc');

class CompCodes extends RestComponent {

	function __construct()
	{
		require_once('globalconfig.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}


	/**
	 * Get component codes information.
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
				$compCode = trim($urlPath[0]);
			} else {
				$compName = trim($urlPath[0]);
			}
		} else {
			$compCode = isset($queryParams['comp_code']) ? trim($queryParams['comp_code']) : '';
			$compName = isset($queryParams['comp_name']) ? trim($queryParams['comp_name']) : '';
		}

		if (!getSessionUserId()) {
			$this->generateErrorOutput(401, 'comp_codes', 'USER_LOGIN_REQUIRED', $output_format);
			return;
		}

		if (!isAdmin(getSessionUserId())) {
			$this->generateErrorOutput(401, 'comp_codes', 'USER_NOT_AUTHORIZED', $output_format);
			return;
		}

		if (!empty($compName)) {
			$compCodes = getComponentCodes($compName);
			if (!isset($compCodes['code'])) {
				$this->generateErrorOutput(404, 'comp_codes', 'COMP_NAME_NOT_FOUND', $output_format);
				return;
			}

		} else if (!empty($compCode)) {
			$compCodes = getComponentCodes(null, $compCode);
			if (!isset($compCodes['name'])) {
				$this->generateErrorOutput(404, 'comp_codes', 'COMP_CODE_NOT_FOUND', $output_format);
				return;
			}

		} else {
			$compCodes = getComponentCodes('', '');
			$this->generateCollectionOutput(200, 'comp_codes', 'comp_code', $compCodes, $output_format);
			return;

		}
		$this->generateItemOutput(200, 'comp_codes', $compCodes, $output_format);
		return;
	}
}
?>