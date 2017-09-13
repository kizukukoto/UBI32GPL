/* athreg.c - contians functions for managing Atheros register files */
/* Copyright (c) 2002 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/athreg.c#90 $, $Header: //depot/sw/src/dk/mdk/devlib/athreg.c#90 $"


#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ  
    #define EILSEQ EIO
#endif	// EILSEQ

#define __int64	long long
#define HANDLE long
typedef unsigned long DWORD;
#define Sleep	delay
#endif	// #ifdef __ATH_DJGPPDOS__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mEeprom.h"
#include "mConfig.h"
#include "mDevtbl.h"
#include "artEar.h"
#include <errno.h>
#ifndef VXWORKS
#include <search.h>
#endif

#if defined(SOC_LINUX)
#define _lfind lfind
#endif
#if defined (VXWORKS) || defined (__ATH_DJGPPDOS__)
void *_lfind(const void *key,const void *base0,size_t *nmemb,size_t size,int (* compar)(const void *,const void *));
#endif

static A_BOOL getUnsignedFromStr( A_UINT32 devNum, A_CHAR *inString, A_BOOL signedValue, A_UINT16 fieldSize, A_UINT32 *returnValue);
static A_BOOL getBitFieldValues( A_UINT32 devNum, A_CHAR  *pString, ATHEROS_REG_FILE *pFieldDetails);
static A_UINT16 createRfPciValues(const ATHEROS_REG_FILE *pRfFields, A_UINT32 indexRfField, A_UINT32 sizeRegArray, PCI_REG_VALUES *pRfWriteValues);
static int compareFields(const void *arg1, const void *arg2);
static A_UINT16 ParseRfRegs(A_UINT32 devNum, ATHEROS_REG_FILE *pRfFields, A_UINT32 indexRfField, A_UINT32         sizeRegArray,
 PCI_REG_VALUES *pRfWriteValues, RF_REG_INFO	*pRfRegBanks);
static A_UINT16 createRfBankPciValues(A_UINT32 devNum, RF_REG_INFO	*pRfRegBank);
static A_UINT16 new_createRfPciValues(A_UINT32 devNum, A_UINT32 bank, A_BOOL writePCI);
static void writeRfBank(A_UINT32 devNum, RF_REG_INFO *pRfRegBank);


// non-ansi functions are mapped to some equivalent functions
#ifdef LINUX
#include <ctype.h>
#include "linux_ansi.h"
#endif

static A_BOOL  keepAGCDisable = 0;

A_UINT32 reverseRfBits(A_UINT32 val, A_UINT16 bit_count, A_BOOL dontReverse);

char delimiters[]   = " \t\n";


/** 
 * - Function Name: athregCheckField
 * - Description  : check the particular field name for a match
 * - Arguments
 *     - 
 * - Returns      : 1 if field name matches, 0 otherwise
 *******************************************************************************/
A_UINT32 athregCheckField( A_UINT32 devNum, A_CHAR* fieldname ) {
    char* token;
    LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];

    token     = strtok(fieldname, delimiters);
    if( !isMerlin(pLibDev->swDevID) ) {
        if( (strncmp(token,"bb_ch2_",7) == 0) || 
            (strncmp(token, "bb_chn2_",8) == 0) ) 
        {
            return(1); /* these register field names do not exist in merlin */
        }
    }
    return(0);
}
/* athregCheckField */

A_INT8 start_strcmp(A_CHAR *substring, A_CHAR *string) {

	A_UINT32 iIndex, ssLen, sLen;
	ssLen = strlen(substring);
	sLen = strlen(string);

	if (sLen < ssLen) return 1;

	for(iIndex=0;iIndex<ssLen;iIndex++) {
		if (tolower(string[iIndex]) == tolower(substring[iIndex]))
			continue;
		else 
			return 1;
	}
	return 0;

}

