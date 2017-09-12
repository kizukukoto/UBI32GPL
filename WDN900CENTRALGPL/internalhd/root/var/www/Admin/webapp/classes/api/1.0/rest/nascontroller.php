<?php
require_once('logMessages.php');
require_once('util.inc');


class NasController{

	private static $componentConfig = array();
	private static $NO_AUTH=1;
	private static $USER_AUTH=2;
	private static $ADMIN_AUTH=3;
	private static $NO_AUTH_LAN=4;
	private static $USER_AUTH_LAN=5;
	private static $ADMIN_AUTH_LAN=6;

	function __construct() {
		require_once('component_config.php');
		self::$componentConfig = $componentConfig;
		self::$NO_AUTH = $NO_AUTH;
		self::$USER_AUTH = $USER_AUTH;
		self::$ADMIN_AUTH = $ADMIN_AUTH;
		self::$NO_AUTH_LAN = $NO_AUTH_LAN;
		self::$USER_AUTH_LAN = $USER_AUTH_LAN;
		self::$ADMIN_AUTH_LAN = $ADMIN_AUTH_LAN;

	}

	function createComponent($comp_name){
		$comp = null;
		try{
			if(!array_key_exists($comp_name, self::$componentConfig)){
				return NULL;
			}
			$compInfo = self::$componentConfig[$comp_name];
			if(is_null($compInfo)){
				return NULL;
			}
			// Add prefix to component path. version number and standard.
			require("1.0/rest/".$compInfo[0]);
			$class = new ReflectionClass($compInfo[1]);
			if ($class->isInstantiable()) {
				$comp = $class->newInstance();
			}
		}catch(Exception $e){
			$comp = null;
		}
		return $comp;
	}

	/**
	 * Rertruns true if component requires authetication. Otherwise false.
	 * @param $compName
	 */
	public function isAuthenticationRequired($compName)
	{
		$compInfo = self::$componentConfig[$compName];
		return $compInfo[2];
	}

	/**
	 * Rertruns true if component requires authetication. Otherwise false.
	 * @param $compName
	 */
	public function getSecurityTypeForMethod($compName, $methodName)
	{
		$compInfo = self::$componentConfig[$compName];
		// Check if the security is defined for component.
		// If security is not defined for component return NO_AUTH.
		// TODO: Remove "NO_AUTH" after all components defined security.
		if(isset($compInfo[2]) && $compInfo[2][$methodName]){
			return $compInfo[2][$methodName];
		}else{
			return self::$NO_AUTH;
		}
	}

