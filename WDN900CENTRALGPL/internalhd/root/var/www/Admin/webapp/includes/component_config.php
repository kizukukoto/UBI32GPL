<?php
  // Key is the Component name. This is part of the request url.
  // Values is an array with two values:
  //   First one is the file name of the PHP component (including relative path).
  //   Second one is the Component class name.

$NO_AUTH=1; 		// Only used for login
$USER_AUTH=2;		// Requires user authentication (LAN/WAN)
$ADMIN_AUTH=3;		// Requires Admin user (LAN/WAN)
$NO_AUTH_LAN=4;		// No authentication if request is from LAN
$USER_AUTH_LAN=5;	// Requires User authentication and request is from LAN.
$ADMIN_AUTH_LAN=6;	// Requires Admin User authentication and request is from LAN.
$USER_AUTH_ALL = array('GET' => $USER_AUTH, 'POST' => $USER_AUTH, 'PUT' => $USER_AUTH, 'DELETE' => $USER_AUTH);
$ADMIN_AUTH_LAN_ALL = array('GET' => $ADMIN_AUTH_LAN, 'POST' => $ADMIN_AUTH_LAN, 'PUT' => $ADMIN_AUTH_LAN, 'DELETE' => $ADMIN_AUTH_LAN);
$ADMIN_AUTH_LAN_MODIFY = array('GET' => $USER_AUTH, 'POST' => $ADMIN_AUTH_LAN, 'PUT' => $ADMIN_AUTH_LAN, 'DELETE' => $ADMIN_AUTH_LAN);
$ADMIN_AUTH_LAN_GET_NO_AUTH_LAN = array('GET' => $NO_AUTH_LAN, 'POST' => $ADMIN_AUTH_LAN, 'PUT' => $ADMIN_AUTH_LAN, 'DELETE' => $ADMIN_AUTH_LAN);
$USER_AUTH_GET_NO_AUTH_LAN = array('GET' => $NO_AUTH_LAN, 'POST' => $USER_AUTH, 'PUT' => $USER_AUTH, 'DELETE' => $USER_AUTH);

