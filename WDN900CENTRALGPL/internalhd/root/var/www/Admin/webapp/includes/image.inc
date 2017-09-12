<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

/**
 * Create image from file
 *
 * @param string $file
 * @return resource $img
 */
function createImage($file) {
	$img = imagecreatefromjpeg($file);
	return $img;
}


/**
 * Rotate image by direction and degrees
 *
 * @param resource $img
 * @param integer $rotate  - ['cw','ccw',-270,-180,-90,0,90,180,270]
 * @return resource $new_img
 */
function rotateImage($img, $rotate) {
	switch ($rotate) {
		case 'cw':
		$img = rotateImageCCW($img, 270);
		break;

		case 'ccw':
		$img = rotateImageCCW($img, 90);
		break;

		default:
		if ($rotate > 0) {
			$rotate = 360 - $rotate;
			$img = rotateImageCCW($img, abs($rotate));
		} else {
			$img = rotateImageCCW($img, abs($rotate));
		}
		break;
	}
	return $img;
}


/**
 * Rotate image counter-clockwise by degrees
 *
 * @param resource $img
 * @param integer $degrees
 * @return resource $new_img
 */
function rotateImageCCW($img, $degrees) {
	switch ($degrees) {
		case  90: $n = 1; break;
		case 180: $n = 2; break;
		case 270: $n = 3; break;
		case   0: return $img; break;
		case 360: return $img; break;
		default: return $img; break;
	}
	for ($i=0; $i < $n; $i++) {
		$width   = imagesx($img);
		$height  = imagesy($img);
		$new_img = imagecreatetruecolor($height, $width);
		imagealphablending($new_img, false);
		imagesavealpha($new_img, true);
		for ($w=0; $w < $width; $w++) {
			for ($h=0; $h < $height; $h++) {
				$ref = imagecolorat($img, $w, $h);
				imagesetpixel($new_img, $h, ($width-1)-$w, $ref);
			}
		}
		$img = $new_img;
	}
	return $img;
}


/**
 * Save image to file
 *
 * @param resource $img
 * @param string $file
 * @return integer $status
 */
function saveImage($img, $file) {
	$status = imagejpeg($img, $file);
	return $status;
}


/**
 * Rotate image clockwise by degrees - DO NOT USE!
 * PROBLEM: Causes line on left edge when $degrees == 180
 * @param resource $img
 * @param integer $degrees
 * @return resource $new_img
 */
function rotateImageCW_000($img, $degrees) {
	$width = imagesx($img);
	$height = imagesy($img);
	switch ($degrees) {
		case  90: $new_img = @imagecreatetruecolor($height, $width ); break;
		case 180: $new_img = @imagecreatetruecolor($width , $height); break;
		case 270: $new_img = @imagecreatetruecolor($height, $width ); break;
		case   0: return $img; break;
		case 360: return $img; break;
	}
	if ($new_img) {
		for ($i = 0;$i < $width ; $i++) {
			for ($j = 0;$j < $height ; $j++) {
				$reference = imagecolorat($img,$i,$j);
				switch ($degrees) {
					case  90: if(!@imagesetpixel($new_img, ($height - 1) - $j, $i, $reference)){return false;}break;
					case 180: if(!@imagesetpixel($new_img, $width - $i, ($height - 1) - $j, $reference)){return false;}break;
					case 270: if(!@imagesetpixel($new_img, $j, $width - $i, $reference)){return false;}break;
				}
			}
		} return $new_img;
	}
	return false;
}
?>