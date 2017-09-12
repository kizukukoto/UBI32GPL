<?php

class OwnerConfiguration{

    function OwnerConfiguration() {
    }
    
    function getConfig(){
        unset($ownerName);
        exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
        if($retVal !== 0) {
            return NULL;
        }

        unset($fullName);
        exec("sudo /usr/local/sbin/getUserInfo.sh $ownerName[0] 'fullname' ", $fullName, $retVal);
        if($retVal !== 0) {
            return NULL;
        }

        unset($hash);
        exec("sudo awk -v usr={$ownerName[0]} -F: '{if ($1 == usr) print $2}' /etc/shadow", $hash, $retVal);
        if($retVal !== 0) {
            return NULL;
        }

        $emptyPassword = ($hash[0] === '') ? 'true' : 'false'; 

        return( array( 
                    'owner_name' => "$ownerName[0]",
                    'fullname' => "$fullName[0]",
                    'empty_passwd' => "$emptyPassword"
                    ));
    }

    function modifyConfig($changes){
        //Require entire representation and not just a delta to ensure a consistant representation
        if( !isset($changes["owner_name"]) ||
            !isset($changes["fullname"]) ||
            !isset($changes["change_passwd"]) ||
            !isset($changes["owner_passwd"]) ){

            return 'BAD_REQUEST';
        }

        if($changes["change_passwd"] === 'true'){
            $decodedPw = base64_decode($changes["owner_passwd"], true);
            if("$decodedPw" === FALSE){
                return 'BAD_REQUEST';
            }
        }

        //Verify changes are valid
        //UI is doing input validation, but we should add tests here too so to make REST API standalone for possible future clients.
        //!!!Validate $changes["owner_name"], $changes["fullname"], $decodedPw (Only if change_passwd is true)
        if($changes["change_passwd"] !== 'true' &&
           $changes["change_passwd"] !== 'false'){

            return 'BAD_REQUEST';
        }

        //Don't allow renaming owner to an already existing user.
        unset($ownerName);
        exec("sudo /usr/local/sbin/getOwner.sh", $ownerName, $retVal);
        if($retVal !== 0) {
            return 'SERVER_ERROR';
        }

        if($ownerName[0] !== $changes["owner_name"]){
            unset($outputUsers);
            exec("sudo /usr/local/sbin/getUsers.sh", $outputUsers, $retVal);
            if($retVal !== 0) {
                return 'SERVER_ERROR';
            }

            foreach($outputUsers as $testUser){
                if($testUser === $changes["owner_name"]) {
                    return 'BAD_REQUEST';
                }
            }
        }

        //Actually do change
        //If owner_name changed ...
        if($ownerName[0] !== $changes["owner_name"]){
            unset($output);
            exec("sudo /usr/local/sbin/changeOwner.sh \"{$changes["owner_name"]}\"", $output, $retVal);
            if($retVal !== 0) {
                return 'SERVER_ERROR';
            }
        }

        //If full name changed ...
        unset($outputFullName);
        exec("sudo /usr/local/sbin/getUserInfo.sh \"{$changes["owner_name"]}\"", $outputFullName, $retVal);
        if($retVal !== 0) {
            return 'SERVER_ERROR';
        }

        if($outputFullName[0] !== $changes["fullname"]){
            unset($output);
            // exec("sudo /usr/sbin/usermod -c \"{$changes["fullname"]}\" \"{$changes["owner_name"]}\"", $output, $retVal);
            exec("sudo /usr/local/sbin/modUserPassword.sh \"{$changes["owner_name"]}\" \"{$changes["fullname"]}\" '--fullname'", $output, $retVal);
            if($retVal !== 0) {
                return 'SERVER_ERROR';
            }
        }

        //If password changed ...
        if($changes["change_passwd"] === 'true'){
            unset($output);
            //modUserPassword.sh expects one parameter in blank password case.
            if($decodedPw == ""){
                exec("sudo /usr/local/sbin/modUserPassword.sh \"{$changes["owner_name"]}\"", $output, $retVal);
            } else {
                exec("sudo /usr/local/sbin/modUserPassword.sh \"{$changes["owner_name"]}\" \"$decodedPw\"", $output, $retVal);
            }

            if($retVal !== 0) {
                return 'SERVER_ERROR';
            }
        }
        
        return 'SUCCESS';
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