$componentConfig = array(
	'album'							=> array('albums/album.php','Album', $USER_AUTH_ALL),
	'album_demo'					=> array('albumdemo/album.php', 'Albums', $USER_AUTH_ALL),
	'album_item'					=> array('albums/albumitem.php', 'AlbumItem', $USER_AUTH_ALL),
	'album_item_contents'			=> array('albums/albumitemcontents.php','AlbumItemContents', $USER_AUTH_ALL),
	'alert_configuration'			=> array('alerts/alert_configuration.php','Alert_configuration'),
	'alert_notify'					=> array('alerts/alert_notify.php','Alert_notify'),
	'alert_test_email'				=> array('alerts/alert_configuration_test.php','Alert_configuration_test'),
	'alerts'						=> array('alerts/alerts.php','Alerts'),
	'comp_codes'					=> array('compcodes/compcodes.php','CompCodes', $ADMIN_AUTH_LAN_ALL),
    //DO *NOT* SET ANY AUTHENTICATION FOR CONFIG, it internally blocks any request not from localhost
	'config'						=> array('config/config.php','Config'),
	//****
	'copy_local_server_share'		=> array('shares/copy_local_server_share.php','Copy_local_server_share'),
	'copy_status'					=> array('storage/copy_status.php','Copy_status'),
	'date_time_configuration'		=> array('device/date_time_configuration.php','Date_time_configuration'),
	'device'						=> array('device/device.php','Device', $USER_AUTH_ALL),
	'device_description'			=> array('device/device_description.php','Device_description'),
	'device_registration'			=> array('device/device_registration.php','Device_registration'),
	'device_user'					=> array('device/deviceuser.php','DeviceUser', $USER_AUTH_ALL),
	'device_user_notify'			=> array('device/deviceusernotify.php', $USER_AUTH_ALL),
	'dir'							=> array('dir/dir.php','Dir', $USER_AUTH_ALL),
	'dir_demo'						=> array('dirdemo/dirdemo.php', 'Dir', $USER_AUTH_ALL),
	'disk_status'					=> array('disk/disk_status.php','Disk_status'),
	'display_alert'					=> array('alerts/displayalert.php','DisplayAlert'),
	'drives'						=> array('drives/drives.php','Drives', $USER_AUTH_ALL),
	'error_codes'					=> array('errorcodes/errorcodes.php','ErrorCodes', $ADMIN_AUTH_LAN_ALL),
	'eula_acceptance'				=> array('device/eula_acceptance.php','Eula_acceptance'),
	'file'							=> array('file/file.php','File', $USER_AUTH_ALL),
	'file_contents'					=> array('filecontents/filecontents.php','FileContents', $USER_AUTH_ALL),
	'firmware_info'					=> array('firmware/firmware_info.php','Firmware_info'),
	'firmware_update'				=> array('firmware/firmware_update.php','Firmware_update'),
	'firmware_update_configuration'	=> array('firmware/firmware_update_configuration.php','Firmware_update_configuration'),
	'hdd_standby_time'				=> array('disk/hdd_standby_time.php','Hdd_standby_time'),
	'hidden_share_files'			=> array('shares/hidden_share_files.php','Hidden_share_files'),
	'image'							=> array('image/image.php','Image', $USER_AUTH_ALL),
	'itunes_configuration'			=> array('itunes/itunes_configuration.php','Itunes_configuration'),
	'itunes_scan'					=> array('itunes/itunes_scan.php','Itunes_scan'),
	'language_configuration'		=> array('device/language_configuration.php','Language_configuration'),
	'local_servers'					=> array('local_servers/local_servers.php','Local_servers'),
	'local_server_shares'			=> array('local_servers/local_server_shares.php','Local_server_shares'),
	'local_login'					=> array('login/locallogin.php','LocalLogin', array('GET' => $NO_AUTH)),
	'login'							=> array('login/login.php','Login', array('GET' => $NO_AUTH)),
	'logout'						=> array('logout/logout.php','Logout', array('GET' => $USER_AUTH)),
	'media_server_configuration'	=> array('media_server/media_server_configuration.php','Media_server_configuration'),
	'media_server_database'			=> array('media_server/media_server_database.php','Media_server_database', array('GET' => $USER_AUTH)),
	'media_server_connected_list'	=> array('media_server/media_server_connected_list.php','Media_server_connected_list'),
	'media_server_blocked_list'		=> array('media_server/media_server_blocked_list.php','Media_server_blocked_list'),
	'metadb_info'					=> array('metadbinfo/metadbinfo.php','MetaDbInfo', array('GET' => $USER_AUTH)),
	'metadb_summary'				=> array('metadbsummary/metadbsummary.php','MetaDbSummary', $USER_AUTH_ALL),
	'miocrawler_status'				=> array('miocrawlerstatus/miocrawlerstatus.php','MioCrawlerStatus', $USER_AUTH_ALL),
	'miodb'							=> array('miodb/miodb.php','MioDB', $USER_AUTH_ALL),
//	'mionet_configuration'			=> array('mionet/mionet_configuration.php','Mionet_configuration'),
	'mionet_state'					=> array('mionet/mionet_state.php','Mionet_state'),
//	'mionet_registration'			=> array('mionet/mionet_registration.php','Mionet_registration'),
	'mionet_registered'				=> array('mionet/mionet_registered.php','Mionet_registered'),
	'network_configuration'			=> array('network/network_configuration.php','Network_configuration'),
	'network_services_configuration'=> array('network/network_services_configuration.php','Network_services_configuration'),
	'network_workgroup'				=> array('network/network_workgroup.php','Network_workgroup'),
	'owner_configuration'			=> array('users/owner_configuration.php','Owner_configuration'),
	//DO *NOT* SET ANY AUTHENTICATION for port_test: it is called from the Server and there is no security risk to leaving it open
	'port_test'						=> array('porttest/porttest.php','PortTest'),
	//****
	'remote_account'				=> array('remoteuser/remoteaccount.php','RemoteAccount', $USER_AUTH_ALL),
	'shares'						=> array('shares/usersharecontrol.php','UserShareControl', $ADMIN_AUTH_LAN_MODIFY),
	'share_access'					=> array('shares/usershareaccess.php','UserShareAccess', $ADMIN_AUTH_LAN_MODIFY),
	'share_access_configuration'	=> array('shares/share_access_configuration.php','Share_access_configuration'),
	'shutdown'						=> array('system_configuration/shutdown.php','Shutdown'),
	'smart_test'					=> array('disk/smart_test.php','Smart_test'),
	'ssh_configuration'				=> array('network/ssh_configuration.php','Ssh_configuration'),
	'status'						=> array('status/status.php','Status'),
	'storage_usage'					=> array('storage/storage_usage.php','Storage_usage'),
	'storage_usage_by_share'		=> array('storage/storage_usage_by_share.php','Storage_usage_by_share'),
	'system_configuration'			=> array('system_configuration/system_configuration.php','System_configuration'),
	'system_factory_restore'		=> array('system_configuration/system_factory_restore.php','System_factory_restore'),
	'system_information'			=> array('system_reporting/system_information.php','System_information'),
	'system_log'					=> array('system_reporting/system_log.php','System_log'),
	'system_state'					=> array('system_reporting/system_state.php','System_state'),
	'time_zones'					=> array('device/time_zones.php','Time_zones'),
	'users'							=> array('users/users.php','Users', $USER_AUTH_GET_NO_AUTH_LAN),
	'verify_remote_user'			=> array('remoteuser/verifyremoteuser.php','VerifyRemoteUser', $USER_AUTH_ALL),
	'version'						=> array('version/version.php','Version', array('GET' => $NO_AUTH)),
	'volumes'						=> array('volumes/volumes.php','volumes', $USER_AUTH_ALL),
	'album_access'					=> array('albums/albumaccess.php', 'AlbumAccess', $USER_AUTH_ALL),
	'album_info'					=> array('albums/albuminfo.php', 'AlbumInfo', $USER_AUTH_ALL),
	'album_item_info'				=> array('albums/albumiteminfo.php', 'AlbumItemInfo', $USER_AUTH_ALL),
	'test_data'						=> array('file/test_data.php', 'TestData', $NO_AUTH),
	'vft_configuration'			    => array('system_configuration/vft_configuration.php','Vft_configuration'),
	'manual_port_forward'			=> array('manualportforward/manual_port_forward.php','ManualPortForward', $ADMIN_AUTH_LAN_MODIFY),
	'miocrawler'					=> array('miocrawler/miocrawler.php','MioCrawler', $ADMIN_AUTH_LAN_MODIFY),
	'dir_contents'					=> array('dircontents/dircontents.php','DirContents', $USER_AUTH_ALL),
	'update_counts'					=> array('update_counts/update_counts.php', 'Update_counts', $USER_AUTH_GET_NO_AUTH_LAN),
	'usb_drive'					=> array('usb_drive/usb_drive.php', 'Usb_drive', $USER_AUTH_GET_NO_AUTH_LAN),
	'usb_drives'					=> array('usb_drives/usb_drives.php', 'Usb_drives', $USER_AUTH_GET_NO_AUTH_LAN),
	'share_names'					=> array('shares/sharenames.php', 'ShareNames', array('GET' => $NO_AUTH_LAN)),
//Marker used to insert new entries above this line from components DO NOT REMOVE  Do terminate line above with comma.
	);

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>
