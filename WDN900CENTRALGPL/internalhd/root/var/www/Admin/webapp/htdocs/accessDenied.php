<?php
require_once('secureCommon.inc');

?>

<!DOCTYPE html>
<html>
	<head>
		<meta http-equiv="X-UA-Compatible" content="IE=edge" >
		<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">		
		<LINK REL=StyleSheet HREF="css/main.css" TYPE="text/css">
		<style>
		.contentTables td {
            color: #FFFFFF;
        }

        .formTable .roundBox input {
            height: 48px;
        }	
		</style>

		
		<script type="text/javascript" src="js/jquery.js"> </script>
	</head>

	<body>
	<?php include ('header.inc') ?>
				
			<div class="topGrad">
				<img src='images/WD2go_ColorStrip.png'/>

				
				<div class="contentTables">
		            <span class='title'><?php echo gettext2('ACCESS_DENIED')?></span>
					<div class='titleSeperatorSpacing'>
						<div class='titleSeperator'>
						</div>
					</div>
					
					<br/>
					<p>
						<?php
							$_link_ = '"logout.php"'; 
							eval('echo "' . addslashes(gettext2('ACCESS_DENIED_CONTENT')) . '";');

						?>
					</p>
			
				</div>		
				
				<div class='bottomGlow'>
					<img src="images/WD2go_Glow.png" align='bottom'/>
				</div>		
													 				
			</div>						
		
	</body>
</html>

