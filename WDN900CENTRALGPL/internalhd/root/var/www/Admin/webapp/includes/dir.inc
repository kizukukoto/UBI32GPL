<?php
// Copyright � 2010 Western Digital Technologies, Inc. All rights reserved.

function getAttributes($path, $requestPath, $file=null, $includePermissions) {
	if (isset($file)) {
		$fullname = $path."/".$file;
	} else {
		$fullname = $path;
	}
	$fstat = @lstat($fullname);
	if (!$fstat) {
		return false;
	}
	if (is_dir($fullname)) {
		$attributes['is_dir'] = array('TYPE'=>'STRING', 'VALUE'=>"true");
	} else {
		$attributes['is_dir'] = array('TYPE'=>'STRING', 'VALUE'=>"false");
		//Big files (more than 2Gigs) need to use a special command to find the file size
		//blksize is incorrect so it is being hard-wired as 512 (we should use $fstat['blksize'])
		if(sysLargeFile($fstat['size'])) {
			$size = sysFSize($fullname);
			if($size === -1){
				$size = $fstat['size'];
			}			
		} else
		{
			$size = $fstat['size'];
		}

		$attributes['size'] = array('TYPE' => 'NUMBER', 'VALUE' => $size);

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'command', $command);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'result', $result);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'results', print_r($results,true));

	}
	// removed double slash in path
	// $attributes['path'] = $requestPath."/".$file;
	// $attributes['path'] = $requestPath.$file;
	// Keeping rtrim because request might come from ../<sharename> or ../<sharename>/
	$attributes['path'] = array('TYPE'=>'STRING', 'VALUE'=>rtrim($requestPath, '/'));
	if (isset($file)) {
		$attributes['name'] = array('TYPE'=>'STRING', 'VALUE'=>$file);
	}
	$attributes['mtime'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fstat['mtime']);
	//$attributes['ctime'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fstat['ctime']);
	if($includePermissions){
		$permissions = getPermissions($fstat);
		$attributes['permissions'] = array('TYPE'=>'STRING', 'VALUE'=>$permissions['permissions']);
		$attributes['owner_name'] = array('TYPE'=>'STRING', 'VALUE'=>$permissions['owner_name']);
		$attributes['group_name'] = array('TYPE'=>'STRING', 'VALUE'=>$permissions['group_name']);

		$attributes['mode'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fstat['mode']);
		$attributes['owner_id'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fstat['uid']);
		$attributes['group_id'] = array('TYPE'=>'NUMBER', 'VALUE'=>$fstat['gid']);
	}
	return $attributes;
}

function getDirList($dirFullPath, $requestPath, $getAttributes=true){
	$dirLsit = array();
	if (is_dir($dirFullPath)) {
		if ($dh = opendir($dirFullPath)) {
		while (($file = readdir($dh)) !== false) {
			//Ignore any file that starts with a period (e.g. '.', '..', '.hidden')
			if(strcmp(substr($file, 0, 1), '.')===0) {
				continue;
			}
			if ($getAttributes) {
				$attributes = getAttributes($dirFullPath, $requestPath, $file);
				if($attributes != false){
					$dirLsit[] = $attributes;
				}
			} else {
				$dirLsit[] = $file;
			}
		}
		closedir($dh);
		}
	}
	return $dirLsit;
}

function generateAttributes($output, $path, $requestPath, $file=null, $includePermissions) {
	if (isset($file)) {
		$fullname = $path."/".$file;
	} else {
		$fullname = $path;
	}
	$fstat = @lstat($fullname);
	if (!$fstat) {
		return false;
	}
	if (is_dir($fullname)) {
		$output->element("is_dir", "true");
	} else {
		$output->element("is_dir", "false");
		//Big files (more than 2Gigs) need to use a special command to find the file size
		//blksize is incorrect so it is being hard-wired as 512 (we should use $fstat['blksize'])
		if(sysLargeFile($fstat['size'])) {

			$size = sysFSize($fullname);
			if($size === -1){
				$size = $fstat['size'];
			}
		} else
		{
			$size = $fstat['size'];
		}

		$output->numberElement('size', $size);
	}
	$output->element("path", rtrim($requestPath, '/'));
	if (isset($file)) {
		$output->element("name", $file);
	}
	$output->numberElement('mtime', $fstat['mtime']);
	//$output->numberElement('ctime', $fstat['ctime']);

	if($includePermissions){
		$permissions = getPermissions($fstat);
		$output->element('permissions', $permissions['permissions']);
		$output->element('owner_name', $permissions['owner_name']);
		$output->element('group_name', $permissions['group_name']);

		$output->numberElement('mode', $fstat['mode']);
		$output->numberElement('owner_id', $fstat['uid']);
		$output->numberElement('group_id', $fstat['gid']);
	}
}

