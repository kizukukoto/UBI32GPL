<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
	require_once("../includes/util.inc");
	require_once("../includes/NasXmlWriter.class.php");
	
	function genNasListXML($user)
	{
		//test code
		$xml = new NasXmlWriter();
		$xml->push('user', array('name' => 'joe', 'uid' => 'AD1778EFFCC22892', 'sid' => 'FFFEDC12345678912'));
			$xml->push('nas');
				$xml->element('name', 'Golla\'s  NAS');
				$xml->element('url', 'http://12.53.74.67/AlbumChooser.php');
			$xml->pop();
			$xml->push('nas');
				$xml->element('name', 'Office NAS');
				$xml->element('url', 'http://12.53.74.71/AlbumChooser.php');
			$xml->pop();
		$xml->pop();
		return $xml->getXml();
	}
	
	///Test code
	
	//$xmlstr = genNasListXML("joe");
	//var_dump($xmlstr);
	
?>