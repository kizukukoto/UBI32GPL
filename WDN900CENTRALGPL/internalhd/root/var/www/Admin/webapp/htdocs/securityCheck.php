<?php
require_once 'secureCommon.inc';

$devUserId = $_GET["deviceUserId"];
$authCode = $_GET["deviceUserAuthCode"];

$devUserDao = new DeviceUsersDB();


$_SESSION['mapkey']="__".strval(rand())."__";
$_SESSION[$_SESSION['mapkey']] = array('deviceUserId' => $devUserId, 'deviceUserAuthCode' => $authCode);



/*
$subDHostName = $_SERVER["HTTP_HOST"];
$subDPort = $_SERVER["SERVER_PORT"];
$subDProtocol = "http://";
if ($_SERVER["HTTPS"] === "on") {
    $subDProtocol = "https://";  
}
$subDUrl = $subDProtocol . $subDHostName;
*/
$deviceUser = $devUserDao->getDeviceUser($devUserId);
$userDao = new UsersDB();

if ($devUserDao->isValid($devUserId, $authCode)) {
   $deviceUser = $devUserDao->getDeviceUser($devUserId); 
   $user = $userDao->getUser($deviceUser["user_id"]);
   $_SESSION['username'] = $user['username'];
   $_SESSION['fullname'] = $user['fullname'];

}
if (!$devUserDao->isValid($devUserId, $authCode) || !isLocalUser($deviceUser["user_id"])) {
	header('Location: /Admin/webapp/htdocs/accessDenied.php');
} else {
	
	if (!isPasswordRequiredForLocalUser($user['username'])) {
		//echo isLocalUser($deviceUser["user_id"]).' nopwd '. $deviceUser["user_id"];
	    //redirect
    	header('Location: /Admin/webapp/htdocs/mapDrive.php?mapkey='.$_SESSION['mapkey']);
		return;
	}
	
    //local auth

?>

<!DOCTYPE html>
<html>
	<head>
		<meta http-equiv="X-UA-Compatible" content="IE=edge" >
		<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
		<LINK REL=StyleSheet HREF="css/main.css" TYPE="text/css">
		<script type="text/javascript" src="js/jquery.js"> </script>
		<title><?php echo getDeviceUserDisplayName();?>My Net N900 Central</title>
        <link rel="shortcut icon" href="images/WD2go_Favicon.ico" type="image/x-icon"/>
	</head>

	<body>

	<?php include('header.inc');?>


				
		<form name="localAuthForm" method="post" action="localAuth.php" id="loginForm">			
            <input name="localUserName" type="hidden" value='<?php echo $user["username"]?>'/>
			
			<div class="topGrad">
				<img src='images/WD2go_ColorStrip.png'/>

				
				<div class="contentTables">
            	<span class='title'><?php echo gettext2("WELCOME") ?> <?php echo $user["username"]?></span>
					<div class='titleSeperatorSpacing'>
						<div class='titleSeperator'>
						</div>
					</div>
					
					<br/>
					<table class="formTable">

						<tr>
							<td valign='middle'><?php echo gettext2("PASSWORD") ?><br/></td>
							<td>
								<div class='roundBox'>
									<div class="roundBoxRight">
										<input  type='password' size='40' class='textInput' value='' name='localPassword'/>
									</div>
								</div>
							</td>
							<td valign='middle'><a class='errorAlert'></a></td>
						</tr>
						<tr>
							<td valign='middle'></td>
							<td>
								<div class=" errorAlert" style="height:8px; padding-left:18px;" > 
								<?php 
									if (!empty($_SESSION['error_msg'])) {
										echo $_SESSION['error_msg'];
										unset($_SESSION['error_msg']);
									}
								?>
								</div>
							</td>
							<td valign='middle'></td>



						</tr>
<!--
						<tr>
							<td>&nbsp;</td>

							<td>
								<div style="margin-left: 9px;">

									<input type='checkbox' name='remmeberMe'/>&nbsp;
									<div style="display:inline-block;">
										<a class='normalTitle'><?php echo gettext2('REMEMBER_ME') ?></a>
										&nbsp;&nbsp;
									</div>

								</div>

							</td>
							<td>&nbsp;</td>
						</tr>
-->
						<tr>
							<td>&nbsp;</td>
							<td align='right'>

								<a href="<?php echo $portalUrl . '/getDevices.do'?>" style="float:left" class="button">
									<span style="padding-right:15px; padding-left: 25px"><?php echo gettext2('CANCEL') ?></span> 
								</a>
								<a href="#" id ="submit" style="float:left" class="button">
									<span style="padding-right:15px; padding-left: 25px"><?php echo gettext2('SIGN_IN') ?></span> 
								</a>


							</td>							
							<td valign='middle'><a class='errorAlert'></a></td>
						</tr>

					</table>					
				</div>		
				
				<div class='bottomGlow'>
					<img src="images/WD2go_Glow.png" align='bottom'/>
				</div>		
													 				
			</div>						
		</form>

		<script type="text/javascript">
		
		(function() {

			$(document).ready(function() {
				//init sign in button
				$('a.button#submit').click(function() {
					$('form#loginForm').submit();
				});
				
				
				//init checkbox
				$('input[type=checkbox]').each(function(i,e) {
				    var $chkbox = $(e);
				    var $stylebox = $('<span class="checkbox_unchecked"> </span>');
				    $stylebox.data('isChecked', false);
				    
				    $chkbox.hide();
				    $stylebox.insertBefore($chkbox);
				    
				    //install event listener
				    $stylebox.click(function() {
				    	if ($stylebox.data('isChecked')) {
				    		$stylebox.removeClass();
				    		$stylebox.addClass('checkbox_unchecked');
				    		$chkbox.attr('checked', false);
				    		$stylebox.data('isChecked', false);
				    	} else {
				    		$stylebox.removeClass();
				    		$stylebox.addClass('checkbox_checked');
				    		
				    		$chkbox.attr('checked', true);
				    		$stylebox.data('isChecked', true);
				    	}
				    	
				    });

				});
			});

		})();
		</script>
	</body>
</html>


<?php } ?>