/**************************************************************************
* parseAtherosRegFile - Parse the register file to produce C array
*
* Returns: TRUE if file parsed successfully, FALSE otherwise.
*/
A_BOOL
parseAtherosRegFile(
 LIB_DEV_INFO* pLibDev,
 char*         filename
)
{
    FILE        *fileStream;
    A_CHAR      lineBuffer[MAX_FILE_WIDTH];
    A_CHAR      fieldBaseValueString[MAX_FILE_WIDTH];
    A_CHAR      fieldTurboValueString[MAX_FILE_WIDTH];
    A_CHAR      *token;
    A_UINT32    currentLineNumber = 0;
    A_BOOL      returnValue = 1;
    A_BOOL      fileError = 0;
    A_UINT32    signedValue;
    A_UINT16    i, j;
    A_UINT32    tempValue;
    ATHEROS_REG_FILE    fieldInfoIn;
    ATHEROS_REG_FILE    *pFieldDetails = NULL;
    MODE_INFO    modeFieldInfoIn;
    MODE_INFO    *pModeFieldDetails = NULL;
    size_t     tempSize;
	A_BOOL		modeSpecific = 0;
	A_UINT32    tempSwField;
	A_BOOL		tempModeField;
	A_UINT32	cfgVersion = 0;
    static int printOnce = 0;

#ifndef MDK_AP    
    if(*filename == '\0') {
        //user has chosen to go with default values
#ifndef NO_LIB_PRINT
        if( !printOnce ) {
            printf("Using defaults from : %s\n", ar5kInitData[pLibDev->ar5kInitIndex].pCfgFileVersion);
            printOnce = 1;
        }
#endif
        return 1;
    }
    else {
        printf("Runtime parsing cfg file: \n\t%s\n\n", filename);
    }
// file processing removed till flash filesystem is up
    //open file     
    fileStream = fopen(filename, "r");
    if( NULL == fileStream ) {
        mError(pLibDev->devNum, EINVAL, "Unable to open atheros text file - %s\n", filename);
        return 0;
    }
#ifndef NO_LIB_PRINT		
    printf("Using the values from configuration file %s\n\n", filename);
#endif

    memset(&fieldInfoIn, 0, sizeof(ATHEROS_REG_FILE));
    memset(&modeFieldInfoIn, 0, sizeof(MODE_INFO));


    //start reading lines and parsing info
    while( !fileError ) {
        if( fgets( lineBuffer, MAX_FILE_WIDTH, fileStream ) == NULL ) {
            break;
        }

        currentLineNumber++;
        //check for this being a comment line
        if( lineBuffer[0] == '#' ) {
            //comment line
            continue;
        }

        //extract value from the line
        token = strtok( lineBuffer, delimiters );
        if( NULL == token ) {
            //blank line
            continue;
        }
        //process command lines
        if( token[0] == '@' ) {
            if( strnicmp("@FORMAT:", token, strlen("@FORMAT:")) == 0 ) {
                token = strtok( NULL, delimiters ); //get FORMAT number
                if( NULL == token ) {
                    printf("Bad @FORMAT syntax at line %d\n", currentLineNumber);
                    fclose(fileStream);
                    return(0);
                }

                if( !sscanf(token, "%ld", &cfgVersion) ) {
                    printf("Bad @FORMAT syntax at line %d\n", currentLineNumber);
                    fclose(fileStream);
                    return(0);
                }

            }
            else if( strnicmp("@MODE:", token, strlen("@MODE:")) == 0 ) {
                token = strtok( NULL, delimiters ); //get MODE type
                if( NULL == token ) {
                    printf("Bad @MODE syntax at line %d\n", currentLineNumber);
                    fclose(fileStream);
                    return(0);
                }

                /*
                 * for now only care about finding the mode specific section, 
                 * ignore everything else
                 */
                if( strnicmp("MODE_SPECIFIC", token, strlen("MODE_SPECIFIC")) == 0 ) {
                    modeSpecific = 1;
                }

            }

            //continue and get next line
            continue;

        }

        if( token[0] == '*' ) {
            //set flag to create section in mode specific section, point past *
            token++;
        }

        for( i = FIELD_NAME; (i <= PUBLIC_NAME) && !fileError && !modeSpecific; i++ ) {

            switch( i ) {
                case FIELD_NAME:
                    //zero out the local structure at start or new field
                    memset(&fieldInfoIn, 0, sizeof(ATHEROS_REG_FILE));
                    if( strlen(token) > MAX_NAME_LENGTH ) {
                        mError(pLibDev->devNum, EINVAL, "field name on line is more than %d characters\n", 
                               currentLineNumber, MAX_NAME_LENGTH);
                        fileError = 1;
                        break;
                    }

                    //search for this field within default array
                    tempSize = pLibDev->sizeRegArray;
                    pFieldDetails = (ATHEROS_REG_FILE *)_lfind(token, pLibDev->regArray, &tempSize, sizeof(ATHEROS_REG_FILE), compareFields);

                    if( pFieldDetails == NULL ) {
                        mError(pLibDev->devNum, EINVAL, "%s fieldName at line number %d not found\n", 
                               token, currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    strcpy(fieldInfoIn.fieldName, token);
                    
                    if( strncmp((char *)strlwr(token), "rf", 2) == 0 ) {
                        //this is a radio register, needs special consideration later
                        fieldInfoIn.radioRegister = 1;
                    }
                    else {
                        fieldInfoIn.radioRegister = 0;
                    }
                    break;

                case BASE_VALUE:
                    //store off the field value till later to see whether it is signed or not
                    strcpy(fieldBaseValueString, token);
                    break;

                case TURBO_VALUE:
                    if( 0 == cfgVersion ) {
                        //store off the field value till later to see whether it is signed or not
                        strcpy(fieldTurboValueString, token);
                        break;
                    }
                    else {
                        //if this is version 2 cfg file, there is no turbo, 
                        //so increment i and fall through to next column, hence no break.
                        i++;
                    }

                case REG_NAME:
                    if( strlen(token) > MAX_NAME_LENGTH ) {
                        mError(pLibDev->devNum, EINVAL, "register name on line is more than %d characters\n", 
                               currentLineNumber, MAX_NAME_LENGTH);
                        printf("err C : %d > %d\n", strlen(token), MAX_NAME_LENGTH);
                        fileError = 1;
                        break;
                    }
                    strcpy(fieldInfoIn.regName, token);
                    break;

                case REG_OFFSET:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &fieldInfoIn.regOffset) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with reg offset value on line %d\n", currentLineNumber);
                        printf("err D \n");
                        fileError = 1;
                        break;
                    }
                    break;

                case BIT_RANGE:
                    if( !getBitFieldValues(pLibDev->devNum, token, &fieldInfoIn) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with bit range field on line %d\n", currentLineNumber);
                        printf("err E \n");
                        fileError = 1;
                        break;
                    }

                    //create and add maxValue/mask
                    //printf("fieldname: %s\n", fieldInfoIn.fieldName);
                    fieldInfoIn.maxValue = 0;
                    for( j = 0; j < fieldInfoIn.fieldSize; j++ ) {
                        fieldInfoIn.maxValue |= (0x1 << j);
                    }
                    //printf("fieldsize     : 0x%08lx\n",fieldInfoIn.fieldSize);
                    //printf("fieldmaxValue : 0x%08lx\n",fieldInfoIn.maxValue);

                    break;

                case VALUE_SIGNED:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &signedValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with value is signed flag on line %d\n", currentLineNumber);
                        printf("err F \n");
                        fileError = 1;
                        break;
                    }

                    if( (signedValue != 0) && (signedValue != 1) && !fieldInfoIn.radioRegister ) {
                        mError(pLibDev->devNum, EINVAL, "value of signed flag should be 0 or 1 on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    fieldInfoIn.valueSigned = (A_BOOL)signedValue;

                    if( fieldInfoIn.radioRegister ) {
                        if( signedValue > 5 ) {
                            mError(pLibDev->devNum, EINVAL, "bad rf register number on line %d\n", currentLineNumber);
                            fileError = 1;
                            break;
                        }
                        fieldInfoIn.rfRegNumber = (A_UCHAR)signedValue;
                    }

                    //can now do the conversions for base and turbo values
                    if( !getUnsignedFromStr(pLibDev->devNum, fieldBaseValueString, (A_BOOL)signedValue, 
                                            fieldInfoIn.fieldSize,
                                            &fieldInfoIn.fieldBaseValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with base value on line %d\n", currentLineNumber);
                        fileError = 1;
                    }

                    if( fieldInfoIn.fieldBaseValue > fieldInfoIn.maxValue ) {
                        mError(pLibDev->devNum, EINVAL, 
                               "base value is too large for field size on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    if( 0 == cfgVersion ) {
                        if( strcmp((char *)strlwr(fieldTurboValueString), "nc") != 0 ) {
                            if( !getUnsignedFromStr(pLibDev->devNum, fieldTurboValueString, (A_BOOL)signedValue, 
                                                    fieldInfoIn.fieldSize,
                                                    &fieldInfoIn.fieldTurboValue) ) {
                                mError(pLibDev->devNum, EINVAL, "problem with turbo value on line %d\n", currentLineNumber);
                                fileError = 1;
                                break;
                            }
                        }
                        else {
                            //turbo value is the same as the base value
                            fieldInfoIn.fieldTurboValue = fieldInfoIn.fieldBaseValue;
                        }

                        if( fieldInfoIn.fieldTurboValue > fieldInfoIn.maxValue ) {
                            mError(pLibDev->devNum, EINVAL, 
                                   "turbo value is too large for field size on line %d\n", currentLineNumber);
                            fileError = 1;
                            break;
                        }
                    }
                    else {
                        //Turbo value is not coming from this array, it will come from mode specific
                        fieldInfoIn.fieldTurboValue = 0;
                    }
                    break;

                case REG_WRITABLE:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &tempValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with writable flag on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    if( (tempValue != 0) && (tempValue != 1) ) {
                        mError(pLibDev->devNum, EINVAL, "writable flag should be 0 or 1 on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }
                    fieldInfoIn.writable = (A_BOOL)tempValue;
                    break;

                case REG_READABLE:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &tempValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with readable flag on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    if( (tempValue != 0) && (tempValue != 1) ) {
                        mError(pLibDev->devNum, EINVAL, "readable flag should be 0 or on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    fieldInfoIn.readable = (A_BOOL)tempValue;
                    break;

                case SW_CONTROLLED:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &tempValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with software controlled flag on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    if( (tempValue < 0) || (tempValue > 11) ) {
                        mError(pLibDev->devNum, EINVAL, "software controlled flag should be  between 0 and 9 on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }
                    fieldInfoIn.softwareControlled = (A_BOOL)tempValue;
                    break;

                case EEPROM_VALUE:
                    if( cfgVersion == 0 ) {
                        if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &tempValue) ) {
                            mError(pLibDev->devNum, EINVAL, "problem with exists in eeprom flag on line %d\n", currentLineNumber);
                            fileError = 1;
                            break;
                        }

                        if( (tempValue != 0) && (tempValue != 1) ) {
                            mError(pLibDev->devNum, EINVAL, "exists in eeprom flag should be 0 or 1 on line %d\n", currentLineNumber);
                            fileError = 1;
                            break;
                        }
                        fieldInfoIn.existsInEepromOrMode = (A_BOOL)tempValue;
                        break;
                    }
                    else {
                        //this column does not exist in the version 2 file, so increment i
                        //and fall through to next line, hence no break.
                        i++;
                    }

                case PUBLIC_NAME:
                    if( !getUnsignedFromStr(pLibDev->devNum, token, 0, 0, &tempValue) ) {
                        mError(pLibDev->devNum, EINVAL, "problem with public flag on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }

                    if( (tempValue != 0) && (tempValue != 1) ) {
                        mError(pLibDev->devNum, EINVAL, "public flag should be 0 or 1 on line %d\n", currentLineNumber);
                        fileError = 1;
                        break;
                    }
                    fieldInfoIn.publicText = (A_BOOL)tempValue;
                    break;

            }


            //get the next token
            if( !fileError && (i != PUBLIC_NAME) ) {
                token = strtok( NULL, delimiters );
                if( NULL == token ) {
                    //bad line give an error and get out
                    mError(pLibDev->devNum, EINVAL, "Incomplete line at line number %d\n", currentLineNumber);
                    fileError = 1;
                }
            }
        }

        if( modeSpecific ) {
            //parse mode specific section
            for( i = FIELD_NAME; (i <= BASE_11G_VALUE) && !fileError; i++ ) {

                switch( i ) {
                    case FIELD_NAME:
                        //zero out the local structure at start of new field
                        memset(&modeFieldInfoIn, 0, sizeof(MODE_INFO));
                        if( strlen(token) > MAX_NAME_LENGTH ) {
                            printf("field name on line is more than %d characters\n", 
                                   currentLineNumber, MAX_NAME_LENGTH);
                            fileError = 1;
                            break;
                        }

                        //look for this field in the main array to get its index and other info
                        //search for this field within default array
                        tempSize = pLibDev->sizeModeArray;
                        pModeFieldDetails = (MODE_INFO *)_lfind(token, pLibDev->pModeArray, &tempSize, 
                                                                sizeof(MODE_INFO), compareFields);

                        if( pModeFieldDetails == NULL ) {
                            printf("%s fieldName at line number %d, in mode specific section, not found in original mode section\n", 
                                   token, currentLineNumber);
                            fileError = 1;
                            break;
                        }

                        //get pointer to field details in main array
                        pFieldDetails = &(pLibDev->regArray[pModeFieldDetails->indexToMainArray]);

                        break;

                    case BASE_11A_VALUE:
                        if( !getUnsignedFromStr(pLibDev->devNum, token, pFieldDetails->valueSigned, 
							pFieldDetails->fieldSize, &modeFieldInfoIn.value11a))
					{
                            printf("problem with base 11a value in mode specific section on line %d\n", currentLineNumber);
                            fileError = 1;
                        }
                        break;

                    case TURBO_11A_VALUE:
                        if( !getUnsignedFromStr(pLibDev->devNum, token, pFieldDetails->valueSigned, 
							pFieldDetails->fieldSize, &modeFieldInfoIn.value11aTurbo))
					{
                            printf("problem with turbo 11a value in mode specific section on line %d\n", currentLineNumber);
                            fileError = 1;
                        }
                        break;

                    case BASE_11B_VALUE:
                        if( !getUnsignedFromStr(pLibDev->devNum, token, pFieldDetails->valueSigned, 
							pFieldDetails->fieldSize, &modeFieldInfoIn.value11b))
					{
                            printf("problem with base 11b value in mode specific section on line %d\n", currentLineNumber);
                            fileError = 1;
                        }
                        break;

                    case BASE_11G_VALUE:
                        if( !getUnsignedFromStr(pLibDev->devNum, token, pFieldDetails->valueSigned, 
							pFieldDetails->fieldSize, &modeFieldInfoIn.value11g))
					{
                            printf("problem with base 11g value in mode specific section on line %d\n", currentLineNumber);
                            fileError = 1;
                        }
                        break;
                } //end switch

                //get the next token
                if( !fileError && (i != BASE_11G_VALUE) ) {
                    token = strtok( NULL, delimiters );
                    if( NULL == token ) {
                        //bad line give an error and get out
                        printf("Incomplete line at line number %d, in mode specific section\n", currentLineNumber);
                        fileError = 1;
                    }
                }
            } //end for

        }

        if( !fileError ) {
            if( !modeSpecific ) {
                //overwrite the regArray with the field info
                //incase this is a ver 2 config file, take a copy of 2 fields we don't want changed
                tempSwField = pFieldDetails->softwareControlled;
                tempModeField = pFieldDetails->existsInEepromOrMode;
                memcpy(pFieldDetails, &fieldInfoIn, sizeof(ATHEROS_REG_FILE));

                if( cfgVersion >= 2 ) {
                    //write back the 2 fields want to keep
                    pFieldDetails->softwareControlled = tempSwField;
                    pFieldDetails->existsInEepromOrMode = tempModeField;
                }
            }
            else {
                //update the info in the mode specific array
                pModeFieldDetails->value11a = modeFieldInfoIn.value11a;
                pModeFieldDetails->value11aTurbo = modeFieldInfoIn.value11aTurbo;
                pModeFieldDetails->value11b = modeFieldInfoIn.value11b;
                pModeFieldDetails->value11g = modeFieldInfoIn.value11g;
            }
        }
    } //end while !fileError

    if( fileError ) {
        returnValue = 0;
    }
    fclose(fileStream);
#endif //ifndef MDK_AP
    return(returnValue);
}
/* parseAtherosRegFile */

/**************************************************************************
* getUnsignedFromStr - Given a string that will contain a value
*       calculate the unsigned interger value.  The string can be in hex
*       or decimal format
*
* Returns: The unsigned integer extracted from the string. TRUE if converted
*          false if not
*/
A_BOOL
getUnsignedFromStr
(
 A_UINT32 devNum,
 A_CHAR     *pString,
 A_BOOL     signedValue,
 A_UINT16   fieldSize,
 A_UINT32   *pReturnValue
)
{   
    A_CHAR          *pStopString;
    A_UINT32        mask = 0x01;
    A_BOOL           negValue = 0;
    A_UCHAR         i;

    //setting the base of strtoul and strtol will work out the base of the number
    //however values that start with 0 but second char is not 'x' or 'X',
    //are treated as octal values,  adding a check so we don't
    //have this problem
    if((pString[0] == '0') && ((pString[1] == 'x') || (pString[1] == 'X'))) {
        //assume hex values are unsigned.
        *pReturnValue = strtoul(pString, (char **)&pStopString, 16);
    }
    else {
        if(signedValue) {
            if (pString[0] == '-') {
                negValue = 1;
                pString++;          //move pointer beyond the - sign
            }
        }
        
        *pReturnValue = strtoul(pString, (char **)&pStopString, 10);

        if (negValue) {
            //perform the 2's compliment to get the negative value

            //first need to work out the mask for the field
            if (fieldSize == 0) {
                mError(devNum, EINVAL, "getUnsignedFromStr: field size can't be zero for negative values\n");
                return(0);
            }
            for (i = 0; i < fieldSize; i++) {
                mask |= 0x1 << i;
            }

            *pReturnValue = ((~(*pReturnValue) & mask) + 1) & mask;
        }
    }

    if(*pStopString != '\0') {
        //there was an error in the conversion, return error
        mError(devNum, EINVAL, "unable to convert to unsigned integer\n");
        return(0);
    }
    
    return(1);
}


/**************************************************************************
* getBitFieldValues - Given a string that contains the field information
*       in the format msb:lsb, extract the starting bit position and the
*       size of the field
*
* Returns: The start bit pos and size extracted from the string. TRUE if extracted
*          false if not
*/
A_BOOL
getBitFieldValues
(
 A_UINT32 devNum,
 A_CHAR  *pString,
 ATHEROS_REG_FILE *pFieldDetails
)
{
    A_CHAR  *pEndField = NULL;
    A_CHAR  *pStopString;
    A_UINT32 lengthString = sizeof(pString);
    A_UINT32 i;
    A_BOOL   colonDetected = 0;
    A_UINT16 firstValue;
    A_UINT16 secondValue;
	A_UINT16 temp;
 
    //look for the :
    for (i = 0; i < lengthString; i++) {
        if (pString[i] == ':') {
            pString[i] = '\0';
            pEndField = pString + i + 1;
            colonDetected = 1;
            break;
        }
    }

    //get the start position
    firstValue = (A_UINT16)strtoul(pString, (char **)&pStopString, 10);

    if(*pStopString != '\0') {
        //there was an error in the conversion, return error
        mError(devNum, EINVAL, "unable to convert to unsigned integer\n");
        return(0);
    }

    if (!colonDetected) {
        //this is a 1 bit field
        pFieldDetails->fieldSize = 1;
        pFieldDetails->fieldStartBitPos = firstValue;
    }
    else {
        //get second value for start postion and size
        secondValue = (A_UINT16)strtoul(pEndField, (char **)&pStopString, 10);

        if(*pStopString != '\0') {
            //there was an error in the conversion, return error
            mError(devNum, EINVAL, "unable to convert to unsigned integer\n");
            return(0);
        }
        if (firstValue < secondValue) {
			if(pFieldDetails->radioRegister) {
				//flag for later
				pFieldDetails->dontReverse = 1;
				//switch the order round
				temp = firstValue;
				firstValue = secondValue;
				secondValue = temp;
			}
			else {
                mError(devNum, EINVAL, "Bit fields must be in format msb:lsb\n");
                return(0);
			}
        }
        pFieldDetails->fieldSize = 
		    (A_UINT16)((firstValue - secondValue) + 1);
		pFieldDetails->fieldStartBitPos = secondValue;
    }
    return(1);
}


/**
 * - Function Name: createPciRegWrites
 * - Description  : create a two dimensional array that will contain the pci 
 *                  register offset and the value to write to the register.
 *                  The source of this array is the register ini file that
 *                  lists all the fields for all the registers.  The register
 *                  ini file comes from the configuration file.
 *                  
 * - Arguments
 *     -      
 * - Returns      : TRUE if array was created, else false
 ******************************************************************************/
A_BOOL
createPciRegWrites( A_UINT32 devNum )
{
	LIB_DEV_INFO* pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT16      currentField = 0;
    A_UINT16      currentRegister = 0; /* tracks num writable "non-rf" regs */
    A_UINT32      baseValueToWrite;
    A_UINT32      turboValueToWrite;
    A_UINT32      addressToWrite;
    A_UINT16      numRfPciWrites;
	A_UINT32	  regStartIndex, regEndIndex, iIndex, writableRegister, includeRegister;
	
	/*
     * create the pciValues array, create it for same number
     * of entries as the regArray.  This array will be smaller
     * than this 
     */
    //create an array large enough to hold all the register fields
    if(pLibDev->pciValuesArray) {
		//free up previous allocation if there was one.
		free(pLibDev->pciValuesArray);
        pLibDev->pciValuesArray = NULL;
	}
	pLibDev->pciValuesArray = (PCI_REG_VALUES *)malloc(sizeof(PCI_REG_VALUES) * pLibDev->sizeRegArray);

    if (NULL == pLibDev->pciValuesArray) {
        mError(devNum, ENOMEM, "Device Number %d:Unable to allocate memory for pciValuesArray\n", devNum);
        return 0;
    }
    
    /*****************************************************/
    /* start off by doing the mac and baseband registers */
    /*****************************************************/
    while((currentField<pLibDev->sizeRegArray) && (!pLibDev->regArray[currentField].radioRegister)) {
        baseValueToWrite = 0;
        turboValueToWrite = 0;
		regStartIndex=currentField;

        /* this loop parses all the fields that belong in a given register */
        /* the parsing relies on all the fields being next to each other in */
        /* the register ini file, hence, in the config file */
        if ((isKiteSW(pLibDev->swDevID))  && ( pLibDev->libCfgParams.txGainType != 1 )  && ( strncmp(pLibDev->regArray[currentField].fieldName,"hig", 3) == 0 )){
            currentField++;
 		    continue;
 	    }
        do {
//			printf("currentField=%d\n", currentField);
            addressToWrite = pLibDev->regArray[currentField].regOffset;
            baseValueToWrite |= 
                pLibDev->regArray[currentField].fieldBaseValue << pLibDev->regArray[currentField].fieldStartBitPos;
            turboValueToWrite |= 
                pLibDev->regArray[currentField].fieldTurboValue << pLibDev->regArray[currentField].fieldStartBitPos;
//			printf("addressToWrite=%x:currentField=%x\n", addressToWrite, currentField);

            currentField++;
        } while((currentField<pLibDev->sizeRegArray) && addressToWrite == pLibDev->regArray[currentField].regOffset);

		regEndIndex=currentField;
		writableRegister=0;
//		printf("SNOOP::regStartIndex=%d:regEndIndex=%d\n", regStartIndex, regEndIndex);
		for(iIndex=regStartIndex;iIndex<regEndIndex;iIndex++) {
//			printf("SNOOP::writable=%d\n", pLibDev->regArray[iIndex].writable);
			if (pLibDev->regArray[iIndex].writable) {
				writableRegister=1;
				break;
			}
		}

		includeRegister = 1;
#ifdef PHOENIX
		if (  !start_strcmp("phs", pLibDev->regArray[iIndex].regName) ||
					!start_strcmp("analog", pLibDev->regArray[iIndex].regName)) {

			includeRegister = 1;
		}
		else 
			includeRegister = 0;
#endif

		if (writableRegister && includeRegister) {
			pLibDev->pciValuesArray[currentRegister].offset = addressToWrite;
			pLibDev->pciValuesArray[currentRegister].baseValue = baseValueToWrite;
			pLibDev->pciValuesArray[currentRegister].turboValue = turboValueToWrite;
			currentRegister++;     
		}
//		printf("currentRegister=%d\n", currentRegister);
    } /* end 1st while loop */

    /* Merlin does not have "rf" regs in the historical sense */
    if( !isMerlin(pLibDev->swDevID) ) {
        //now do the radio registers.
        pLibDev->pRfPciValues = &pLibDev->pciValuesArray[currentRegister];
        pLibDev->rfRegArrayIndex = currentField;
        if( !ParseRfRegs(devNum, &pLibDev->regArray[currentField], 
                         currentField, pLibDev->sizeRegArray, &pLibDev->pciValuesArray[currentRegister], pLibDev->rfBankInfo) ) {
            mError(devNum, EIO, "Device Number %d:Unable to parseRfFields\n", devNum);
            free(pLibDev->pciValuesArray);
            pLibDev->pciValuesArray = NULL;
            //		free(pLibDev->resetFieldValuesArray);
            return(0);
        }
        //    numRfPciWrites = createRfPciValues(&pLibDev->regArray[currentField], 
        //                currentField, pLibDev->sizeRegArray, &pLibDev->pciValuesArray[currentRegister]);

        numRfPciWrites = new_createRfPciValues(devNum, ALL_BANKS, 0);

#ifndef PHOENIX
        if( numRfPciWrites == 0 ) {
            mError(devNum, EIO, "Device Number %d:Error creating pci values for rf registers\n", devNum);
            free(pLibDev->pciValuesArray);
            pLibDev->pciValuesArray = NULL;
            //		free(pLibDev->resetFieldValuesArray);
            return(0);
        }
#endif
        //
        pLibDev->sizePciValuesArray = (A_UINT16)(numRfPciWrites + currentRegister);
#ifdef _DBEUG
        printf("numRfPciWrites=%d:currentRegister=%d:sizePciValues=%d\n", numRfPciWrites, currentRegister, pLibDev->sizePciValuesArray);
#endif
    }
    else { /* Merlin */
//#endif    /* #if (0) */
        pLibDev->sizePciValuesArray = currentRegister;
    } 

    /* DEBUG: print out the pciValuesArray */
/*  {                                                                               */
/*      A_UINT32 mm;                                                                */
/*      printf("PCI_REG_VALUES START...........................................\n");*/
/*      for( mm = 0; mm < pLibDev->sizePciValuesArray; mm++ ) {                     */
/*          printf("offset(0x%08lx), value(0x%08lx)\n",                             */
/*                 pLibDev->pciValuesArray[mm].offset,                              */
/*                 pLibDev->pciValuesArray[mm].baseValue);                          */
/*      }                                                                           */
/*      printf("PCI_REG_VALUES END.............................................\n");*/
/*  }                                                                               */
    return 1;
}
/* createPciRegWrites */

/**
 * - Function Name: ParseRfRegs
 * - Description  : 
 * - Arguments
 *     -      
 * - Returns      : 
 ******************************************************************************/
A_UINT16 ParseRfRegs(
 A_UINT32 devNum,
 ATHEROS_REG_FILE *pRfFields,
 A_UINT32         indexRfField,
 A_UINT32         sizeRegArray,
 PCI_REG_VALUES *pRfWriteValues,
 RF_REG_INFO	*pRfRegBanks)
{
    LIB_DEV_INFO     *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32 currentRegBank;
	A_UINT16 sizeRfReg;
    A_UINT16 bitsPerColumn;

	//loop round all remaining register fields
    while (indexRfField < sizeRegArray) {
        if((pRfFields->regOffset + 1) > NUM_RF_BANKS) {
			mError(devNum, EIO, "Num rf register banks exceeds number hard coded in software\n");
			return 0;
		}
        
        currentRegBank = pRfFields->regOffset;
		pRfRegBanks[currentRegBank].bankNum = currentRegBank;
		pRfRegBanks[currentRegBank].pRegFields = pRfFields;
		pRfRegBanks[currentRegBank].pPciValues = pRfWriteValues;
        pRfRegBanks[currentRegBank].numRegFields = 0;
		sizeRfReg = 0;
        if ((pRfFields->regOffset <= 3) && (isOwl(pLibDev->swDevID))) {
            bitsPerColumn = 5;
        } else {
            bitsPerColumn = 8;
        }

		//loop round all the fields of this register bank
		while ((indexRfField < sizeRegArray) &&
			(pRfFields->regOffset == currentRegBank)) {
			//see if the end of the register is larger than current calculation of reg size
			if((A_UINT32)(pRfFields->fieldStartBitPos - 1 + pRfFields->fieldSize) > sizeRfReg) {
				sizeRfReg = (A_UINT16)(pRfFields->fieldSize + sizeRfReg);
			}
			pRfRegBanks[currentRegBank].numRegFields++;
			pRfFields++;
			indexRfField++;
		}
		//calculate number pci registers
		pRfRegBanks[currentRegBank].numPciRegs = (A_UINT16)(sizeRfReg / bitsPerColumn); 
		if(sizeRfReg > pRfRegBanks[currentRegBank].numPciRegs * bitsPerColumn) {
			pRfRegBanks[currentRegBank].numPciRegs++;
		}
		pRfWriteValues += pRfRegBanks[currentRegBank].numPciRegs;
	}

	return 1;
} 

A_UINT16
createRfBankPciValues
(
 A_UINT32 devNum,
 RF_REG_INFO    *pRfRegBank
)
{
   	LIB_DEV_INFO     *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *pRfFields;
    PCI_REG_VALUES   *pRfWriteValues;
    A_UCHAR          currentRfColumn;
    A_UINT16         numRegBits;
    A_UCHAR          baseByteValue;
    A_UINT32         tempBaseValue = 0;
    A_UCHAR          turboByteValue;
    A_UINT32         tempTurboValue = 0;
    A_UINT16         i, j;
    A_UINT32         x = 0;
    A_UINT32         pciAddress;
    A_BOOL           load, addr6Mode;
    A_UINT16         bitsPerColumn;
    A_BOOL           fieldLeftOver = 0;
    A_UINT16         tempValueSize = 0;
    A_UCHAR          mask, fullBitMask;

    pRfFields = pRfRegBank->pRegFields;
    if(pRfFields == NULL) {
        return 1;
    }
    pRfWriteValues = pRfRegBank->pPciValues;
    /* loop round all the fields of this register bank */

    while( x < pRfRegBank->numRegFields ) {
        currentRfColumn = pRfFields->rfRegNumber;
        numRegBits = 0;
        if ((pRfFields->regOffset <= 3) && (isOwl(pLibDev->swDevID))) {
            addr6Mode = 1;
            bitsPerColumn = 5;
            fullBitMask = 0x1f;
        } else {
            addr6Mode = 0;
            bitsPerColumn = 8;
            fullBitMask = 0xff;
        }

        /* loop round all the columns inside this bank */
        while ((x < pRfRegBank->numRegFields ) &&
                (pRfFields->rfRegNumber == currentRfColumn )) {
            baseByteValue = 0;
            turboByteValue = 0;
            load = 0;
            //create 8 bit values to be written on Pci bus
            for(i = 0; i < bitsPerColumn; ) {
                /* check for any fields left over from last time */
                if (fieldLeftOver) {
                    //write the rest of the bit field 
                    if (tempValueSize > bitsPerColumn) {
                        baseByteValue = (A_UCHAR)(tempBaseValue & fullBitMask);
                        turboByteValue = (A_UCHAR)(tempTurboValue & fullBitMask);
                        tempBaseValue = tempBaseValue >> bitsPerColumn;
                        tempTurboValue = tempTurboValue >> bitsPerColumn;
                        fieldLeftOver = 1;
                        tempValueSize = (A_UINT16)(tempValueSize - bitsPerColumn);
                        i = (A_UINT16)(i + bitsPerColumn);
                    } else { 
                        baseByteValue |= tempBaseValue;
                        turboByteValue |= tempTurboValue;
                        i = (A_UINT16)(i + tempValueSize);

                        fieldLeftOver = 0;
                        pRfFields++; 
                        x++;
                    }
                } else if((pRfFields->fieldStartBitPos - 1) > (i + numRegBits)) {
                    /* Dont have anything to write, write 0 */
                    i++;
                } else {
                    if (i + pRfFields->fieldSize > bitsPerColumn) {
                        //note code is not setup to handle a field size > 32
                        if (pRfFields->fieldSize > 32) {
                            mError(devNum, EINVAL, "Rf field size is greater than 32, not currently supported by software\n");
                            return(0);
                        }
                        
                        /* create a mask for the number of bits that will fit */
                        mask = 0;
                        for (j = 0; j < (bitsPerColumn - i); j++) {
                            mask |= (0x1 << j);
                        }
                        tempBaseValue = reverseRfBits(pRfFields->fieldBaseValue, pRfFields->fieldSize, pRfFields->dontReverse);
                        tempTurboValue = reverseRfBits(pRfFields->fieldTurboValue, pRfFields->fieldSize, pRfFields->dontReverse);
                        baseByteValue |= (tempBaseValue & mask) << i;
                        turboByteValue |= (tempTurboValue & mask) << i;
                        fieldLeftOver = 1;
                        tempBaseValue = tempBaseValue >> (bitsPerColumn - i);
                        tempTurboValue = tempTurboValue >> (bitsPerColumn - i);
                        tempValueSize = (A_UCHAR)(pRfFields->fieldSize - (bitsPerColumn - i));
                        i = (A_UCHAR)(i + (bitsPerColumn - i));
                    } else {

                        baseByteValue |= (reverseRfBits(pRfFields->fieldBaseValue, pRfFields->fieldSize, pRfFields->dontReverse) << i);
                        turboByteValue |= (reverseRfBits(pRfFields->fieldTurboValue, pRfFields->fieldSize, pRfFields->dontReverse) << i);

                        //check for the field going over 8 bits
                        i = (A_UCHAR)(i + pRfFields->fieldSize);
                        pRfFields++;
                        x++;
                    }
                }

                if( (x >= pRfRegBank->numRegFields)||(pRfFields->rfRegNumber != currentRfColumn) ||
                    (pRfFields->regOffset != pRfRegBank->bankNum)) {
                    load = 1;
                    break;
                }
            }

            numRegBits = (A_UINT16)(numRegBits + i);
            /* should now have a full column bit group to write */
            /* calculate the pci address to write to */
            pciAddress = (((i - 1) + (load << 4) + (addr6Mode << 3)) << 2) + 0x9880;

            if(currentRfColumn == 0) {
                /* first register of bank so write address to pci array */
                pRfWriteValues->offset = pciAddress;
                pRfWriteValues->baseValue = 0;
                pRfWriteValues->turboValue = 0;
            } else {
                //we should be at the correct address, do a check
                if (pciAddress != pRfWriteValues->offset) {
                    mError(devNum, EIO, "createRfBankPciValues: unexpected internal software problem, bank %d, field %s\n", pRfRegBank->bankNum, pRfFields->fieldName);
                    return(0);
                }
            }

            /* write the byte value into array */
            pRfWriteValues->baseValue |= (baseByteValue << (currentRfColumn * bitsPerColumn));
            pRfWriteValues->turboValue |= (turboByteValue << (currentRfColumn * bitsPerColumn));
            pRfWriteValues++;
        } //end while same register

        //moved onto a new rf register,  move
        //pRfWriteValues back to the start of the writes for this bank
        pRfWriteValues = pRfRegBank->pPciValues;
    }
    return 1;
}

A_UINT16
new_createRfPciValues
(
 A_UINT32 devNum,
 A_UINT32	  bank,
 A_BOOL		  writePCI
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	A_UINT32 numPciWrites = 0;
	A_UINT32 bankIndex = 0;
	A_BOOL earHere = FALSE;
	A_UINT32 modifier;
	
	//mask off any bits set in error
//	bank = bank & 0x1ff;
	bank = bank & 0xff;
	while (bank) {
		if(bank & 0x01) {
			if(!createRfBankPciValues(devNum, &pLibDev->rfBankInfo[bankIndex])) {
				mError(devNum, EIO, "Device Number %d:new_createRfPciValues: unable to create writes for bank %d\n", devNum, bank);
				return 0;
			}
			if (pLibDev->eePromLoad && pLibDev->eepData.eepromChecked && writePCI) {
				if(!isDragon(devNum) && !isOwl(pLibDev->swDevID)) {
    				if(pLibDev->p16kEepHeader->majorVersion >= 4) {
	    				earHere = ar5212IsEarEngaged(devNum, pLibDev->pEarHead, pLibDev->freqForResetDevice);
		    		}
			    	if (earHere) {
				    	ar5212EarModify(devNum, pLibDev->pEarHead, EAR_LC_RF_WRITE, pLibDev->freqForResetDevice, &modifier);
				    }
				}

			}
			numPciWrites += pLibDev->rfBankInfo[bankIndex].numPciRegs;
			if(writePCI) {
				writeRfBank(devNum, &pLibDev->rfBankInfo[bankIndex]);			
			}
		}
		bankIndex++;
		bank = bank >> 1;
	}
	
	return (A_UINT16)numPciWrites;
}

void
writeRfBank
(
 A_UINT32		devNum,
 RF_REG_INFO	*pRfRegBank
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    PCI_REG_VALUES *pRfWriteValues = pRfRegBank->pPciValues;
	A_UINT32		i;
	A_UINT32		regValue;
	A_UINT32		cfgVersion;

    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;
	for(i = 0; i < pRfRegBank->numPciRegs; i++) {
		if((pLibDev->turbo == TURBO_ENABLE) && (cfgVersion == 0)) {
			regValue = pRfWriteValues->turboValue;
		}
		else {
			regValue = pRfWriteValues->baseValue;
		}
		if (pLibDev->devMap.remoteLib) {
			pciValues[i].offset = pRfWriteValues->offset;
			pciValues[i].baseValue = regValue;
		}
		else {
	 	REGW(devNum, pRfWriteValues->offset, regValue);
		}
		pRfWriteValues++;
	}
		if (pLibDev->devMap.remoteLib) {
			sendPciWrites(devNum, pciValues, pRfRegBank->numPciRegs);
		}

	return;
}


/* get the value held by the software array (ie does not read hardware) */
MANLIB_API void getField
(
 A_UINT32 devNum,
 A_CHAR   *fieldName,
 A_UINT32 *baseValue,		//base value out
 A_UINT32 *turboValue		//turbo value out
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t		     tempSize;
	A_UINT32		 cfgVersion;
	
    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;
	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:getField\n", devNum);
		return;
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before getField \n", devNum);
        return;
    }
    
    //search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
    fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);



    if(fieldDetails == NULL) {
        mError(devNum, EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
        return;
    }

	*baseValue = fieldDetails->fieldBaseValue;
	if(cfgVersion == 0) {
		*turboValue = fieldDetails->fieldTurboValue;
	}
	else {
		*turboValue = fieldDetails->fieldBaseValue;
	}

	return;
}


MANLIB_API void changeField
(
 A_UINT32 devNum,
 A_CHAR *fieldName, 
 A_UINT32 newValue
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t     tempSize;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:changeField\n", devNum);
		return;
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before changeField \n", devNum);
        return;
    }

    /* For merlin, check if field exists */
    if( isMerlin(pLibDev->swDevID) ) {
        if( athregCheckField(devNum,fieldName) ) {
            return; /* field does not exist, quit */
        }
    }

    //search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
	fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);

    if(fieldDetails == NULL) {
        mError(devNum, EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
        return;
    }
    

	updateField(devNum, fieldDetails, newValue, 0);
	return;
}    
	
	
/**
 * - Function Name: updateField
 * - Description  : this is a general purpose routine for updating fields
 *                  whether modal or not.  It will update the base value of a
 *                  field in the reg array and in the modal array if it exists
 *                  there. Because the routine is so general, it tends to
 *                  overwrite whatever is currently in the arrays whether it's
 *                  needed or not. This array also updates the
 *                  pciValuesArray[].  It updates the first occurrence of the
 *                  register offset that it finds.
 * - Arguments 
 *     - devNum: device number
 *     - fieldDetails: ptr to the field struct, (in the register array), that
 *       is being updated
 *     - newValue: new base value of the field
 *     - immediate: 1: if value is to be written to the register directly, 0
 *       if value is to be used to update the pciArray
 * - Returns      : 
 ******************************************************************************/
void updateField(
 A_UINT32 devNum,
 ATHEROS_REG_FILE *fieldDetails,
 A_UINT32 newValue,
 A_BOOL immediate)
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    A_UINT32         i, j; 
    A_UINT32         clearMask;
    A_UINT32         *pValue = NULL;
    A_UINT32         cfgVersion;
    A_UINT32         modeIndex;
    A_UCHAR          bankMask;
    A_UINT32         regValue;

    //check that the value if not too big for the field size
    //note can't do this check if its a signed value, will just have
    //to crop the value to its field size
    if((!fieldDetails->valueSigned) && (newValue > fieldDetails->maxValue)) {
        mError(devNum, EINVAL, "Device Number %d:updateField: field %s value [%d] is too large for field size [max=%d]\n", devNum, 
                fieldDetails->fieldName, newValue, fieldDetails->maxValue);
        return;
    }
    
    if (fieldDetails->valueSigned) {
        newValue  = newValue & fieldDetails->maxValue;
    }
    
    //change the value in reg array
    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;
    if((pLibDev->turbo == TURBO_ENABLE) && (cfgVersion == 0)) {
        fieldDetails->fieldTurboValue = newValue;
    }
    else {
        fieldDetails->fieldBaseValue = newValue;
    }

    if(cfgVersion >= 2) {
        //check to see if this is a field that exists in the mode section,
        //if so then update the mode specific value to the new value
        //note in some cases we may overwrite the mode value with the same value, thats OK
        modeIndex = fieldDetails->softwareControlled; //index to mode reg
        if(fieldDetails->existsInEepromOrMode) { //this field exists in mode section
            switch(pLibDev->mode) {
            case MODE_11A:  //11a
	            if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) {
                    pLibDev->pModeArray[modeIndex].value11aTurbo = newValue;
				}
                else if(pLibDev->turbo != TURBO_ENABLE) {    //11a base
                    pLibDev->pModeArray[modeIndex].value11a = newValue;
                }
                else {          //11a turbo
                    pLibDev->pModeArray[modeIndex].value11aTurbo = newValue;
                }
                break;

            case MODE_11G: //11g
	            if (isOwl(pLibDev->swDevID) && pLibDev->libCfgParams.ht40_enable) {
                    pLibDev->pModeArray[modeIndex].value11b = newValue;
				}
                else if((pLibDev->turbo != TURBO_ENABLE) || (cfgVersion < 3)) {
                    pLibDev->pModeArray[modeIndex].value11g = newValue;
                }
                else {
                    pLibDev->pModeArray[modeIndex].value11gTurbo = newValue;
                }
                break;

            case MODE_11B: //11b
                pLibDev->pModeArray[modeIndex].value11b = newValue;
                break;
            } //end switch
        }//end if mode
    }

    if(fieldDetails->radioRegister) {
        //regenerate values again
//      createRfPciValues(&pLibDev->regArray[pLibDev->rfRegArrayIndex], 
//                          pLibDev->rfRegArrayIndex, pLibDev->sizeRegArray, pLibDev->pRfPciValues);
        //work out the mask for this bank
        bankMask = 0x01;
        for(j = 0; j < fieldDetails->regOffset; j++) {
            bankMask = (A_UCHAR)((bankMask << 1) & 0xff);
        }

        if (immediate) {
            //also write out to the registers
            borrowAnalogAccess(devNum);
            new_createRfPciValues(devNum, bankMask, 1);
            // Re-enable AGC
            if(keepAGCDisable) {
                printf("SNOOP: Not re-enabling AGC");
            } else {
                returnAnalogAccess(devNum);
            }
        }
        else {
            new_createRfPciValues(devNum, bankMask, 0);
        }
    }
    else {
        //find address and update value
        if ((isKiteSW(pLibDev->swDevID))){ // serach for reg offset from the botttom of the Array (there might be 2 entries for the same Register; so second should get updates since the register write happens from the top of the Array)
        for (i = (pLibDev->sizePciValuesArray-1); i >=0; i--) {

            if(fieldDetails->regOffset == pLibDev->pciValuesArray[i].offset) {
                if((pLibDev->turbo == TURBO_ENABLE) && (cfgVersion == 0)) {
                    pValue = &(pLibDev->pciValuesArray[i].turboValue);
                }
                else {
                    pValue = &(pLibDev->pciValuesArray[i].baseValue);
                }
                break;
            }
        }
        }else{ // if ((isKiteSW(
        for (i = 0; i < pLibDev->sizePciValuesArray; i++) {
            if(fieldDetails->regOffset == pLibDev->pciValuesArray[i].offset) {
                if((pLibDev->turbo == TURBO_ENABLE) && (cfgVersion == 0)) {
                    pValue = &(pLibDev->pciValuesArray[i].turboValue);
                }
                else {
                    pValue = &(pLibDev->pciValuesArray[i].baseValue);
                }
                break;
            }
        }
        }
        if(NULL == pValue) {
            //don't expect to get here, means we didn't find the address of reg in pci array
            mError(devNum, EIO, "Device Number %d:updateField: Inexpected internal software error\n", devNum);
            return;
        }

        //do a modify on the value
        clearMask = ~(fieldDetails->maxValue << fieldDetails->fieldStartBitPos);
        *pValue =  *pValue & clearMask | (newValue << fieldDetails->fieldStartBitPos);

        if(immediate) {
            //also do a read modify write on the register value
            //read the register
			if(isKite1_1mac(pLibDev->macRev))
			{
				if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
				{
					REGW(devNum, 0x7870, 0x10142c14);
					mSleep(10);
				}
			}
            regValue = REGR(devNum, fieldDetails->regOffset);

            //modify the value and write back
            clearMask = ~(fieldDetails->maxValue << fieldDetails->fieldStartBitPos);
            regValue =  regValue & clearMask | (newValue << fieldDetails->fieldStartBitPos);

            REGW(devNum, fieldDetails->regOffset, regValue);
			
//if (isKiteSW(pLibDev->swDevID)) {             
//regValue = REGR(devNum, 0x7828); /* kite left over ?? */
//}
			if(isKite1_1mac(pLibDev->macRev))
			{
				if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
					REGW(devNum, 0x7870, 0x10142c00);
			}
        }
    }
    return;
}
/* updateField */

//change the register value without updating the software array
MANLIB_API void changeRegValueField
(
 A_UINT32 devNum,
 A_CHAR *fieldName, 
 A_UINT32 newValue
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t     tempSize;
    A_UINT32   clearMask;
	A_UINT32   regValue;

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before changeField \n", devNum);
        return;
    }
    
    if(!strncmp(fieldName, "rf_", sizeof("rf_")))
	{
		mError(devNum, EIO, "Device Number %d:rf registers not supported by this function\n", devNum);
		return;
	}

	//search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
    fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);

    if(fieldDetails == NULL) {
        mError(devNum, EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
        return;
    }
    
   //check that the value if not too big for the field size
	//note can't do this check if its a signed value, will just have
	//to crop the value to its field size
 	if((!fieldDetails->valueSigned) && (newValue > fieldDetails->maxValue)) {
        mError(devNum, EINVAL, "Device Number %d:changeRegValueField field %s value is too large for field size\n", devNum, 
				fieldDetails->fieldName);
        return;
    }
	
	if (fieldDetails->valueSigned) {
		newValue  = newValue & fieldDetails->maxValue;
	}
	if(isKite1_1mac(pLibDev->macRev))
	{
		if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
		{
			REGW(devNum, 0x7870, 0x10142c14);
			mSleep(10);
		}
	}
	regValue = REGR(devNum, fieldDetails->regOffset);
	if(isKite1_1mac(pLibDev->macRev))
	{
		if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
		{
			REGW(devNum, 0x7870, 0x10142c00);
		}
	}

	//modify the value and write back
	clearMask = ~(fieldDetails->maxValue << fieldDetails->fieldStartBitPos);
	regValue =  regValue & clearMask | (newValue << fieldDetails->fieldStartBitPos);

	REGW(devNum, fieldDetails->regOffset, regValue);
	return;
}

/* read the register to get the value of the field.  Does a mask and shift 
   to return only the value of the field */
MANLIB_API void readField
(
 A_UINT32	devNum,
 A_CHAR		*fieldName,
 A_UINT32	*pUnsignedValue,			//returns unsigned value
 A_INT32	*pSignedValue,				//returns signed value
 A_BOOL		*pSignedFlag				//return true if this is a signed value
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t		     tempSize;
	A_UINT32		 regValue;
	A_UINT32		 fieldValue;
	A_UINT32		 signedBit;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:readField\n", devNum);
		return;
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before readField \n", devNum);
        return;
    }
    

	//see if this is an rf register.  rf registers not supported
	if(strncmp(fieldName, "rf", 2) == 0) {
		mError(devNum, EINVAL, "Device Number %d:rf register reading is not supported\n", devNum);
		return;
	}

    //search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
    fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);

    if(fieldDetails == NULL) {
        mError(devNum, EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
        return;
    }
    
	if(!fieldDetails->readable) {
		mError(devNum, EIO, "Device Number %d:%s field is not readable\n", devNum, fieldName);
		return;
	}
	
	//read the register
	if(isKite1_1mac(pLibDev->macRev))
	{
		if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
		{
			REGW(devNum, 0x7870, 0x10142c14);
			mSleep(10);
		}
	}
	regValue = REGR(devNum, fieldDetails->regOffset);
	if(isKite1_1mac(pLibDev->macRev))
	{
		if(isKiteSW(pLibDev->swDevID) && (fieldDetails->regOffset & 0xff00) == 0x7800)
		{
			REGW(devNum, 0x7870, 0x10142c00);
		}
	}
	//mask and shift to get just the field value
	fieldValue = (regValue >> fieldDetails->fieldStartBitPos) & fieldDetails->maxValue;
	
	if (fieldDetails->valueSigned) {
		*pSignedFlag = 1;
				
		*pSignedValue = fieldValue;		//will contain signed representation of number
		*pUnsignedValue = fieldValue;	//will contain unsigned representation of number

		//see if the value is negative
		signedBit = 1 << (fieldDetails->fieldSize - 1);

		if(fieldValue & signedBit) {
			*pSignedValue = ((~(*pSignedValue) & fieldDetails->maxValue) + 1) & fieldDetails->maxValue;
			*pSignedValue *= -1;
		}
	}
	else {
		*pUnsignedValue = fieldValue;
		*pSignedFlag = 0;
	}

    return;
}

/* do a read modify write on the field */
MANLIB_API void writeField(
 A_UINT32 devNum,
 A_CHAR *fieldName, 
 A_UINT32 newValue)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t      tempSize;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:writeField\n", devNum);
		return;
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before writeField \n", devNum);
        return;
    }

    /* For merlin, check if field exists */
    if( isMerlin(pLibDev->swDevID) ) {
        if( athregCheckField(devNum,fieldName) ) {
            return; /* field does not exist, quit */
        }
    }

    //search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
	fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);

    if(fieldDetails == NULL) {
        mError(devNum, EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
        return;
    }
    
   	updateField(devNum, fieldDetails, newValue, 1);

	return;
}

void writeMultipleFieldsInRfBank
(
 A_UINT32 devNum,
 PARSE_FIELD_INFO *pFieldsToChange,
 A_UINT32 numFields,
 A_UINT32 bankIndex
)
{
    A_UINT32    i;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    size_t           tempSize;
    ATHEROS_REG_FILE *fieldDetails;
    A_UINT32        newValue;
    A_UINT32        cfgVersion;

    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;

    if (checkDevNum(devNum) == FALSE) {
        mError(devNum, EINVAL, "Device Number %d:writeField\n", devNum);
        return;
    }

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:writeMultipleFieldsInRfBank: Software has no register file values, run resetDevice before writeField \n", devNum);
        return;
    }

    tempSize = pLibDev->sizeRegArray;
    for(i = 0; i < numFields; i++) {
        fieldDetails = (ATHEROS_REG_FILE *)_lfind(pFieldsToChange[i].fieldName, pLibDev->regArray, &tempSize, 
                    sizeof(ATHEROS_REG_FILE), compareFields);

        if(fieldDetails == NULL) {
            mError(devNum, EINVAL, "Device Number %d:writeMultipleFieldsInRfBank: %s fieldName not found i = %d, numFields = %d\n", devNum, pFieldsToChange[i].fieldName, i, numFields);
            return;
        }

        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].valueString, (A_BOOL)fieldDetails->valueSigned, 
                fieldDetails->fieldSize,
                &newValue)) {
            mError(devNum, EINVAL, "Device Number %d:writeMultipleFieldsInRfBank:problem with base value on line %d\n", devNum, i);
            return;
        }

        updateField(devNum, fieldDetails, newValue, 0);
        if(cfgVersion == 0) {
            //for this case we need to change the turbo field also (mainly for AP)
            pLibDev->turbo = !(pLibDev->turbo);
            updateField(devNum, fieldDetails, newValue, 0);

            //put mode back to what it was
            pLibDev->turbo = !(pLibDev->turbo);
        }

        if(pLibDev->mdkErrno) {
            return;
        }
    }

    if(!createRfBankPciValues(devNum, &pLibDev->rfBankInfo[bankIndex])) {
        mError(devNum, EIO, "Device Number %d:writeMultipleFieldsInRfBank: unable to create writes for bank %d\n", devNum, bankIndex);
        return;
    }
    writeRfBank(devNum, &pLibDev->rfBankInfo[bankIndex]);           
}

