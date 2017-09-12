<?php

/**
 *
 * Constants for alert php code
 * 
 * @author Sapsford_J
 */

$SEVERITIES = array("CRITICAL","WARNING","INFO");

$CRITICALSTART = 1;
$WARNINGSTART = 1000;
$INFOSTART = 2000;

//CRITICAL ALERTS

$CRITICALALERTS = 
	array (
		$CRITICALSTART     => "System over-temperature",
		$CRITICALSTART + 1 => "System under-temperature",
		$CRITICALSTART + 2 => "Drive SMART failure",
		$CRITICALSTART + 3 => "Volume %1 failed to mount",
		$CRITICALSTART + 4 => "Volume %1 does not exist"
		);
		
$WARNINGALERTS =
	array (
		$WARNINGSTART     => "Disk usage is above 95%"
	);

$INFOALERTS =
	array (
		$INFOSTART        => "System shutdown",
		$INFOSTART + 1    => "System restart",
		$INFOSTART + 2    => "A new version of the firmware is available for download"
	);
	

?>