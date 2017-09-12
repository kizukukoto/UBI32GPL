<?php
	// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
	
	require_once("util.inc");
	require_once("filesystem.inc");
	require_once("NasXmlWriter.class.php");
	
	
	/**
	 *
	 *  XML Format for Directory contents: 
	 
<FILECHANNEL volume={volume}, host={hostname} >
	<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir}>
		<ATTRIBUTES>
			<READONLY>{true/false}</READONLY>
			<CREATED>{datetime}</CREATED>
			<LASTMODIFIED>{lastmodified}</LASTMODIFIED>
		</ATTRIBUTES>
		<FILE name={name}, path={pathurl} id={id}, owner={owner}, isdir={true/false}>
			<ATTRIBUTES>
				<READONLY>{true/false}</READONLY>
				<CREATED>{datetime}</CREATED>
				<LASTMODIFIED>{lastmodified}</LASTMODIFIED>
				<SIZE>{sizeinbytes}</SIZE>
				<OWNER>{owner}</OWNER>
			</ATTRIBUTES>		
			<PERMISSIONS owner={rwx}, group={rwx}, others={rwx} />
		</FILE>
	</DIRECTORY>
</FILECHANNEL>


***
* XML Format for Directory tree

<FILECHANNEL volume={volume}, host={hostname} >
	<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir}>
		<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
		<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
		<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir}>
			<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
			<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
			<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} >
				<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
				<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
				<DIRECTORY name={name} id={id} owner={owner} parent={ParentDir} />
			</DIRECTORY>
		</DIRECTORY>
		...
		...
		etc
		...
		...
	</DIRECTORY>	
</FILECHANNEL>



*/
	
	function getUserHome()
	{
		$dirConfig = getGlobalConfig("directories");
		if (isset($dirConfig) && sizeof($dirConfig) > 0 )
		{
			$userHome = $dirConfig['DIRHOMEPATH'];
		}
		else
		{
			$userHome = "/user/home";
		}
		return $userHome;
	}
	
	function genDirContentsXML($dirPath)
	{
		global $_SERVER;
		$userHome = getUserHome(); 
		$dirPath = $userHome . "/". $dirPath;
		$dirName = end(explode('/',$dirPath));
		$dirAttrs = getDirAttributes($dirPath);
		if (isset($dirAttrs) && (sizeof($dirAttrs) > 0) ) {
			$fileFilter = array();
			$fileFilter[0] = "*";
			$contents = getDirContents($dirPath, $dirName, $fileFilter);
			if (isset($contents) && (sizeof($contents) > 0) ) {
				$xml = new NasXmlWriter();
				$xml->push('filechannel', array('volume' => $dirAttrs['dir_volume'], 'hostname' => $_SERVER['SERVER_NAME']) );
					$xml->push('directory', array('name' => $dirAttrs['dir_name'], 'id' => $dirAttrs['dir_id'], 'owner' => $dirAttrs['dir_owner'], 'parent' => $dirAttrs['dir_parent']) );
							$xml->push('attributes');
								$xml->element('readonly', $dirAttrs['dir_readonly'] ? "true" : "false");
								$xml->element('created', $dirAttrs['dir_created']);
								$xml->element('lastmodified', $dirAttrs['dir_modified']);
							$xml->pop();
							foreach($contents as $content)
							{
								$fileName = end(explode('/', $content['file_path']));
								$xml->push('file', array('name'=>$fileName, 'id'=>$content['file_id'], 'owner'=>$content['file_owner'], 'isdir'=>($content['file_isdir'] ? "1" : "0") ));
									$xml->push('attributes');
										$xml->element('readonly', $content['file_readonly'] ? "true" : "false");
										$xml->element('created', $content['file_created']);
										$xml->element('lastmodified', $content['file_modified']);
										$xml->element('size', $content['file_size']);
										$xml->element('owner', $content['file_owner']);
									$xml->pop();
								$xml->pop();
							}
					$xml->pop();
				$xml->pop();
				return $xml->getXml();		 
			}
		}
		return NULL;
	}
	
	function genDirTreeXMLRecursive($path, $xmlin = NULL)
	{
		$userHome =  getUserHome();
		if ($xmlin === NULL)
		{ //first call
			$xml = new NasXmlWriter();
			$dirPath = $userHome . "/" . $path;
			$first = true;
			$popCount = 0;
		}
		else 
		{  //recursive call
			$xml = $xmlin;
			$dirPath = $path;
			$first = false;
		}
		$dirRelPath = substr($dirPath, strlen($userHome)+1);

		$dirAttrs = getDirAttributes($dirPath);
		if (isset($dirAttrs) && (sizeof($dirAttrs) > 0) ) 
		{
			$fileFilter = array("*");
			if ($first)
			{
				$xml->push('filechannel', array('volume' => $dirAttrs['dir_volume'], 'hostname' => $_SERVER['SERVER_NAME']) );
				$first = false;
			}
			$dirName = end(explode('/',$dirAttrs['dir_name']));
			if ((substr($dirName,0,1) != '.'))
			{
				$contents = getDirContents($dirPath, $dirName, $fileFilter);
				if (isset($contents) && (sizeof($contents) > 0) ) 
				{
					$xml->push('directory', array('name' => $dirName, 
												  'path' => $dirRelPath, 	
												  'id' => $dirAttrs['dir_id'], 
												  'owner' => $dirAttrs['dir_owner'], 
												  'parent' => $dirAttrs['dir_parent']) );
					foreach($contents as $content)
					{
						if ($content['file_isdir'])
						{
							$childPath = $content['file_path']; 
							if ((substr($childPath,strlen($childPath)-1,1) != '.'))
							{
								genDirTreeXMLRecursive($childPath, $xml, $popCount);
							}
						}
					}
					$xml->pop();
				}
				else
				{
					$xml->emptyElement('directory', array('name' => $dirName, 
												   'path' => $dirRelPath,
												   'id' => $dirAttrs['dir_id'], 
												   'owner' => $dirAttrs['dir_owner'], 
												   'parent' => $dirAttrs['dir_parent']) );
				}
			}
			if ($xmlin === NULL)
			{//back at top
				$xml->pop();
				return $xml->getXml();
			}
		}
		return NULL;
	}
	
	function genDirTreeXML($pathList)
	{
		//walk path to gather directories and create an XML representation of the tree structure
		//if recurse is true, we wil expand teh entire ir tree from the given path and return it.
		
		$userHome = getUserHome(); 
		$first = true;
		$popCount = 0;
		$xml = new NasXmlWriter();
		$dirPath = $userHome;
		for ($i = 0; $i < sizeof($pathList); ++$i )
		{
			$dirPath = $dirPath . "/" .$pathList[$i];
			$dirRelPath = $i==0 ? $pathList[$i] : $dirRelPath . "/" .  $pathList[$i];
			
			$dirAttrs = getDirAttributes($dirPath);
			if (isset($dirAttrs) && (sizeof($dirAttrs) > 0) ) {
				$fileFilter = array();
				$fileFilter[0] = "*";
				if ($first)
				{
					$xml->push('filechannel', array('volume' => $dirAttrs['dir_volume'], 'hostname' => $_SERVER['SERVER_NAME']) );
					$first = false;
					++$popCount;
				}

				$dirName = end(explode('/',$dirAttrs['dir_name']));
				if ((substr($dirName,0,1) != '.'))
				{
					$contents = getDirContents($dirPath, $dirName, $fileFilter);
					if (isset($contents) && (sizeof($contents) > 0) ) 
					{
						$xml->push('directory', array('name' => $dirName, 
													  'path' => $dirRelPath, 	
													  'id' => $dirAttrs['dir_id'], 
													  'owner' => $dirAttrs['dir_owner'], 
													  'parent' => $dirAttrs['dir_parent']) );
						++$popCount;
						foreach($contents as $content)
						{
							if ($content['file_isdir'])
							{
								$dirName = end(explode('/', $content['file_path']));
								if ((substr($dirName,0,1) != ".") &&
									(($i == sizeof($pathList)-1) ||
								     ($i < sizeof($pathList)-1) && ( strcmp($dirName, $pathList[$i+1]) != 0)) )
								{
									$xml->push('directory', array('name' => $dirName, 
																		  'path' => $dirRelPath . "/" . $dirName, 	
																		  'id' => $content['file_id'], 
																		  'owner' => $content['file_owner'], 
																		  'parent' => $dirAttrs['dir_parent']) );
									$xml->emptyElement('directory', array('name' => "loading..."));
									$xml->pop();
								}
							}
						}
					}
					else
					{
						$xml->push('directory', array('name' => $dirName, 
													   'path' => $dirRelPath,
													   'id' => $dirAttrs['dir_id'], 
													   'owner' => $dirAttrs['dir_owner'], 
													   'parent' => $dirAttrs['dir_parent']) );
						$xml->emptyElement('directory', array('name' => "loading..."));
						$xml->pop();
					}
				}
			}
		}
		for ($i = 0; $i < $popCount; ++$i) {
			$xml->pop();
		}
		return $xml->getXml();		 
	}
	
	//test code
	
	//$xmldoc = genDirXML("C:\dump");
	//var_dump($xmldoc);
	
	//$xmldoc = genDirTreeXML("joe/My Music");
	//var_dump($xmldoc);
	
	//$xmldoc= genDirTreeXMLRecursive("joe");
	//var_dump($xmldoc);
	
?>