	function exec() {


		// Parse query string
		//header("Cache-Control: no-cache, must-revalidate");

		// Check whether remote access enabled
		$remoteAccess = getRemoteAccess();
		if(strcasecmp($remoteAccess,"FALSE") == 0 && !isLanRequest()){
			setHttpStatusCode(401, "Request is not allowed.");
			return;
		}

		parse_str( $_SERVER['QUERY_STRING'] , $queryParams);
		$outputFormat = 'xml';
		if(isset($queryParams['format'])){
			$outputFormat = $queryParams['format'];
		}

		$logObj = new LogMessages();

		// remove format from query_params.
		unset($queryParams['format']);

		// Get HTTP method name
		$methodName = strtoupper($_SERVER['REQUEST_METHOD']);

		// Get component name. Always 4th in the url /api/1.0/rest/<component_name>
		$urlParts = explode('/', trim($_SERVER['SCRIPT_URL']));

		// Do a error check.
		if(sizeof($urlParts) <= 3 || strcmp ( $urlParts[4], "") == 0) {
			header("HTTP/1.0 404 Not Found");
			return;
		}
		$compName = $urlParts[4];

		$urlPath = array();
		if(count($urlParts) > 5){
			$urlPath = array_slice($urlParts, 5);
		}

		$compObject = $this->createComponent($compName);
		if($compObject == NULL){
			$logObj->LogData('ERROR', get_class($compObject),  $methodName,  'Not Found');
			header("HTTP/1.0 404 Not Found");
			return;
		}

		// Check EULA acceptance
 /*	   if($compName !== 'eula_acceptance'){
			if(!is_file("/etc/.eula_accepted")){

				if (!((($compName === 'system_factory_restore') &&
					($methodName === 'GET')) ||
							($compName === 'system_state') ||
							($compName === 'system_information') ||
							($compName === 'language_configuration'))) {

						$logObj->LogData('ERROR', get_class($compObject),  $methodName,  'EULA Not Accepted');
						header("HTTP/1.0 403 Forbidded");
						return;
					}
			}
		}
*/
		/*
		if ($methodName === 'PUT') {
			parse_str(file_get_contents("php://input"), $putData);
			foreach($putData as $key => $val){
				$queryParams[$key] = $val;
			}
		}
		*/
		if ($methodName === 'POST') {
			foreach($_POST as $key => $val){
				$queryParams[$key] = $val;
			}
		}
		if ($methodName == 'POST') {
			if (isset($_POST['rest_method'])) {
				$methodName = strtoupper($_POST['rest_method']);
				unset($_POST['rest_method']);
			}else if(isset($queryParams['rest_method'])){
				$methodName = $queryParams['rest_method'];
				$methodName = strtoupper($methodName);
				unset($queryParams['rest_method']);
			}
			$queryParams = $queryParams + $_POST;
		} else if ($methodName == 'GET') {
			if(isset($_GET['rest_method'])) {
				$methodName = strtoupper($_GET['rest_method']);
				unset($_GET['rest_method']);
			}
			$queryParams = $queryParams + $_GET;
		}

		// Call component HTTP method.
		//$logObj->LogData('BEGIN', get_class($compObject),  $methodName,  NULL);

		$securityType = $this->getSecurityTypeForMethod($compName, $methodName);
		if(!isLanRequest() && ($securityType == self::$NO_AUTH_LAN ||
								$securityType == self::$USER_AUTH_LAN ||
								$securityType == self::$ADMIN_AUTH_LAN)){
			setHttpStatusCode(401, "Request is allowed in LAN only.");
			return;
		}
		// Authentication check
		// Get Security type for REST method

		session_cache_limiter( FALSE );
		session_start();
		$authenticated = false;
		$authNotRequired = false;
		switch($securityType){
			case self::$NO_AUTH:
				$authNotRequired = true;
				break;

			case self::$USER_AUTH:
				$authenticated = isAuthenticated($queryParams, false);
				break;

			case self::$ADMIN_AUTH:
				$authenticated = isAuthenticated($queryParams, true);
				break;

			case self::$NO_AUTH_LAN:
				if(!isLanRequest()){
					session_destroy();
					setHttpStatusCode(401, "Request is allowed in LAN only.");
					return;
				}
				$authNotRequired = true;
				break;

			case self::$USER_AUTH_LAN:
					// isAuthenticated($queryParams, $isAdminRequired)
					if(!isLanRequest()){
						session_destroy();
						setHttpStatusCode(401, "Request is allowed in LAN only.");
						return;
					}
					$authenticated = isAuthenticated($queryParams, false);
				break;

			case self::$ADMIN_AUTH_LAN:
					if(!isLanRequest()){
						session_destroy();
						setHttpStatusCode(401, "Request is allowed in LAN only.");
						return;
					}
					$authenticated = isAuthenticated($queryParams, true);
				break;

		}

		if(!$authNotRequired){
			if(!$authenticated){
				session_destroy();
				setHttpStatusCode(401, "Authentication required.");
				return;
			}

			if( $authenticated && isSessionExpired()){
				deauthenticate();
				if($this->isAuthenticationRequired($compName)){
					setHttpStatusCode(401, "Session expired");
					return;
				}
			}
		}
		unset($queryParams['rest_method']);
		try {

			switch ($methodName) {
				case 'GET':
					$result = $compObject->get($urlPath, $queryParams, $outputFormat);
					break;
				case 'POST':
					$result = $compObject->post($urlPath, $queryParams, $outputFormat);
					break;
				case 'PUT':
					$result = $compObject->put($urlPath, $queryParams, $outputFormat);
					break;
				case 'DELETE':
					$result = $compObject->delete($urlPath, $queryParams, $outputFormat);
					break;
			}
				
		} catch (Exception $e) {
			$compObject->generateErrorOutput($e->getCode(), $compName, $e->getMessage(), $outputFormat);
		}

		if(!$authNotRequired){
			if( $authenticated && isRequestBasedAuthentication($queryParams)){
				deauthenticate();
			}
		}
		// Clear Session if request based authentication.

	}
}

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>