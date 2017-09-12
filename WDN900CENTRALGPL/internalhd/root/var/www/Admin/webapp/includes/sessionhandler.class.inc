<?php
/**
 * Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
 * Session Handler ( singleton )
 * @author Loknath Bharti
 * @since 2010-07-23
 * @version 0.1 
 * @package system
 * @property singleton
 */
class SessionHandler {
	
	/**
	 * Class name
	 */	
	var $name = "SessionHandler";
	
	/**
	 * Instance variable
	 */
	private static $instance;
	
	/**
	 * Session timeout in seconds
	 */
	private static $timeout = 30000;
	
	/**
	 * Default constructor
	 * @param integer $timeout
	 */
	private function __construct( $timeout = null ) {
		//start a session 
		session_start();
		if ( $timeout ) {
			$this->setTimeout( $timeout );
		}
		
		if ( !$this->isSessionValid() ) {
			$this->end();
			session_start();
			$timeout = $timeout ? $timeout : self::$timeout;
			$this->setTimeout( $timeout );
		}
		
		$this->__setAccessTime();
	}
	
	/**
	 * Initiate the singleton
	 * @param integer $timeout
	 */
	public static function init( $timeout = null ) {
		if ( !isset ( self::$instance ) ) {
			$c = __CLASS__;
			self::$instance = new $c( $timeout );
		}
		else if ( $timeout ){
			self::$instance->setTimeout( $timeout );
		}
		return self::$instance;
	}
	
	/**
	 * Check if session is valid
	 * @return boolean
	 */
	function isSessionValid() {
		$now = time();
		$lastAccessTime = $this->getAccessTime();
		if ( $now - $lastAccessTime > $this->getTimeout() ) {
			return false;
		}
		return true;
	}
	
	/**
	 * Set timeout limit in seconds for the session
	 * @param integer $timeout
	 */
	function setTimeout($timeout) {
		$this->write('timeout', $timeout);
	}
	
	/** 
	 * Get timeout limit in seconds of the session
	 * @return integer
	 */
	function getTimeout() {
		return $this->read('timeout');
	}
	
	/**
	 * Write to the session
	 * @param string $variableName
	 * @param mixed $value
	 */
	function write( $variableName, $value ) {
		$_SESSION[$variableName] = $value;
	}
	
	/**
	 * Read from the session
	 * @param string $variableName
	 * @return mixed
	 */
	function read( $variableName ) {
		if ( !empty ( $_SESSION[$variableName] ) ) {
			return $_SESSION[$variableName];
		}
		return null;
	}
	
	/**
	 * Destroy the session
	 */
	function end() {
		session_destroy();
		session_commit();
	}
	
	/**
	 * Return last access time stored in session
	 * @return integer
	 */
	function getAccessTime() {
		if( !empty( $_SESSION['last_access_time'] )) {
			return $_SESSION['last_access_time'];
		}
		else return null;
	}
	
	/** 
	 * Set last access time to now
	 */
	private function __setAccessTime() {
		$_SESSION['last_access_time'] = time();
	}
	
}
?>
