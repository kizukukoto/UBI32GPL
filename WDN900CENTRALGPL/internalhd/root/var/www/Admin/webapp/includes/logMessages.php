<?php


class LogMessages
{
	
	function LogMessages()
	{
	}

	function LogParameters($className, $methodName, $parameterArray)
	{
		openlog("REST_API", LOG_PID, LOG_LOCAL7);

		$methodName = strtoupper($methodName);
		
		if ($parameterArray === null)
		{
			syslog(LOG_INFO, $_SERVER["REMOTE_ADDR"]." PARAMETER $className $methodName : No Input paramters");
			
		}
		else if (!is_array($parameterArray))
		{
			syslog(LOG_INFO, $_SERVER["REMOTE_ADDR"]." PARAMETER $className $methodName : PARAMETER = $parameterArray");			
		}
		else
		{	
			$keys = array_keys($parameterArray);		
			$count = 0;
			foreach($parameterArray as $parameter)
			{
				syslog(LOG_INFO, $_SERVER["REMOTE_ADDR"]." PARAMETER $className $methodName : $keys[$count] = $parameter");
				$count++;
			}
		}
		closelog();

	}
	function LogData($beginEnd, $className, $methodName, $msgStatus) 
	{

		openlog("REST_API", LOG_PID, LOG_LOCAL7);

		$methodName = strtoupper($methodName);

		if ($beginEnd === 'BEGIN')
		{
			syslog(LOG_INFO, $_SERVER["REMOTE_ADDR"]." $beginEnd $className $methodName");
		}
		elseif ($msgStatus === 'SUCCESS' )
		{
			syslog(LOG_INFO, $_SERVER["REMOTE_ADDR"]." $beginEnd $className $methodName $msgStatus ");
		}
		else
		{
			syslog(LOG_WARNING, $_SERVER["REMOTE_ADDR"]." $beginEnd $className $methodName $msgStatus");			
		}
		closelog();
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
