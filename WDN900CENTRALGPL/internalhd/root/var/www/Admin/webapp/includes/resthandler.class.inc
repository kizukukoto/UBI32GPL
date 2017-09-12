<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
class RestHandler {
	
	var $name = "RestHandler";

	private $server;
	private $resourcePath;
	private $curlOptions = array();
	private $protocol = "http";
	
	public function __construct ( $args = array() ) {
		$this->curlOptions[ CURLOPT_RETURNTRANSFER ] = TRUE;		
		if ( !empty( $args['server'] ) ) {
			$this->server = $args['server'];
		}
		else {
			$global = getGlobalConfig( "global" );
			$this->server = $global['HOSTNAME'];
		}
		
		if (!empty ( $args['resourcePath'] ) ) {
			$this->resourcePath = $args['resourcePath'];
		}
		else {
			$this->resourcePath = "";
		}
		
		if (!empty ($args['curlOptions'])) {
			$this->curlOptions = $args['curlOptions'];
		}
		
		if (!empty ($args['protocol'])) {
			$this->protocol = $args['protocol'];
		}
	}
	
	public function post ( $args = array() ) {
		if ( !empty ( $args['resourcePath'] ) ) {
			$this->resourcePath = $args['resourcePath'];
		}
		$curlClient = $this->__getCurl();
		curl_setopt ( $curlClient, CURLOPT_POST, TRUE );
		if ( !empty ( $args['data'] ) ) {
			curl_setopt( $curlClient, CURLOPT_POSTFIELDS, getUrlString( $args['data'] ) );
		}
		
		if ( $output = curl_exec( $curlClient ) ) {
			return $output;
		}
		else {
			echo 'curl error:' . curl_error($curlClient);
		}
		return null;
	}
	
	private function __getCurl () {
		$curl = curl_init();
		$curlOptions = $this->curlOptions;
		$curlOptions[CURLOPT_URL] = $this->protocol . "://" . $this->server . $this->resourcePath;
		curl_setopt_array( $curl, $curlOptions );
		return $curl;
	}
}
