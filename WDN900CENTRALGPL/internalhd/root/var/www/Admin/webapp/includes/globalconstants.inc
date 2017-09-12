<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.

define ( 'DS', DIRECTORY_SEPARATOR );

define ( 'PS', '/' );

define ('TMP', sys_get_temp_dir() );

// @todo mikeC: test DEVICETYPE (global constant) to determine OS path.. maybe there should be a 'OS_TYPE'
// define ('APACHE_CONF_PATH', '/etc/apache2');
define ('APACHE_CONF_PATH', 'C:/Program Files/Apache Software Foundation/Apache2.2/conf');

define('SKIP_SHELL_SCRIPT', true);

// definition fo Stringray device type
define('STINGRAY_DEVICE_TYPE', 5);

?>