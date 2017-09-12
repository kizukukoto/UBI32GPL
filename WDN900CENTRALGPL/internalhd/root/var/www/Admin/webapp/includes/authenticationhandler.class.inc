<?php
/**
 * Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
 * 
 * Authentication Handler ( singleton )
 * @author Loknath Bharti
 * @since 2010-07-27
 * @version 0.1 
 * @package system
 */
class AuthenticationHandler {
	
	/**
	 * Class name
	 */
	var $name = "AuthenticationHandler";
	
	/**
	 * Instance variable
	 * @var AuthenticationHandler
	 */
	private static $instance;
	
	/** 
	 * @var SessionHandler
	 */
	
	private $sessionHandler;
		
	/**
	 * Number of authentication attempt threshold constant
	 * @var integer
	 */
	private $authThreshold = 3;
	
	/**
	 * 
	 * Blockable time offset constant in seconds
	 * @var integer
	 */
	private $authOffset = 300;
	
	/** 
	 * 
	 * Default constructor
	 * @access private
	 */
	private function __construct(){
		$this->sessionHandler = SessionHandler::init();
		if ( !$this->isAuthenticated() ) {
			$this->authenticate();
		}
	}
	
	/**
	 * 
	 * Initiates the singleton
	 * @static
	 */
	public static function init() {
		if ( !isset ( self::$instance ) ) {
			$c = __CLASS__;
			self::$instance = new $c;
		}
		return self::$instance;
	}
	
	/**
	 * 
	 * Verify throttle attempt and autheticate visitor  
	 * @return boolean
	 */
	function authenticate() {
		$remoteIp = ip2long($_SERVER['REMOTE_ADDR']);
		$count = 0;
		
		$db = new DBAccess();
		$sql = "select * from LoginAttempts where ip = :remote_ip";
		$bindVarNVTArray = array( array( ':remote_ip', $remoteIp, PDO::PARAM_INT ) );
		$cacheKey = 'LOGIN_ATTEMPTS';
		$rows = $db->executeQuery( $sql, $cacheKey, $bindVarNVTArray );
		$count = !empty($rows) ? $rows[0]['count'] : 0;

		if (!empty($rows)) print_r($rows);
		
		$blockablePeriod = $count ? $this->__getBlockablePeriod( $count ) : 0;
		$now = $this->sessionHandler->getAccessTime();
		$lastAttemptTime = !empty($rows) ? $rows[0]['time'] : 0;
		if ( $lastAttemptTime + $blockablePeriod <= $now ) {
			//authenticate
			if( $this->__authenticate() ) {
				$this->sessionHandler->write( 'authenticated', 1 );
				$sql = "delete from LoginAttempts where ip = :remote_ip";
				$bindVarNVTArray = array( array( ':remote_ip', $remoteIp, PDO::PARAM_INT ) );
				$cacheKey = 'LOGIN_ATTEMPT_DELETE';
				$rows = $db->executeQuery( $sql, $cacheKey, $bindVarNVTArray );
				
				return true;
			}
		};
		
		//increment fail count
		$count += 1;
		
		if ( $count > 1 ) {	
			$sql = "update LoginAttempts set count = :count, time = :time where id = :id";
			$bindVarNVTArray = array(
				array( ':id', $rows[0]['id'], PDO::PARAM_INT),
				array( ':count', $count, PDO::PARAM_INT),
				array( ':time', $now, PDO::PARAM_INT)
			);
			$cacheKey = 'UPDATE_LOGIN_ATTEMPTS';
			$db->executeUpdateWithPreparedStatements( $sql, $cacheKey, $bindVarNVTArray );
		}
		else {
			$sql = "insert into LoginAttempts (ip, count, time) values (:remote_ip, :count, :time)";
			$bindVarNVTArray = array(
				array( ':remote_ip', $remoteIp, PDO::PARAM_INT),
				array( ':count', $count, PDO::PARAM_INT),
				array( ':time', $now, PDO::PARAM_INT)
			);
			$cacheKey = 'INSERT_LOGIN_ATTEMPTS';
			$db->executeInsert( $sql, $cacheKey, $bindVarNVTArray );
		}
		return false;
	}
	
	/**
	 * 
	 * Test if user is local or router and authenticate with local database 
	 * @return boolean
	 */
	private function __authenticate() {
		if ($this->__isRouter()) {
			return false;
		}
		
		if ($this->__isLocal()) {
			return true;
		}
		require_once('security.inc');
		$deviceUserId = !empty( $_GET['dui'] ) ? $_GET['dui'] : 1;
		$auth = !empty( $_GET['auth'] ) ? $_GET['auth'] : 'asdf';
		//return true;
		return authenticateCentralUser( $deviceUserId, $auth );
	}
	
	/**
	 * 
	 * Test if the visitor is the router itself
	 * to ascertain the visitor is not using the router
	 * to establish itself as a local user
	 * @return boolean
	 */
	private function __isRouter() {
		exec( "sudo route|grep default|cut -d: -f2|awk '{print $2}'", $routerIp );
		$routerIp = ip2long(trim($routerIp[0]));
        $remoteIp = ip2long($_SERVER['REMOTE_ADDR']);
        if ( $routerIp === $remoteIp ) {
            return true;
        }
        return false;
	}
	
	/**
	 * 
	 * Calculates blockable time period with the given number of visit count
	 * to a maximum of twenty four hours
	 * @param $count integer
	 * @return integer
	 */
	private function __getBlockablePeriod($count) {
		$daySeconds = 24 * 3600;
		$blockableSeconds = floor( $count / $this->authThreshold ) * $this->authOffset;
		return $blockableSeconds < $daySeconds ? $blockableSeconds : $daySeconds;
	}
	 
	/**
	 * 
	 * Return if the visitor is authenticated
	 * @return boolean
	 */
	function isAuthenticated() {
		return true;
		return $this->sessionHandler->read('authenticated');
	}
	
	/**
	 * 
	 * Set the visitor as authenticated
	 */
	private function __setAuthenticated() {
		$this->sessionHandler->write( 'authenticated', 1 );
	}
	
	/**
	 *  
	 * Test whether the visitor is local
	 * @return boolean
	 */
	private function __isLocal() {
		$remoteIp = ip2long($_SERVER['REMOTE_ADDR']);
		$ipRange[] = array(
			'begin' => ip2long("127.0.0.0"),
			'end' => ip2long("127.255.255.255")
		);
		$ipRange[] = array(
			'begin' => ip2long("10.0.0.0"),
			'end' => ip2long("10.255.255.255")
		);
		$ipRange[] = array(
			'begin' => ip2long("172.16.0.0"),
			'end' => ip2long("172.31.255.255")
		);
		$ipRange[] = array(
			'begin' => ip2long("192.168.0.0"),
			'end' => ip2long("192.168.255.255")
		);
		
		foreach($ipRange as $range) {
			if ( ($range['begin'] <= $remoteIp) && ($remoteIp <= $range['end'] ) ) {
				return true;
			}
		}
		return false;
	}
}
?>