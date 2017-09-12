<?php

/**
 * Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.
 * Certificate Handler (Singleton)
 * @author Loknath Bharti
 * @since 2010-08-11
 * @version 0.1
 * @package system
 */
class CertificateHandler {
	
	/*
	 * Class Name
	 */
	var $name = 'CertificateHandler';

	/*
	 * Static instance variable
	 */
	private static $instance;
	
	/*
	 * SSL Configuration Variable to store global configuration
	 */
	private $sslConf;
	
	/*
	 * SSL Ready variable
	 */
	private static $sslReady = false;

	/**
	 * 
	 * Constructor
	 * Checks if SSL is enabled in global config
	 * and key and certificate are ready
	 * @access private
	 */
	private function __construct() {
		$this->sslConf = getGlobalConfig("openssl");
		if ( $this->sslConf['SSL_ENABLED'] ) {
			if ( !$this->__checkCert() || TRUE ) {
				if ( $this->__init() ) {
					self::$sslReady = true;
				}
			}
			else {
				self::$sslReady = true;
			}
			$this->__setApacheSsl( "enable" );
		}
		else {
			$this->__setApacheSsl( "disable" );
		}
	}
	
	/**
	 * 
	 * Initiates the singleton
	 * @static
	 */
	public static function init() {
		if ( !isset( self::$instance ) ) {
			$c = __CLASS__;
			self::$instance = new $c;
		}
		return self::$instance;
	}
	
	/**
	 * 
	 * Generates key and gets signed certificate from central server
	 * and stores them at the location provided by global config
	 * @return boolean
	 */
	private function __init() {
		
		$opensslConf = array(
			'config' => $this->sslConf['OPENSSL_CONF_PATH'],
			'encrypt_key' => false,
			'private_key_type' => OPENSSL_KEYTYPE_RSA,
			'digest_alg' => 'sha1',
			'private_key_bits' => 1024, 
		);
		
		if ($res = openssl_pkey_new($opensslConf)) {
			openssl_pkey_export_to_file(
				$res, 
				$this->sslConf['CERT_PATH'].'server.key', 
				NULL, 
				$opensslConf
			);
			
			$dn = array(
				"countryName" => 'US', 
				"stateOrProvinceName" => 'California', 
				"localityName" => 'Mountain View', 
				"organizationName" => 'Western Digital', 
				"organizationalUnitName" => 'Test Device', 
				"commonName" => 'LokeTestDevice', 
				"emailAddress" => 'loke@localhost.com'
			);
			
			$csr_res = openssl_csr_new( $dn, $res, $opensslConf );
			openssl_csr_export( $csr_res, $csr_str );
			
			openssl_csr_export_to_file(
				$csr_res,
				$this->sslConf['CERT_PATH'].'server.csr'
			);
			
			if ( $signed_cert = $this->__getSignedCert( $csr_str ) ) {
				$cert_file = fopen( $this->sslConf['CERT_PATH'].'server.crt', 'w' );
				fwrite($cert_file, $signed_cert);
				fclose($cert_file);
				if ( $this->__setApacheSsl( "enable" ) ) {
					return true;
				}
			}
		}
		return false;
	}
	
	/**
	 * 
	 * Returns SSL status
	 * @return boolean 
	 */
	function isSslReady() {
		return self::$sslReady;
	}
	
	/**
	 * 
	 * Checks the local system for key and certificate
	 * @return boolean
	 */
	private function __checkCert() {
		if ( 
			file_exists( $this->sslConf['CERT_PATH'].'server.key' )
			&& file_exists( $this->sslConf['CERT_PATH'].'server.crt' ) 
			) {
				return true;
			}
			return false;
	}
	
	/**
	 * 
	 * Connects to central server to retrieve signed certificate
	 * @param string csr 
	 * @return string
	 */
	private function __getSignedCert( $csr_str ) {
		$deviceConfig = getGlobalConfig('device');
		$resourcePath = $deviceConfig['DEVICE_REST_PATH']. PS . 'devicecertificate' ;
		$deviceId = getDeviceId();
		$deviceAuth = getDeviceAuthode(); 
		$restHandler = new RestHandler(
			array( 
				'resourcePath' => $resourcePath
			)
		);
		
		$response = $restHandler->post(
			array(
				'data' => array (
					'device_id' => $deviceId,
					'csr' => $csr_str,
					'auth' => $deviceAuth,
					'format' => 'xml'
				)
			)
		);
		
		$xmlReturn = new SimpleXMLElement( $response );
		if ( $xmlReturn->status ) {
			return $xmlReturn->signed_cert;
		}
		return null;	
	}
	
	/**
	 * Enables or disables SSL for Apache
	 * @param action enable/disable
	 * @return boolean
	 * @todo mikeC: test DEVICETYPE (globalconstants.inc) to determine correct path
	 */
	private function __setApacheSsl( $action = null ) {

// maybe there should be an 'OS_TYPE' define...		
		$origPath = APACHE_CONF_PATH. DS . 'extra' . DS . 'httpd-ssl.conf';
		$destPath = APACHE_CONF_PATH. DS . 'extra' . DS . 'auto' . DS . 'httpd-ssl.conf';
//		$origPath = APACHE_CONF_PATH. DS . 'mods-available' . DS . 'ssl.conf';
//		$destPath = APACHE_CONF_PATH. DS . 'mods-enabled' . DS . 'ssl.conf';


		$status = false;
		switch ( $action ) {
			case "enable":
				if (copy ( $origPath, $destPath ) ) {
					$status = true;
				}
				break;
				
			case "disable":
				if (file_exists ($destPath)) {
					if ( unlink ( $destPath ) ) {
						$status = true;
					}
				}
				else {
					$status = true; //if disabled, file doesn't have to exist
				}
				break;
			default: 
				return $status;
		}
		if ( $status ) {
			/**
			 * Following solution is not good, since once the service stops
			 * PHP cannot start the service.
			 * A cron job would be an appropriate solution on Linux.
			 * For now it would be better to just start the service oneself.
			 */
			/* Following code segment requires win32service extension which
			 * is not a standard part of windows PHP installation 
			 * thus disabled.
			 */ 
			/*if ( win32_stop_service('apache2.2') ) {
				win32_start_service('apache2.2');	
			}*/
			/*exec ("sc stop apache2.2");
			exec ("sc start apache2.2");*/
		}
		return $status;
	}
}
?>