MANLIB_API void changeMultipleFields
(
 A_UINT32		  devNum,
 PARSE_FIELD_INFO *pFieldsToChange,
 A_UINT32		  numFields
)
{
	A_UINT32	i;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    size_t		     tempSize;
    ATHEROS_REG_FILE *fieldDetails;
	A_UINT32		newValue;
	A_UINT32		cfgVersion;

    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:writeField\n", devNum);
		return;
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before writeField \n", devNum);
        return;
    }

    tempSize = pLibDev->sizeRegArray;
	for(i = 0; i < numFields; i++) {
		fieldDetails = (ATHEROS_REG_FILE *)_lfind(pFieldsToChange[i].fieldName, pLibDev->regArray, &tempSize, 
					sizeof(ATHEROS_REG_FILE), compareFields);

		if(fieldDetails == NULL) {
			mError(devNum, EINVAL, "Device Number %d:changeMultiFields: %s fieldName not found i = %d, numFields = %d\n", devNum, pFieldsToChange[i].fieldName, i, numFields);
			return;
		}


        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].valueString, (A_BOOL)fieldDetails->valueSigned, 
                fieldDetails->fieldSize,
                &newValue)) {
            mError(devNum, EINVAL, "Device Number %d:problem with base value on line %d\n", devNum, i);
			return;
        }

		updateField(devNum, fieldDetails, newValue, 0);
		if(cfgVersion == 0) {
			//for this case we need to change the turbo field also (mainly for AP)
			pLibDev->turbo = !(pLibDev->turbo);
			updateField(devNum, fieldDetails, newValue, 0);

			//put mode back to what it was
			pLibDev->turbo = !(pLibDev->turbo);
		}

		if(pLibDev->mdkErrno) {
			return;
		}
	}
}

