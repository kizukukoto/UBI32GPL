<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.

/**
 * Setup include paths for webapp
 */
define('ADMIN_API_ROOT', $_SERVER["__ADMIN_API_ROOT"] . '/');

ini_set('include_path',
	'.' . ':' . ADMIN_API_ROOT . 'webapp/includes/' . ':'
	. ADMIN_API_ROOT . 'webapp/classes/api/' . ':'
	. ini_get('include_path')
	);

/**
 * Register autoload function
 */
function __autoload($className) {
	$className = strtolower($className);
	@(include_once("$className.class.inc"))
		OR require_once("$className.inc");
}

/**
 * Load global configuration file
 */
require_once 'globalconfig.inc';

/**
 * Load global constants file
 */
require_once 'globalconstants.inc';

/**
 * Load utility functions
 */
require_once 'util.inc';

/**
 * Initialize NAS controller
 */
require_once('1.0/rest/nascontroller.php');

$nasController = new NasController();
$nasController->exec();
?>