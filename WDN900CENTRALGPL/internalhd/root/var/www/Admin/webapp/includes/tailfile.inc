<?php 

/**
 * 
 * This function will read the last $lineCount lines from the file with 
 * path: $filePath and return them as an array of strings.
 * 
 * Unlike the shell 'tail' command, this function can return the lines from last to first,
 * so the first line in the array is the last line in the file, the second, the last -1 etc.
 * To get this behaviour, simply set $lastFirst to true. 
 *
 * You can pass in the name of a function to call in the $callback parameter, this
 * function should have one parameter only and that parameter will be passed a
 * line from the file. 
 *
 * Class methods may also be invoked statically using this function by passing array($classname, $methodname) 
 * to $callback. Additionally, class methods of an object instance may be called by 
 * passing array($objectinstance, $methodname) to $callback.  
 * 
 * @param string $filePath - fully qualified path of file to tail
 * @param int $lineCount - number of lines to tail
 * @param boolean $lastFirst - if true, the last line in the file will be at index 0 in the array, etc.
 * @param string $callback - name of function to be called for each lined tailed.
 * @return an array of strings, one string per line, or null on failure.
 * 
 * @author Sapsford_J

 */

function tailFile($filePath, $lineCount, $lastFirst = false, $callback = null)
{
	if (!file_exists($filePath)) {
		return null;
	}
	
	$file = fopen($filePath, "r");
	if ($file === false) {
		return null;
	}
	
	$avgLineLen = 80;
	$fileSize = filesize( $filePath );
	
	$readLines = array();
	$overShoot = 2;
	while (true) {
		$seekOffset = (($lineCount + $overShoot) * $avgLineLen); 
		if ($seekOffset > $fileSize) {
			$seekOffset = $fileSize;
		}
		fseek($file, 0-$seekOffset, SEEK_END); //negative seek offset as we are seeking from end
		//discard first line, if not at start
		$seekOffset < $fileSize ? fgets($file) : null;
		//read lines from offset till end
		$linesRead = 0;
		while (true)
		{
			if ($fileLine = fgets($file)) {
				$readLines[] = $fileLine;
				++$linesRead;
			}
			else {
				//eof
				break;	
			}
		}
		//check for correct number of lines or whole file being read
		if ( ($linesRead == $lineCount) || ($seekOffset == $fileSize))
		{
			break;
		}
		if ($linesRead > $lineCount) {
			//get last $lineCount lines 
			$readLines = array_slice($readLines, $linesRead - $lineCount);
			break;
		}
		
		//too few lines, re-seek from further up in the file
		$overShoot += 4;
		$readLines = array();
	}
	
	if ($lastFirst) {
		//using array_reverse  to reverse the order at the end is a lot more efficient 
		//than using array_shift() during iteration each time we add a line
		$readLines = array_reverse($readLines);
	}
	
	if ($callback !== null) {
		//call callback function for each line
		foreach($readLines as $line) {
			call_user_func($callback, $line);
		}
	}
	
	return $readLines;
}

/*
 * Test code
 * 
 */

/*
function testCallBack($line)
{
	var_dump($line);
}

$lines = tailFile("C:\\SVNBranches\\Nas3G\\REST\\webapp\\htdocs\\logs\\wdalerts.log", 10, true, "testCallBack");

var_dump($lines);
*/

?>