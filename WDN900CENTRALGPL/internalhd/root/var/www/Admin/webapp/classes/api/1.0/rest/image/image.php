<?php
/**
 * Class Image
 *
 * PHP version 5
 *
 * @category  Classes
 * @package   Orion
 * @author    WDMV Software Engineering
 * @copyright 2011 Western Digital Technologies, Inc. - All rights reserved
 * @license   http://support.wdc.com/download/netcenter/gpl.txt GNU License
 * @link      http://www.wdc.com/
 *
 */
require_once('restcomponent.inc');


/**
 * Image Class extends the Rest Component and uses the GD extension.
 *
 * @version Release: @package_version@
 *
 */
class Image extends RestComponent {

	public function __construct() {
		require_once('globalconfig.inc');
		require_once('image.inc');
		require_once('outputwriter.inc');
		require_once('security.inc');
		require_once('util.inc');
	}

	/**
	 * Rotate image by specified degrees
	 *
	 * @param array $args
	 * @param array $params
	 * @param string $format
	 *
	 * @return array $results
	 */
	public function put($args, $params=null, $format='xml') {
		$rotate  = isset($params['rotate'])  ? strtolower(trim($params['rotate']))  : '';
		$width   = isset($params['width'])   ? strtolower(trim($params['width']))   : '';
		$height  = isset($params['height'])  ? strtolower(trim($params['height']))  : '';

		$shares_path = getSharePath();
		$share_name  = isset($args[0]) ? $args[0] : '';
		$paths       = array_slice($args, 1);
		$file        = implode('/', $paths);
		$path        = $shares_path . '/' . $share_name . '/' . $file;

		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'shares_path', $shares_path);
		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'share_name', $share_name);
		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'paths', print_r($paths, true));
		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'file', $file);
		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'path', $path);
		//printf("<PRE>%s.%s = [%s]</PRE>\n", __METHOD__, 'params', print_r($params, true));

		if (empty($share_name)) {
			$this->generateErrorOutput(400, 'image', 'SHARE_NAME_MISSING', $format);
			return;
		}

		if (empty($file)) {
			$this->generateErrorOutput(400, 'image', 'FILE_MISSING', $format);
			return;
		}

		if (!file_exists($path)) {
			$this->generateErrorOutput(400, 'image', 'FILE_NOT_FOUND', $format);
			return;
		}

		$image = new ImageModel();

		$status = $image->installed();

		if (!$status) {
			$this->generateErrorOutput(500, 'image', 'IMAGE_LIB_NOT_INSTALLED', $format);
			return;
		}

		$img = $image->create($path);

		if (!$img) {
			$this->generateErrorOutput(500, 'image', 'IMAGE_CREATE_FAILED', $format);
			return;
		}

		$img = $image->rotate($img, $rotate);

		if (!$img) {
			$this->generateErrorOutput(500, 'image', 'IMAGE_ROTATION_FAILED', $format);
			return;
		}

		$status = $image->save($img, $path);

		if (!$status) {
			$this->generateErrorOutput(500, 'image', 'IMAGE_SAVE_FAILED', $format);
			return;
		}

		$results = array('status' => 'success');
		$this->generateSuccessOutput(200, 'image', $results, $format);
	}
}

/*
HOW TO INSTALL GD EXTENSION

1. Download the packages below from:
   http://packages.debian.org/squeeze/powerpc/php5-gd/download

2. Execute following commands in order (ignore any errors):
   dpkg -i libxpm4_3.5.8-1_powerpc.deb
   dpkg -i libt1-5_5.1.2-3_powerpc.deb
   dpkg -i libjpeg62_6b1-1_powerpc.deb
   dpkg -i ttf-dejavu-core_2.31-1_all.deb
   dpkg -i ttf-bitstream-vera_1.10-8_all.deb
   dpkg -i ttf-freefont_20090104-7_all.deb
   dpkg -i gsfonts_8.11+urwcyr1.0.7~pre44-4.2_all.deb
   dpkg -i fontconfig-config_2.8.0-2.1_all.deb
   dpkg -i libfontconfig1_2.8.0-2.1_powerpc.deb
   dpkg -i gsfonts-x11_0.21_all.deb
   dpkg -i libgd2-xpm_2.0.36~rc1~dfsg-5_powerpc.deb
   dpkg --configure -a
   dpkg -i php5-gd_5.2.6.dfsg.1-1+lenny9_powerpc.deb
   dpkg -l | grep -i php
   /etc/init.d/apache2 restart

3. Use phpinfo to check for GD extension
*/
?>