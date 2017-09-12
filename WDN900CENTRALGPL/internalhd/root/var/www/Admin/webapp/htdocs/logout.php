


<?php

require_once("secureCommon.inc");
session_start();
// resets the session data for the rest of the runtime
$_SESSION = array();
// sends as Set-Cookie to invalidate the session cookie
if (isset($_COOKIES[session_name()])) { 
    $params = session_get_cookie_params();
    setcookie(session_name(), '', 1, $params['path'], $params['domain'], $params['secure'], isset($params['httponly']));
}
session_destroy();

?>
<html>
<head>
	<script type="text/javascript">
		window.location="<?php echo $portalUrl . '/logout.do'?>";
	</script>
</head>
<body>

</body>
</html>
