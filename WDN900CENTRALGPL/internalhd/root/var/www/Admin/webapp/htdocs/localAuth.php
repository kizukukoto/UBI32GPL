<?php
require_once("secureCommon.inc");


$localName = $_POST["localUserName"];
$localPwd = $_POST["localPassword"];


if (authenticateLocalUser($localName, $localPwd)) {

    header('Location: /Admin/webapp/htdocs/mapDrive.php?mapkey='.$_SESSION['mapkey']);

} else {
	$_SESSION['error_msg'] =  gettext2('AUTH_FAILED');
    header('Location: '.$_SERVER['HTTP_REFERER']);
}

?>