MANLIB_API A_INT32 getFieldForMode(
 A_UINT32 devNum,
 A_CHAR   *fieldName,
 A_UINT32  mode,			//desired mode 
 A_UINT32  turbo		//Flag for base or turbo value
)
{

	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    ATHEROS_REG_FILE *fieldDetails;
    size_t	     tempSize;
	A_INT32		 tempVal=0xdeadbeef;
	A_UINT32		 modeIndex;
	A_UINT32		cfgVersion;

    cfgVersion = ar5kInitData[pLibDev->ar5kInitIndex].cfgVersion;
	
//	if ((mode != MODE_11A) && (turbo == 1))
//	{
//		mError(EINVAL, "Device Number %d:Turbo not supported for this 802.11 mode\n", devNum);
//		return(0xdeadbeef);
//	}

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "Device Number %d:getField\n", devNum);
		return(0xdeadbeef);
	}

    //check for the existance of the regArray before we start
    if(pLibDev->regArray == NULL) {
        mError(devNum, EIO, "Device Number %d:Software has no register file values, run resetDevice before getField \n", devNum);
        return(0xdeadbeef);
    }

    /* For merlin, check if field exists */
    if( isMerlin(pLibDev->swDevID) ) {
        if( athregCheckField(devNum,fieldName) ) {
            return (0); /* field does not exist, quit */
        }
    }

    //search for the field name within the register array
    tempSize = pLibDev->sizeRegArray;
    fieldDetails = (ATHEROS_REG_FILE *)_lfind(fieldName, pLibDev->regArray, &tempSize, 
                sizeof(ATHEROS_REG_FILE), compareFields);


	if(fieldDetails == NULL) {
//		mError(EINVAL, "Device Number %d:%s fieldName not found\n", devNum, fieldName);
		return(0xdeadbeef);
	}

	if(cfgVersion == 0) {
		if(mode != MODE_11A) {
			mError(devNum, EIO, "Device Number %d:Modes other than 11a are not supported for older style config files\n", devNum);
			return(0xdeadbeef);
		}
		if(turbo == TURBO_ENABLE) {
			tempVal = fieldDetails->fieldTurboValue;
		}
		else {
			tempVal = fieldDetails->fieldBaseValue;
		}
	}
	else {
		if (!(fieldDetails->existsInEepromOrMode))	
		{
			tempVal = fieldDetails->fieldBaseValue;
		} else
		{
			modeIndex = fieldDetails->softwareControlled;

			// half_speed and quarter_speed get the config values of base mode
			switch(mode) {
			case MODE_11A:	//11a
				if(turbo != TURBO_ENABLE) {	//11a base or half_speed or quarter_speed
					tempVal = pLibDev->pModeArray[modeIndex].value11a;
				}
				else {			//11a turbo
					tempVal = pLibDev->pModeArray[modeIndex].value11aTurbo;
				}
				break;

			case MODE_11G: //11g
			case MODE_11O: //ofdm@2.4
				if(turbo != TURBO_ENABLE) {	//11g base or half_speed or quarter_speed
					tempVal = pLibDev->pModeArray[modeIndex].value11g;
				}
				else {			//11a turbo
					tempVal = pLibDev->pModeArray[modeIndex].value11gTurbo;
				}
				break;

			case MODE_11B: //11b
				tempVal = pLibDev->pModeArray[modeIndex].value11b;
				break;
			} //end switch
		}
	}

	if (fieldDetails->valueSigned) {				
		//see if the value is negative
		if(tempVal & (1 << (fieldDetails->fieldSize - 1))) {
			tempVal = ((~(tempVal) & fieldDetails->maxValue) + 1) & fieldDetails->maxValue;
			tempVal *= -1;
		}
	}

	return(tempVal);
}

