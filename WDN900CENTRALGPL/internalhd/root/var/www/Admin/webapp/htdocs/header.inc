
<div class="header">
        <div id="layout" >
                <a href="http://www.wdc.com/"><img src="images/WD_Logo.png" style="float: left; margin:0px;padding:0px;border:0px"></a>
				<div id="title"> <a>My Net N900 Central<?php echo getDeviceUserDisplayName();?></a>  </div>
                <div id="menu" >
					<span><?php echo $_SESSION['username'];?></span>
					<?php if (!empty($_SESSION['username'])) {?>
	                    <span class="separator">|</span>
                    	<a href="<?php echo "$portalUrl/getDevices.do"?>"><?php echo gettext2("WD_DEVICES")?></a>
	                    <span class="separator">|</span>
                    	<a href="logout.php"><?php echo gettext2("LOGOUT") ?></a>
                    <?php }?>

                </div>
        </div>
</div>


