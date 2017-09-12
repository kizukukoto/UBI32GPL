<?php

require_once('dbaccess.inc');

class ShareAccessDB extends DBAccess {

		private static $queries = array (
		'GET_ACL_FOR_SHARE' => "SELECT share_name, user_id, access_level, created_date FROM UserShareAccess WHERE share_name=:share_name",
		'GET_ACL_FOR_SHARE_AND_USER' => "SELECT share_name, user_id, access_level, created_date FROM UserShareAccess WHERE share_name=:share_name AND user_id=:user_id",
		'GET_SHARES_FOR_USER' => " SELECT distinct s.share_name, 
									    CASE  
									       WHEN s.public_access = 'true' then 'RW' 
									       ELSE sa.access_level 
									    END access_level, 
									    s.created_date
									FROM UserShares s
									left outer join UserShareAccess sa
									 ON s.share_name = sa.share_name
									WHERE public_access = 'true'
									 or sa.user_id = :user_id",
		'INSERT_ACL_FOR_SHARE' => "INSERT INTO UserShareAccess (share_name, user_id, access_level, created_date) VALUES (:share_name, :user_id, :access_level, DATETIME('NOW'))",
		'UPDATE_ACCESS_FOR_USER_FOR_SHARE' => "UPDATE UserShareAccess SET access_level=:access_level WHERE user_id=:user_id AND share_name=:share_name",
		'DELETE_ACCESS_FOR_USER_FOR_SHARE' => "DELETE FROM UserShareAccess WHERE user_id=:user_id AND share_name=:share_name",
		'DELETE_ALL_ACCESS_FOR_SHARE' => "DELETE FROM UserShareAccess WHERE share_name=:share_name",
		'DELETE_ALL_ACCESS_FOR_USER' => "DELETE FROM UserShareAccess WHERE user_id=:user_id"
	);

	public function __construct() {
		require_once('dbaccess.inc');
	}

   /**
	 * getAclForShare
	 *
	 * Returns an array of assoc. arrays for a given share. Each array contains user_id and access_level for user.
	 * Format: [[user_id, access_level]]
	 *
	 * @param $shareName - name of share
	 * @param $userId - id of user (optional)
	 */
	public function getAclForShare($shareName,$userId=NULL) {
		if (isset($userId)) {
			$queryName = 'GET_ACL_FOR_SHARE_AND_USER';
			$bindVarNVTArray = array( array(':share_name', $shareName, PDO::PARAM_STR),
									 array(':user_id', $userId, PDO::PARAM_STR));
		}
		else {
			$queryName = 'GET_ACL_FOR_SHARE';
			$bindVarNVTArray = array( array(':share_name', $shareName, PDO::PARAM_STR));
		}
		return $this->executeQuery(self::$queries[$queryName], $queryName, $bindVarNVTArray);
	}


   /**
	 * getSharesForUser
	 *
	 * Returns an array of assoc. arrays for all of the shares a given user has access to.
	 * Each array contains share_name and access_level for each share.
	 * Format: [[share_name, access_level]]
	 *
	 * @param $userId - ID of user
	 */
	public function getSharesForUser($userId) {
		$bindVarNVTArray = array( array(':user_id', $userId, PDO::PARAM_INT));
		return $this->executeQuery(self::$queries['GET_SHARES_FOR_USER'], 'GET_SHARES_FOR_USER', $bindVarNVTArray);
	}

	/**
	 * addAclForShare
	 *
	 * Adds acl for a given user for a share
	 *
	 * @param $shareName - name of share
	 * @param $userId - ID of user
	 * @param $accessLevel 'RO' (read-only), or: 'RW' (read-write)
	 */
	public function addAclForShare($shareName, $userId, $accessLevel){
		$bindVarNVTArray = array(   array(':share_name', $shareName, PDO::PARAM_STR),
									array(':user_id', $userId, PDO::PARAM_STR),
									array(':access_level', $accessLevel, PDO::PARAM_STR),
								 );
		return $this->executeQuery(self::$queries['INSERT_ACL_FOR_SHARE'], 'INSERT_ACL_FOR_SHARE', $bindVarNVTArray);
    }

