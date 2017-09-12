<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.

class OutputWriter {
	var $format;
	var $indent;
	var $indentOffset;
	var $isFirstElement;
	var $stack = array();

	/**
	 *
	 * Creates a new OutputWriter
	 * @param $format must be 'XML' or 'JSON'
	 * @param $indent optional parameter of the indentation which defaults to two spaces
	 */
	public function OutputWriter($format, $indent = '  ') {
		$this->format = $format;
		$this->indent = $indent;
		$this->indentOffset = 0;
		$this->isFirstElement = 1;

		if ( $this->format =='XML' ) {
			header('Content-type: text/xml; charset=utf-8');
			echo '<?xml version="1.0" encoding="utf-8"?>';
		} else if ( $this->format =='JSON' ) {
			header('Content-type: text/html; charset=utf-8');
			echo '{';
			array_push($this->stack, '');
		}else{
			die("400 Invalid format!");
		}
	}

	/**
	 * Must be called at the very end of the document to properly close it
	 */
	public function close() {
		if ( $this->format =='XML' ) {
			echo '';
		} else if ( $this->format =='JSON' ) {
			array_pop($this->stack);
			$this->_indent();
			echo '}';
		}
	}

	/**
	 * Adds the line break of the previous line and the indentation for the new line.
	 */
	private function _indent() {
		echo "\n";
		$indentCount = count($this->stack) - $this->indentOffset;
		for ($i = 0; $i < $indentCount; $i++) {
			echo $this->indent;
		}
	}

	/**
	 * Begins a new element.  When the contents of the element have been completed, popElement must be called.
	 * @param $element the name of the element to be added
	 */
	public function pushElement($element) {
		if ( $this->format == 'XML' ) {
			$this->_indent();
			echo "<$element>";
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$element.'":{';
		}
		$this->stack[] = $element;
		$this->isFirstElement = 1;
	}

	/**
	 * Ends an element that was started with pushElement
	 */
	public function popElement() {
		$element = array_pop($this->stack);
		$this->_indent();
		if ( $this->format =='XML' ) {
			echo "</$element>";
		} else if ( $this->format =='JSON' ) {
			echo '}';
		}
		$this->isFirstElement = 0;
	}

	public function escape_json_string($input){
		return addcslashes($input,'\"\\');
	}
	
	/**
	 * Adds an element with the specified contents (e.g. <name>Eric</name>)
	 * @param $element name of the element
	 * @param $content contents contained within the element
	 */
	public function element($element, $content) {
		if ( $this->format =='XML' ) {
			$this->_indent();
			echo '<'.$element.'>'.htmlspecialchars($content, ENT_QUOTES, 'UTF-8').'</'.$element.'>';
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$element.'":"'.$this->escape_json_string($content).'"';
		}
		$this->isFirstElement = 0;
	}

	/**
	 * Adds an element with the specified contents (e.g. <name>Eric</name>).
	 * Removed double quotes for JSON format
	 * @param $element name of the element
	 * @param $content contents contained within the element
	 */
	public function numberElement($element, $content) {
		if ( $this->format =='XML' ) {
			$this->_indent();
			echo '<'.$element.'>'.htmlspecialchars($content, ENT_QUOTES, 'UTF-8').'</'.$element.'>';
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$element.'":'.$this->escape_json_string($content).'';
		}
		$this->isFirstElement = 0;
	}

   /**
	 * Adds or pushes an element with the specified sttribute list and contents (e.g. <name>Eric</name>)
	 * @param $element name of the element
	 * @param $attributes associative array of attribute-id,value pairs
	 * @param $push if true element is pushed on the stack and not closed, if false, element is closed
	 * @param $content contents contained within the element
	 */
	private function insertElementWithAttributes($element, $attributes, $push=false, $content=NULL) {
		if (isset($element)) {
			if ( $this->format =='XML' ) {
				$this->_indent();
				echo '<'.$element ;
				if (isset($attributes)) {
					foreach($attributes as $id => $value) {
						echo ' '.$id.'='.$value.' ';
					}
				}
				if (isset($content)) {
					echo '>';
					echo htmlspecialchars($content, ENT_QUOTES, 'UTF-8');
					if (!$push) {
						echo'</'.$element.'>';
					}
				}
				else if ($push) {
					echo '>';
				}
				else {
					echo ' />';
				}
			} else if ( $this->format =='JSON' ) {
				if (!$this->isFirstElement)
					echo ', ';
				$this->_indent();
				if (isset($attributes)) {
						echo '"'.$element.'": { "@attributes":{';
					$first = true;
					foreach($attributes as $id => $value) {
						if (!$first)
							echo ',';
						else
							$first = false;
						echo '"'.$id.'":"'.$value.'"';
					}
					echo '}';
					if (isset($content)) {
						echo '", "@content":"' . $this->escape_json_string($content).'"';
					}
					echo '}';
				}
				else {
					echo '"'.$element.'":"'.$this->escape_json_string($content).'"';
				}
			}
			if ($push) {
				$this->stack[] = $element;
				$this->isFirstElement = 1;
			}
			else {
				$this->isFirstElement = 0;
			}
		}
	}
	/**
	 * Adds an element with the specified sttribute list and contents (e.g. <user id=12345>Eric</user>)
	 * @param $element name of the element
	 * @param $attributes associative array of attribute id,value pairs
	 * @param $content contents contained within the element
	 */
	public function elementWithAttributes($element,  $attributes, $content=NULL) {
		$this->insertElementWithAttributes($element, $attributes, false, $content);
	}

