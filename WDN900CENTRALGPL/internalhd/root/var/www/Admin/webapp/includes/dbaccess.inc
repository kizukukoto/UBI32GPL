<?php
// Copyright © 2010 Western Digital Technologies, Inc. All rights reserved.
require_once('dbutil.inc');

class DBAccess {
	/**
	 *
	 * @var Prepared stementes cache array.
	 */
	private static $pdoPreparedStatementsCache = array();

	/**
	 * Returns prepared statement from the cache. If statement is not available
	 * in the cache this function creates new statement and updates cache.
	 *
	 * @param $sql						SQL query
	 * @param $cacheKey					Prepared Statements cache key
	 */
	function getPreparedStatement($sql, $cacheKey) {
		if(!array_key_exists($cacheKey, self::$pdoPreparedStatementsCache)){
			$db = getDb();
			self::$pdoPreparedStatementsCache[$cacheKey] = $db->prepare($sql);
		}
		return self::$pdoPreparedStatementsCache[$cacheKey];
	}

	/**
	 * Binds prepared statement variables.
	 *
	 * @param $stmt						Prepared statement
	 * @param $bindVarNVTArray			Prepared stements bind variables name, value and type.
	 */
	function bindVariables($stmt, $bindVarNVTArray) {
		foreach ($bindVarNVTArray as $bindVarKey => $bindVarInfo) {
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'pdo', print_r($pdo,true));
			$stmt->bindParam($bindVarInfo[0], $bindVarInfo[1], $bindVarInfo[2]);
		}
	}

	/**
	 * Executes query and fetches all rows.
	 *
	 * @param $sql						SQL query
	 * @param $cacheKey					Prepared Statements cache key
	 * @param $prepareStementNVTArray	Prepared stements bind variables name, value and type.
	 */
	function executeQuery($sql, $cacheKey, $bindVarNVTArray=NULL) {
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
		try {
			$stmt = $this->getPreparedStatement($sql, $cacheKey);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));
			if (!empty($bindVarNVTArray)) {
				$this->bindVariables($stmt, $bindVarNVTArray);
			}
			$ret = $stmt->execute();
			$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$rows = $stmt->fetchAll();
			return $rows;
		} catch(PDOException $e) {
			echo 'DBAccess::executeQuery : '.$e;
		}
		return null;
	}

	/**
	 * Executes a plain old SQL query (no prepared statement)
	 *
	 * @param string $sql
	 * @return $count Count of numbers of rows affected.
	 */
	function executeSql($sql) {
		$count = false;
		$db = getDb();
		$count = $db->exec($sql);
		return $count;
	}

	/**
	 * Executes update sql. This function is not using prepared statements.
	 *
	 * @param string $sql
	 * @return $status Return true if update was a success.
	 */
	function executeUpdate($sql) {
		$count = $this->executeSql($sql);
		if ($count == 0) {
			return false;
		}
		return true;
	}

	/**
	 * Executes query and fetches one row.
	 *
	 * @param $sql
	 * @param $cacheKey
	 * @param $bindVarNVTArray
	 */
	function executeQueryAndFetchOneRow($sql, $cacheKey, $bindVarNVTArray=NULL, $fetchAssoc=false) {
		try {
			$stmt = $this->getPreparedStatement($sql, $cacheKey);
			if(!empty($bindVarNVTArray)){
				$this->bindVariables($stmt, $bindVarNVTArray);
			}

			$ret = $stmt->execute();
			if($fetchAssoc == true)
				$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$rows = $stmt->fetch();
			return $rows;
		} catch(PDOException $e) {
			echo 'DBAccess::executeQueryAndFetchOneRow : '.$e;
		}
		return null;
	}

	/**
	 * Executes insert sql and returns last inserted row id.
	 *
	 * @param $sql						SQL query
	 * @param $cacheKey					Prepared Statements cache key
	 * @param $prepareStementNVTArray	Prepared stements bind variables name, value and type.
	 */
	function executeInsert($sql, $cacheKey, $bindVarNVTArray=NULL) {
		$lastInsertId = -1;
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		try {
			$stmt = $this->getPreparedStatement($sql, $cacheKey);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));
			if (!empty($bindVarNVTArray)) {
				$this->bindVariables($stmt, $bindVarNVTArray);
			}
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));
			$ret = $stmt->execute();
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'ret', $ret);
			if($ret == true) {
				$db = getDb();
				$lastInsertId = $db->lastInsertId();
			}
		} catch(PDOException $e) {
			var_dump($e);
		}
		return $lastInsertId;
	}

	/**
	 * Executes delete sql and returns true if success.
	 *
	 * @param $sql						SQL query
	 * @param $cacheKey					Prepared Statements cache key
	 * @param $prepareStementNVTArray	Prepared stements bind variables name, value and type.
	 */
	function executeDelete($sql, $cacheKey, $bindVarNVTArray=NULL) {

		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'sql', $sql);
		//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'bindVarNVTArray', print_r($bindVarNVTArray,true));

		$retVal = false;
		try {
			$stmt = $this->getPreparedStatement($sql, $cacheKey);

			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));

			if (!empty($bindVarNVTArray)) {
				$this->bindVariables($stmt, $bindVarNVTArray);
			}
			$retVal = $stmt->execute();
		} catch(PDOException $e) {
			return false;
		}
		return $retVal;
	}

	/**
	 * Executes update sql. This function is using prepared statements.
	 *
	 * @param $sql
	 * @param $cacheKey
	 * @param $bindVarNVTArray
	 */
	function executeUpdateWithPreparedStatements($sql, $cacheKey, $bindVarNVTArray=NULL) {
		$retVal = false;
		try {
			$stmt = $this->getPreparedStatement($sql, $cacheKey);
			if (!empty($bindVarNVTArray)) {
				$this->bindVariables($stmt, $bindVarNVTArray);
			}
			$retVal = $stmt->execute();
		} catch(PDOException $e) {
		}
		return $retVal;
	}

	/**
	 * Util function to add column in update SQL
	 *
	 * @param $columnName
	 * @param $columnValue
	 * @param $sql
	 * @param $isColAdded
	 */
	function addColumnToUpdateSQL($columnName, $columnValue, $sql, $isColAdded) {
		//if (isset($columnValue) && !(trim($columnValue)==='')) {
		if ($columnValue !== null) {
			if ($isColAdded) {
				$sql = $sql. ", ";
			}
			$sql = $sql. " $columnName='$columnValue' ";
			return array('sql' => $sql, 'isColAdded' => true);
		} else {
			return array('sql' => $sql, 'isColAdded' => $isColAdded);
		}
	}

	/**
	 *
	 * Util function to generate an update statement for a given table name, index column and
	 * column, value pairs held in an associative array
	 *
	 * @param $tableName string name of table
	 * @param $whereColumn string name of column in WHERE clause
	 * @param $whereColumnValue any value to match against whereColumn in WHERE clause
	 * @param $columnValues associative array of column names and values
	 *
	 * JS - added 06/14/10
	 *
	 */
	function generateUpdateSql($tableName, $whereColumn, $whereColumnValue, $columnValues) {
		$sql = "UPDATE " . $tableName . " SET ";
		$colvals = "";
		foreach ($columnValues as $col => $value) {
			if ($value !== null) {
				if (!empty($colvals)) {
					$colvals = $colvals . ",";
				}
				if (is_string($value)) {
					//for sqlite, double-up single quotes
					$value = str_replace("'","''",$value);
					$colvals = $colvals . $col . "='" . trim($value) . "'";
				} else {
					$colvals = $colvals . $col . "=" . $value;
				}
			}
		}
		$sql = $sql . $colvals;
		if (is_string($whereColumnValue)) {
			$whereColumnValue = "'" . trim(getSafeDatabaseText($whereColumnValue)) . "'";
		}
		$sql = $sql . " WHERE " . $whereColumn . "=" . $whereColumnValue;
		return $sql;
	}

	/**
	 * Executes query and fetches all rows.
	 */
	function executeQueryWithDb($db, $sql) {
		try {
			$stmt = $db->prepare($sql);
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));
			if (!is_object($stmt)) return null;
			$ret = $stmt->execute();
			$stmt->setFetchMode(PDO::FETCH_ASSOC);
			$rows = $stmt->fetchAll();
			return $rows;
		} catch(PDOException $e) {
			echo 'DBAccess::executeQueryWithDb : '.$e;
		}
		return null;
	}

	/**
	 * Executes query and returns stmt/cursor.
	 */
	function executeQueryWithDbCursor($db, $sql) {
		try {
			$stmt = $db->prepare($sql, array(PDO::ATTR_CURSOR => PDO::CURSOR_FWDONLY));
			//printf("<PRE>%s.%s=[%s]</PRE>\n", __METHOD__, 'stmt', print_r($stmt,true));
			if (!is_object($stmt)) return null;
			$ret = $stmt->execute();
			return $stmt;
		} catch(PDOException $e) {
			echo 'DBAccess::executeQueryWithDbCursor : '.$e;
		}
		return null;
	}
}
?>