MANLIB_API void setKeepAGCDisable(void) { 
	keepAGCDisable = 1;
}

MANLIB_API void clearKeepAGCDisable(void) { 
	keepAGCDisable = 0;
}

MANLIB_API void changeMultipleFieldsAllModes
(
 A_UINT32		  devNum,
 PARSE_MODE_INFO *pFieldsToChange,
 A_UINT32		  numFields
)
{
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
	MODE_INFO	 *pFieldDetails;
	ATHEROS_REG_FILE *pFullFieldDetails;
	size_t	     tempSize = pLibDev->sizeModeArray;
	A_UINT32	 i, j;
	A_UINT32	newValue = 0xdeadbeef;
	A_UCHAR	    bankMask;
    A_UINT32    *pValue = NULL;
    A_UINT32         clearMask;
	
	for(i = 0; i < numFields; i++) {
		pFieldDetails = (MODE_INFO *)_lfind(pFieldsToChange[i].fieldName, pLibDev->pModeArray, &tempSize, 
					sizeof(MODE_INFO), compareFields);

		if(pFieldDetails == NULL) {
			mError(devNum, EINVAL, "Device Number %d:changeMultiFieldsAllModes: %s fieldName not found\n", devNum, pFieldsToChange[i].fieldName);
			return;
		}

		pFullFieldDetails = &(pLibDev->regArray[pFieldDetails->indexToMainArray]);
		//set all the mode values to those passed in
        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].value11aStr, (A_BOOL)pFullFieldDetails->valueSigned, 
                pFullFieldDetails->fieldSize,
                &pFieldDetails->value11a)) {
            mError(devNum, EINVAL, "Device Number %d:problem with mode specific 11a base value on line %d\n", devNum, i);
			return;
		}

        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].value11aTurboStr, (A_BOOL)pFullFieldDetails->valueSigned, 
                pFullFieldDetails->fieldSize,
                &pFieldDetails->value11aTurbo)) {
            mError(devNum, EINVAL, "Device Number %d:problem with mode specific 11a turbo value on line %d\n", devNum, i);
			return;
		}

        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].value11bStr, (A_BOOL)pFullFieldDetails->valueSigned, 
                pFullFieldDetails->fieldSize,
                &pFieldDetails->value11b)) {
            mError(devNum, EINVAL, "Device Number %d:problem with mode specific 11b value on line %d\n", devNum, i);
			return;
		}

        if (!getUnsignedFromStr(devNum, pFieldsToChange[i].value11gStr, (A_BOOL)pFullFieldDetails->valueSigned, 
                pFullFieldDetails->fieldSize,
                &pFieldDetails->value11g)) {
            mError(devNum, EINVAL, "Device Number %d:problem with mode specific 11g value on line %d\n", devNum, i);
			return;
		}

		if(pFieldsToChange[i].value11gTurboStr[0] != '\0') {
			if (!getUnsignedFromStr(devNum, pFieldsToChange[i].value11gTurboStr, (A_BOOL)pFullFieldDetails->valueSigned, 
					pFullFieldDetails->fieldSize,
					&pFieldDetails->value11gTurbo)) {
				mError(devNum, EINVAL, "Device Number %d:problem with mode specific 11g turbo value on line %d\n", devNum, i);
				return;
			}
		}

		//set the correct value in the main array based on mode
		switch(pLibDev->mode) {
		case MODE_11A:	//11a
			if(pLibDev->turbo != TURBO_ENABLE) {	//11a base
				pFullFieldDetails->fieldBaseValue = pFieldDetails->value11a;
				newValue = pFieldDetails->value11a;
			}
			else {			//11a turbo
				pFullFieldDetails->fieldBaseValue = pFieldDetails->value11aTurbo;
				newValue = pFieldDetails->value11aTurbo;
			}

			break;

		case MODE_11G: //11g
		case MODE_11O: //ofdm@2.4
			if(pLibDev->turbo != TURBO_ENABLE) {	//11g base
				pFullFieldDetails->fieldBaseValue = pFieldDetails->value11g;
				newValue = pFieldDetails->value11g;
			}
			else {
				if(pFieldsToChange[i].value11gTurboStr[0] != '\0') {
					pFullFieldDetails->fieldBaseValue = pFieldDetails->value11gTurbo;
					newValue = pFieldDetails->value11gTurbo;
				}
			}
			break;

		case MODE_11B: //11b
				pFullFieldDetails->fieldBaseValue = pFieldDetails->value11b;
				newValue = pFieldDetails->value11b;
			break;
		} //end switch
	

		//modify the value in the pci writes array
		if(pFullFieldDetails->radioRegister) {
			//work out the mask for this bank
			bankMask = 0x01;
			for(j = 0; j < pFullFieldDetails->regOffset; j++) {
				bankMask = (A_UCHAR)((bankMask << 1) & 0xff);
			}
			new_createRfPciValues(devNum, bankMask, 0);
		}
		else {
			//find address and update value
			for (j = 0; j < pLibDev->sizePciValuesArray; j++) {
				if(pFullFieldDetails->regOffset == pLibDev->pciValuesArray[j].offset) {
					pValue = &(pLibDev->pciValuesArray[j].baseValue);
					break;
				}
			}

			if(NULL == pValue) {
				//don't expect to get here, means we didn't find the address of reg in pci array
				mError(devNum, EIO, "Device Number %d:changeMultipleFieldsAllModes: Unexpected internal software error\n", devNum);
				return;
			}

			//do a modify on the value
			clearMask = ~(pFullFieldDetails->maxValue << pFullFieldDetails->fieldStartBitPos);
			*pValue =  *pValue & clearMask | (newValue << pFullFieldDetails->fieldStartBitPos);
		}
	}

}


