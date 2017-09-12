<?php
	// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
	// Key is the Component name. This is part of the request url.
	// Values is an array with three values:
	//   First one is the file name of the PHP component (including relative path).
	//   Second one is the Component class name.
	//   Third is boolean value to indicate whether component requires authentication.

	$componentConfig = array(
		'album'					=> array('albums/album.php','Album',true),
		"album_access"			=> array("albums/albumaccess.php", "AlbumAccess",true),
		"album_info"			=> array("albums/albuminfo.php", "AlbumInfo",true),
		"album_item_info"		=> array("albums/albumiteminfo.php", "AlbumItemInfo",true),
		'album_item'			=> array('albums/albumitem.php', 'AlbumItem',true),
		'album_item_contents'	=> array('albums/albumitemcontents.php','AlbumItemContents',true),
		'album_demo'			=> array('albumdemo/album.php', 'Albums',true),
		'comp_codes'			=> array('compcodes/compcodes.php','CompCodes',true),
		'config'				=> array('config/config.php','Config',false),
		'device'				=> array('device/device.php','Device',true),
		'device_user'			=> array('device/deviceuser.php','DeviceUser',true),
		'device_user_notify'	=> array('device/deviceusernotify.php','DeviceUserNotify',true),
		'dir'					=> array('dir/dir.php','Dir',true),
		'dir_demo'				=> array('dirdemo/dirdemo.php', 'Dir',true),
		'drives'				=> array('drives/drives.php','Drives',true),
		'error_codes'			=> array('errorcodes/errorcodes.php','ErrorCodes',true),
		'file'					=> array('file/file.php','File',true),
		'file_contents'			=> array('filecontents/filecontents.php','FileContents',true),
		"image"					=> array("image/image.php","Image",true),
		'local_login'			=> array('login/locallogin.php','LocalLogin',false),
		'login'					=> array('login/login.php','Login',false),
		'logout'				=> array('logout/logout.php','Logout',false),
		'metadb_info'			=> array('metadbinfo/metadbinfo.php','MetaDBInfo',true),
		'metadb_summary'		=> array('metadbsummary/metadbsummary.php','MetaDBSummary',true),
		'miocrawler_status'		=> array('miocrawlerstatus/miocrawlerstatus.php','MioCrawlerStatus',true),
		'miodb'					=> array('miodb/miodb.php','MioDB',true),
		'nas_list'				=> array('naslist/naslist.php','NasList',true),
		'port_test'				=> array('porttest/porttest.php','PortTest',false),
		'remote_account'		=> array('remoteuser/remoteaccount.php','RemoteAccount',true),
		"status"				=> array('status/status.php','Status',true),
		'users'					=> array('users/users.php','Users',true),
		'verify_remote_user'	=> array('remoteuser/verifyremoteuser.php','VerifyRemoteUser',true),
		"version"				=> array("version/version.php","Version", false),
		'volumes'				=> array('volumes/volumes.php','volumes',true),
	);
?>