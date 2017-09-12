<?php
//require_once('drivesdb.php');

class DriveInfo {

	public function __construct()
	{
	}


	/**
	 * Select drive information.
	 *
	 * @param string $serialno
	 * @return array $drives
	 */
	public function select($serialno=null)
	{
		$drives = array(
			array(
				'serialno'     => '123456789',
				'vendor'       => 'WDC',
				'model'        => 'Scorpio Blue 1TB - WD10TPVT',
				'is_encrypted' => 'false',
			),
			array(
				'serialno'     => '987654321',
				'vendor'       => 'Seagate',
				'model'        => 'Barracuda 1TB - ST31000520AS',
				'is_encrypted' => 'true',
			),
		);

		if (!empty($serialno)) {
			foreach ($drives as $drive) {
				if ($drive['serialno'] == $serialno) {
					return $drive;
				}
			}
			return false;
		}
		if (empty($drives)) {
			return false;
		}
		return $drives;
	}


	/**
	 * Update drive information.
	 *
	 * @param string $serialno
	 * @param string $password
	 * @param boolean $eject
	 * @return integer $status
	 */
	public function update($serialno, $password=null, $eject=null)
	{
		$status = true;

		### MAKE LOW-LEVEL CALL TO UPDATE DRIVE STATUS
		//$status = updateDriveStatus($serialno, $password, $eject);

		return $status;
	}
}
?>