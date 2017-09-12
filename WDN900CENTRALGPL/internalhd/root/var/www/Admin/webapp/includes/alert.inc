<?php

/**
 * Container class for alerts. To create an alert instance you need to supply a line
 * of text from the alert log file to the constructor which will parse the alert log
 * line and set the member variables: $timestamp, $severity, $code, $message.
 * 
 * @author Sapsford_J
 *
 * Expected Alert Format:
 * Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 06: Network Interface Card failure
 * OR
 * Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 16: With subst values. +;sp1;share;device;-
 */

class Alert {

    private static  $months = array("Jan","Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
    
    public  $timestamp;
    public  $severity;
    public  $code;
    public  $message;
    public  $subst_values;

    function Alert($alertLogRow) {
        
        global $MONTHS;
     
        //NB: TBD - Fail if the alert is not in expected format
        
        //parse a row from the alert log and convert to an Alert instance
        
        $currentTime = time();
        $currentMonth = date("n", $currentTime);
        
        /** strip the last char (\r,\n etc) */
        $alertLogRow = rtrim( $alertLogRow );

        $alertFields = preg_split("/[\s]+/",$alertLogRow); 
        
        $alertMonth = substr($alertFields[0],0,3);
        $alertDayofMonth =  substr($alertFields[1],0,2);
        $alertYear = date("y", $currentTime);  //default to current year, see below for adjustment for previous year 
        $alertTime = explode(":",$alertFields[2]); 
        
        //as there is no year in the alert log date format, we need to test if the
        //log message is for the previous year and subtract 1 from the current year if it is
        //if current month < alert timestamp month, then subtract 1 from year.
        
        $alertMonthNumber = array_search($alertMonth, self::$months) + 1;
        
        if ( $currentMonth < ($alertMonthNumber) ) {
            $alertYear -= 1;   //intended "bug" - if alert is 12 months old, then year will be set to current year
                               //as alert logs are regularly rolled-over and purged this *should* not be an issue
        }
        
        $this->timestamp = mktime($alertTime[0],$alertTime[1],$alertTime[2], $alertMonthNumber, $alertDayofMonth, $alertYear);
        $this->severity = substr($alertFields[5],0,-1);
        $this->code = substr($alertFields[6],0,-1);


        /** -- Adding support for substitution values */

        // NB: Format of last element is +;<value1>;<value2>;<value3>;-

        /** get the entire message if no substitution values specified */
        if ( ! strstr( $alertLogRow, "+;" ) )
        {
            /** re-build message from array */
            $this->message = implode( " ", array_slice( $alertFields, 7 ) );
        }
        else
        {
            /**  split the alert on " +;" string */
            $subst_fields = preg_split( "/[\s]+\+;/", $alertLogRow );

            /** get the last element */
            $end_str = $subst_fields[1];
            //echo( "End String ".$end_str."\n" );

            /** get the substitution values from end of the alert log */
            $subst_values = preg_split( "/;/", $end_str );
            //echo( "ALL SUBST VALUES: ".implode( ".", $subst_values )."\n" );

            /** get the substitution values & ignore "-" end-tag */
            if( isset( $subst_values ) && !empty( $subst_values[0] ) )
            {
                $this->subst_values = array_slice( $subst_values, 0, -1 );
            }
            else
            {
                $this->subst_values = NULL;
            }

            /** re-build message from array, ignore last element */
            $alertFields = preg_split( "/[\s]+/", $subst_fields[0] );
            $this->message = implode( " ", array_slice( $alertFields, 7 ) );
        }
    }
}

//test code
/*
$str = "Nov  5 15:58:33 localhost wdnas3g: CRITICAL: 06: Network Interface Card failure";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Mar  5 15:58:33 localhost wdnas3g: CRITICAL: 06: Network Interface Card failure";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Jun  8 15:58:33 localhost wdnas3g: CRITICAL: 16: Without subst values. +;;-";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 16: With 1 subst values. +;sp1;-";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 16: With subst values. +;sp1;share;device;-";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 16: With subst 1 space value. +;sp1 name;-";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");

$str = "Sep  9 14:58:33 localhost wdnas3g: CRITICAL: 16: With subst space values. +;sp1 name;share name;device name;-";
$alert = new Alert( $str );
echo("Original Alert: " . $str );
echo("\ntimestamp : " . $alert->timestamp);
echo("\nseverity : " . $alert->severity);
echo("\ncode : " . $alert->code);
echo("\nmessage : " . $alert->message);
if( isset( $alert->subst_values ) )
{
    echo( "\nsubst str: ".implode( "|", $alert->subst_values ) );
}
echo("\n");
echo("\n");
*/

?>