int
compareFields
(
 const void *arg1, 
 const void *arg2 
)
{
    return(strcmp( (A_CHAR *)arg1, ((ATHEROS_REG_FILE *)arg2)->fieldName ));
//    return(strcmp( , (A_CHAR *)arg2));
}

#ifndef LIB_NO_PRINT
MANLIB_API void dumpPciRegValues
(
 A_UINT32 devNum
) 
{
    A_UINT32    regValue;
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    PCI_REG_VALUES *pPciValues;

	if (checkDevNum(devNum) == FALSE) {
		mError(devNum, EINVAL, "dumpPciRegValues\n");
		return;
	}

    //don't support the rf registers yet
    printf("Note rf register reads not currently supported\n");

    printf("  offset    value\n");
    pPciValues = pLibDev->pciValuesArray;
    while (pPciValues != pLibDev->pRfPciValues) {
#ifdef SPIRIT_AP
	    if (pPciValues->offset == 0x987c)  {
		pPciValues++;
		continue;	
	    }
#endif
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#ifdef EMULATION
	    if (pPciValues->offset == 0x9928)  {
		pPciValues++;
		continue;	
	    }
#endif
#endif
        regValue = REGR(devNum, pPciValues->offset);
        printf("  0x%04lx    0x%08lx\n", pPciValues->offset,
            regValue);
	    pPciValues++;
    }    
    return;
}

