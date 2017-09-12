<?php 
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

class SslCertificate{
	
	private $sslConf;
	
	public function __construct() {
		$this->sslConf = getGlobalConfig("openssl");
	}
	
	private function generateKeyAndCsrFiles($deviceId, $serverDomain) {
		
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
				"organizationalUnitName" => 'Branded Products', 
				"commonName" => "*.device$deviceId.$serverDomain", 
				"emailAddress" => 'admin@localhost.com'
			);
			
			$csr_res = openssl_csr_new( $dn, $res, $opensslConf );
			openssl_csr_export( $csr_res, $csr_str );
			
			openssl_csr_export_to_file(
				$csr_res,
				$this->sslConf['CERT_PATH'].'server.csr'
			);
			
			return $csr_str;
		}
		return null;
	}
	
	public function getSignedCert($serverDomain, $deviceId, $deviceAuth){
		$csr_str = $this->generateKeyAndCsrFiles($deviceId, $serverDomain);
		if(!empty($csr_str)){
			$deviceConfig = getGlobalConfig('device');
			$globalConfig = getGlobalConfig("global");
			$url = getServerBaseUrl().$deviceConfig['DEVICECERTIFICATE_REST_PATH'];
			
			$httpClient = new HttpClient();
			
			$response = $httpClient->post(
				array(
					'requestUrl' => $url,
					'data' => array (
						'device_id' => $deviceId,
						'csr' => $csr_str,
						'auth' => $deviceAuth,
						'format' => 'xml'
					)
				)
			);
			if($response['status_code'] == 200){
			$xmlReturn = simplexml_load_string( $response['response_text'] );
			if ( $xmlReturn->status ) {
				return $xmlReturn->signed_cert;
			}
			}
			return null;	
		}
	}
}

?>