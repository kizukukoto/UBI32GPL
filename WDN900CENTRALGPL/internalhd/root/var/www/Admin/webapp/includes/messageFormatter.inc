<?php

/**
 * Copyright (c) [2011] Western Digital Technologies, Inc. All rights reserved.
 *
 * @author - Nauzad Sadry
 *
 * Function to format a message with substituted values
 * 
 * @param string $message      - input message to format
 * @param array  $subst_values - values to replace
 * 
 * Message should be in following format:
 * Test %1 must be in %2 format
 * 
 * Rules:
 * - The placeholders must begin with %1
 * - There can be multiple placeholders of same value,
 *   e.g. Test %1 with same test %1
 * - If subst_values is NULL then the same message is returned unchanged
 * - If the message contains more placeholders than substitution values 
 *   then the remaining placeholders with "%undefined%"
 * 
 */

function formatMessage( $message, $subst_values )
{
    /** return the message is no substitution is reqd */
    if( ! isset( $subst_values ) )
    {
        return( $message );
    }

    /** get no of substitution values */
    $count = count( $subst_values );

    /** substitute each %x with corresponding index value */
    for( $i = 0; $i < $count; $i++ )
    {
        $j = $i + 1;
        $message = preg_replace( "/%${j}/", $subst_values[$i], $message );
    }

    /** substitute any remaining %x with <undefined> */
    if( preg_match( "/%[\d]+/", $message ) )
    {
        $message = preg_replace( "/%[\d]+/", "%undefined%", $message );
    }

    // echo( "Substituted DESCRIPTION is ".$description."\n" );

	return( $message );
}

/*
 * Test code
 */

/*
$msg = "Without subst values.";
echo( "Output ".formatMessage($msg, NULL)."\n" );

$msg = "With subst values; no placeholder.";
$subst_values = array( "GC1", "share", "device", "20" );
echo( "Output ".formatMessage($msg, $subst_values)."\n" );

$msg = "With subst values; Successfully created %1 on share %2 and device %3";
$subst_values = array( "GC1", "Public", "device", "20" );
echo( "Output ".formatMessage($msg, $subst_values)."\n" );

$msg = "With subst values; Successfully created %1 on share %2 and device %3";
$subst_values = array( "GC1", "Public" );
echo( "Output ".formatMessage($msg, $subst_values)."\n" );
*/

?>