/**************************************************************************
* reverseRfBits - Reverses bit_count bits in val
*
* RETURNS: Bit reversed value
*/
A_UINT32 reverseRfBits
(
 A_UINT32 val, 
 A_UINT16 bit_count,
 A_BOOL   dontReverse
)
{
	A_UINT32	retval = 0;
	A_UINT32	bit;
	int			i;

	if(dontReverse) {
		return(val);
	}
	for (i = 0; i < bit_count; i++)
	{
		bit = (val >> i) & 1;
		retval = (retval << 1) | bit;
	}
	return retval;
}

MANLIB_API void displayPciRegWrites(A_UINT32 devNum) 
{
    A_UINT32    i;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
    
    printf("  offset    base value    turbo value\n");

    for(i = 0; i < pLibDev->sizePciValuesArray; i++) {
#ifdef SPIRIT_AP
    	if (pLibDev->pciValuesArray[i].offset == 0x987c) continue;
#endif
        printf("  0x%04lx    0x%08lx    0x%08lx\n", pLibDev->pciValuesArray[i].offset,
                pLibDev->pciValuesArray[i].baseValue, pLibDev->pciValuesArray[i].turboValue);
    }    

    return;
}
#endif //NO_LIB_PRINT

#if defined (VXWORKS) || defined (__ATH_DJGPPDOS__)
void *_lfind
    (
    const void *                 key,   /* element to match */
    const void *                 base0, /* initial element in array */
    size_t *                     nmemb, /* array to search */
    size_t                       size,  /* size of array element */
    int (* compar) (const void * ,
    const void *                 )      /* comparison function */
    )
{
	A_INT32 i;
	A_INT32 ret;
	A_CHAR *arrayPtr;
	A_INT32 noElements;

	arrayPtr = (A_CHAR *)base0;
	noElements = *nmemb;

	for (i=0;i<noElements;i++)
	{
		ret = compar((const char *)key,arrayPtr);
		if (ret == 0) return arrayPtr;
		arrayPtr += size;
	}

	return NULL;
}
#endif