    /**
     * updateAclForShare
     *
     * Updates acl for a given user for a share
     *
	 * @param $shareName - name of share
	 * @param $userId - ID of user
	 * @param $accessLevel 'RO' (read-only), or: 'RW' (read-write)
     */

    public function updateAclForShare($shareName, $userId, $accessLevel) {
		$bindVarNVTArray = array(   array(':share_name', $shareName, PDO::PARAM_STR),
									array(':user_id', $userId, PDO::PARAM_STR),
									array(':access_level', $accessLevel, PDO::PARAM_STR),
								 );
		return $this->executeUpdateWithPreparedStatements(self::$queries['UPDATE_ACCESS_FOR_USER_FOR_SHARE'],
														  'UPDATE_ACCESS_FOR_USER_FOR_SHARE', $bindVarNVTArray);
    }

    /**
     * deleteUserAccessForShare
     *
     * Deletes user ACL row for given share
	 *
	 * @param $shareName name of Samba share
	 * @param $userId ID of user
     */

	public function deleteUserAccessForShare( $shareName, $userId) {

		$shareName = getSafeDatabaseText((string)$shareName);
		$bindVarNVTArray = array(  array(':user_id', (int)$userId, PDO::PARAM_INT),
								   array(':share_name', $shareName, PDO::PARAM_STR)
		);

		return $this->executeDelete(self::$queries['DELETE_ACCESS_FOR_USER_FOR_SHARE'], 'DELETE_ACCESS_FOR_USER_FOR_SHARE', $bindVarNVTArray);
    }

    /**
     * deleteAllAccessForShare
     *
     * Deletes all ACL rows for a given share
	 *
	 * @param $shareName name of Samba share
     */

	public function deleteAllAccessForShare($shareName) {
		$bindVarNVTArray = array( array(':share_name', getSafeDatabaseText((string)$shareName), PDO::PARAM_STR));
		return $this->executeDelete(self::$queries['DELETE_ALL_ACCESS_FOR_SHARE'], 'DELETE_ALL_ACCESS_FOR_SHARE', $bindVarNVTArray);
    }

    /**
     * deleteAllAccesForUser()
     *
     * Deletes all ACL rows for a given user
     *
	 * @param $userId ID of user
     */

    public function deleteAllAccessForUser($userId) {
		$bindVarNVTArray = array( array(':user_id',(int)$userId, PDO::PARAM_INT) );
		return $this->executeDelete(self::$queries['DELETE_ALL_ACCESS_FOR_USER'], 'DELETE_ALL_ACCESS_FOR_USER', $bindVarNVTArray);
    }

}

/** TEST CODE **/

/*
$shareAccessdb = new ShareAccessDB();

$shareAccessdb->addAclForShare("burt",1,"RW");
$shareAccessdb->addAclForShare("burt",2,"RO");
$shareAccessdb->addAclForShare("burt",3,"RO");
$shareAccessdb->addAclForShare("ralph",2,"RW");
$shareAccessdb->addAclForShare("huey",3,"RW");


$acl = $shareAccessdb->getAclForShare("burt");
var_dump($acl);

$shareAccessdb->updateAclForShare("burt",2,"RW");
$shareAccessdb->updateAclForShare("burt",1,"RO");

$acl = $shareAccessdb->getAclForShare("burt");
var_dump($acl);



$shareAccessdb->deleteUserAccessForShare("burt",2);
$shareAccessdb->deleteUserAccessForShare("burt",3);

$acl = $shareAccessdb->getAclForShare("burt");
var_dump($acl);


$shareAccessdb->deleteAllAccessForUser(3);

$acl = $shareAccessdb->getAclForShare("burt");
var_dump($acl);
$acl = $shareAccessdb->getAclForShare("huey");
var_dump($acl);

$shareAccessdb->deleteAllAccessForShare("burt");

$acl = $shareAccessdb->getAclForShare("burt");
var_dump($acl);
*/



?>