	/**
	 * Pushes an element with the specified sttribute list and contents (e.g. <user id=12345>Eric</user>)
	 * @param $element name of the element
	 * @param $attributes associative array of attribute id,value pairs
	 * @param $push if true element is pushed on the stack and not closed, if false, element is closed
	 * @param $content contents contained within the element
	 */
	public function pushElementWithAttributes($element,  $attributes, $content=NULL) {
		$this->insertElementWithAttributes($element, $attributes, true, $content);
	}

	/**
	 * Adds a single element with no contents (e.g. <br/>)
	 * @param $element name of element to be added which has no contents.
	 */
	public function emptyelement($element) {
		if ( $this->format =='XML' ) {
			$this->_indent();
			echo '<'.$element.'/>';
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$element.'":null';
		}
		$this->isFirstElement = 0;
	}

	/**
	 * This begins a new array where each entry within the array will by of the specified type of element.
	 * After all elements have been added, popArray must be called.
	 * @param $element type of element for each entry in the array
	 */
	public function pushArray($element) {
		if ( $this->format == 'XML' ) {
			//XML doesn't show the beginning of an array
			$this->indentOffset++;
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$element.'":[';
			$this->isFirstElement = 1;
		}
		array_push($this->stack, $element);
		$this->isFirstElement = 1;
	}

	/**
	 * This ends the whole array that was started with pushArray.
	 */
	public function popArray() {
		$element = array_pop($this->stack);
		if ( $this->format =='XML' ) {
			//XML doesn't show the ending of an array
			$this->indentOffset--;
		} else if ( $this->format =='JSON' ) {
			$this->_indent();
			echo ']';
		}
		$this->isFirstElement = 0;
	}

	/**
	 * Begins a new element of an array.  After this has been called and the contents of the array have been completed,
	 * then popArrayElement will then need to be called to complete this element of the array.
	 */
	public function pushArrayElement() {
		if ( $this->format =='XML' ) {
			//peek to see what is on the stack
			$element = array_pop($this->stack);
			array_push($this->stack, $element);
			$this->_indent();
			echo '<'.$element.'>';
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '{';
		}
		array_push($this->stack, '');
		$this->isFirstElement = 1;
	}

	/**
	 * Ends an element within an array.  This should have been proceeded with a pushArrayElement
	 */
	public function popArrayElement() {
		array_pop($this->stack);
		$this->_indent();
		if ( $this->format =='XML' ) {
			//peek to see what is on the stack
			$element = array_pop($this->stack);
			array_push($this->stack, $element);
			echo '</'.$element.'>';
		} else if ( $this->format =='JSON' ) {
			echo '}';
		}
		$this->isFirstElement = 0;
	}

	/**
	 * Adds an element to the array with the specified content.  The element for this content must have been previously
	 * defined using the pushArray method.
	 * @param unknown_type $content the content within the new element
	 */
	public function arrayElement($content) {
		if ( $this->format =='XML' ) {
			//peek to see what is on the stack
			$element = array_pop($this->stack);
			array_push($this->stack, $element);
			$this->_indent();
			echo '<'.$element.'>'.htmlspecialchars($content, ENT_QUOTES, 'UTF-8').'</'.$element.'>';
		} else if ( $this->format =='JSON' ) {
			if (!$this->isFirstElement)
				echo ', ';
			$this->_indent();
			echo '"'.$this->escape_json_string($content).'"';
		}
		$this->isFirstElement = 0;
	}
}

/*
$output = new OutputWriter('XML');
$attrs = array("id" => "12345", "first" => "john", "second" => "henry");
$output->elementWithAttributes("user",$attrs);
$output->close();
$output->pushElement('family');
$output->element('lastname', 'simpson');
$output->element('phone', '555-1212');
$output->pushElement('address');
$output->emptyelement('residential');
$output->element('street', 'happy rd.');
$output->element('city', 'happyville');
$output->popElement();
$output->emptyelement('married');
$output->pushArray('child');
$output->arrayElement('kid1');
$output->arrayElement('kid2');
$output->arrayElement('kid3');
$output->popArray();
$output->pushArray('pet');
$output->pushArrayElement();
$output->element('name', 'Skippy');
$output->element('type', 'Dog');
$output->popArrayElement();
$output->pushArrayElement();
$output->element('name', 'Crabby');
$output->element('type', 'Hermit Crab');
$output->popArrayElement();
$output->popArray();
$output->popElement();
$output->close();
*/
?>