void sendPciWrites(A_UINT32 devNum, PCI_VALUES *pciValues, A_UINT32 nRegs)
{
  A_UINT32 entriesPerWrite, entriesRemaining, iIndex;
  LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];
  PCI_VALUES *buf;

	  entriesPerWrite = MAX_PCI_ENTRIES_PER_CMD;
          for(iIndex=0; iIndex < nRegs; iIndex+=entriesPerWrite) {
	      if ((nRegs - iIndex) > entriesPerWrite) {
		entriesRemaining = entriesPerWrite;
	      }
	      else {
		entriesRemaining = nRegs - iIndex;
	      }
	      buf = (PCI_VALUES *) (pciValues + iIndex);
	    //  printf("TotalSize=%d:iIndex=%d:entriesRemaining=%d:buf=%x\n", pLibDev->sizePciValuesArray, iIndex, entriesRemaining, buf);
	      pLibDev->devMap.r_pciWrite(devNum, buf, entriesRemaining);
	  }
}

MANLIB_API void printRegField (A_UINT32 devNum, A_CHAR *fieldName)
{
   A_UINT32	UnsignedValue;			//returns unsigned value
   A_INT32	SignedValue;			//returns signed value
   A_BOOL	SignedFlag;				//return true if this is a signed value

   readField(devNum, fieldName, &UnsignedValue, &SignedValue, &SignedFlag);

   if (SignedFlag)
   {
      printf("%s = -0x%x\n", fieldName, SignedValue);
   }
   else
   {
      printf("%s = 0x%x\n", fieldName, UnsignedValue);
   }
}

MANLIB_API void printAntDivFields(A_UINT32 devNum)
{
	printRegField(devNum, "bb_enable_ant_div_lnadiv");
	printRegField(devNum, "bb_ant_div_alt_lnaconf");
	printRegField(devNum, "bb_ant_div_main_lnaconf");
	printRegField(devNum, "bb_ant_div_alt_gaintb");
	printRegField(devNum, "bb_ant_div_main_gaintb");
    printRegField(devNum, "bb_enable_ant_fast_div");
}

MANLIB_API void printSwitchTableFields(A_UINT32 devNum)
{
	printRegField(devNum, "bb_switch_table_com_b");
	printRegField(devNum, "bb_switch_table_b");
	printRegField(devNum, "bb_switch_table_rx12");
	printRegField(devNum, "bb_switch_table_rx1");
	printRegField(devNum, "bb_switch_table_r");
	printRegField(devNum, "bb_switch_table_t");
	printRegField(devNum, "bb_switch_table_idle");

    printRegField(devNum, "bb_switch_table_com_ra12");
	printRegField(devNum, "bb_switch_table_com_ra2l2");
	printRegField(devNum, "bb_switch_table_com_ra1l2");
	printRegField(devNum, "bb_switch_table_com_ra2l1");
	printRegField(devNum, "bb_switch_table_com_ra1l1");
	printRegField(devNum, "bb_switch_table_com_t2");
	printRegField(devNum, "bb_switch_table_com_t1");
	printRegField(devNum, "bb_switch_table_com_idle");
}