function generateDirList($output, $dirFullPath, $requestPath, $getAttributes=true, $includeHidden, $includePermissions, $singleDir=false) {
	if (is_dir($dirFullPath)) {
		$output->pushElement("dir");
		$output->pushArray("entry");
		if ($dh = opendir($dirFullPath)) {
			if($singleDir) {
				$output->pushArrayElement();
				if ($getAttributes) {
					generateAttributes($output, $dirFullPath, $requestPath, NULL, $includePermissions);
				} else {
					$dirPath = substr($dirFullPath, strlen(getSharePath())+1);
					$output->element("name", $dirPath);
				}
				$output->popArrayElement();
			} else {
				while (($file = readdir($dh)) !== false) {
					//Ignore any file that starts with a period (e.g. '.', '..', '.hidden')
					if(!$includeHidden) {
						if(strcmp(substr($file, 0, 1), '.')===0) {
							continue;
						}
					} else { // Including hidden fles and folders. So just skip . and ..
						if($file == '.' || $file == '..') {
							continue;
						}
					}
					$output->pushArrayElement();
					if ($getAttributes) {
						generateAttributes($output,$dirFullPath, $requestPath, $file, $includePermissions);
					} else {
						$output->element("name", $file);
					}
					$output->popArrayElement();
				}
			}
			closedir($dh);
		}
		$output->popArray();
		$output->popElement();
		$output->close();
	}
}

function getPermissions($fstat){
	//Code sinppet copied form http://www.php.net/manual/en/function.stat.php
	$ts=array(
	  0140000=>'ssocket',
	  0120000=>'llink',
	  0100000=>'-file',
	  0060000=>'bblock',
	  0040000=>'ddir',
	  0020000=>'cchar',
	  0010000=>'pfifo'
	);
	$mode = $fstat['mode'];
 	$t=decoct($mode & 0170000); // File Encoding Bit

 	$permissions =(array_key_exists(octdec($t),$ts))?$ts[octdec($t)]{0}:'u';
 	$permissions.=(($mode&0x0100)?'r':'-').(($mode&0x0080)?'w':'-');
 	$permissions.=(($mode&0x0040)?(($mode&0x0800)?'s':'x'):(($mode&0x0800)?'S':'-'));
	$permissions.=(($mode&0x0020)?'r':'-').(($mode&0x0010)?'w':'-');
	$permissions.=(($mode&0x0008)?(($mode&0x0400)?'s':'x'):(($mode&0x0400)?'S':'-'));
	$permissions.=(($mode&0x0004)?'r':'-').(($mode&0x0002)?'w':'-');
	$permissions.=(($mode&0x0001)?(($mode&0x0200)?'t':'x'):(($mode&0x0200)?'T':'-'));

	$owner = (function_exists('posix_getpwuid'))?@posix_getpwuid($fstat['uid']):'';
	$owner_name = '';
	if(isset($owner))
		$owner_name = $owner['name'];

	$group = (function_exists('posix_getgrgid'))?@posix_getgrgid($fstat['gid']):'';
	$group_name = '';
	if(isset($group))
		$group_name = $group['name'];

	return array("permissions" => $permissions, "owner_name" => $owner_name, "group_name" => $group_name);
}

function getChildListForDir($dirFullPath, $requestPath, $includeHidden){
	$fileList = array();
	if (is_dir($dirFullPath)) {
		if ($dh = opendir($dirFullPath)) {
			while (($file = readdir($dh)) !== false) {
				//Ignore any file that starts with a period (e.g. '.', '..', '.hidden')
				if(!$includeHidden){
					if(strcmp(substr($file, 0, 1), '.')===0) {
						continue;
					}
				}else{ // Including hidden fles and folders. So just skip . and ..
					if($file == '.' || $file == '..') {
						continue;
					}
				}
				$fullname = $dirFullPath."/".$file;
				$fileList[$file] = $fullname;
			}
			closedir($dh);
		}
	}
	return $fileList;
}

function sysFSize($fullname) {
	$command = "stat -t \"".$fullname."\"";
	$result  = exec($command);
	$results = explode(" ", $result);
	$cnt =  count($results);
	//get 14th item back ward
	
	$sizeIdx = $cnt - 14;
	$size = !empty($results[$sizeIdx]) ? $results[$sizeIdx] : -1;

	return $size;	
}

function sysLargeFile($size) {
	if (floatval($size)< 0) {
		return true;
	}
	return false;
}
?>