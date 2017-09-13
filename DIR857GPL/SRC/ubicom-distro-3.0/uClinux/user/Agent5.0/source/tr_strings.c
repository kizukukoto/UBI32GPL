/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file strings.c
 *
 */
#include <string.h>
#include <ctype.h>

#include "tr.h"
#include "tr_strings.h"
#include "war_string.h"

char *skip_blanks(const char *str)
{
	while (*str && ((unsigned char) *str) < 33)
		str++;

	return (char *)str;
}


char *skip_non_blanks(const char *str)
{
	while (*str && ((unsigned char) *str) >= 33)
		str++;

	return (char *)str;
}

char *trim_blanks(char *str)
{
	char *work = str;

	if (work) {
		work += strlen(work) - 1;
		while ((work >= str) && ((unsigned char) *work) < 33)
			*(work--) = '\0';
	}

	return str;
}

int string_is_digits(const char *str)
{
	if(*str == '+' || *str == '-')
		str++;

	while(*str) {
		if(*str < '0' || *str > '9')
			return 0;
		str++;
	}

	return 1;
}

int string2boolean(const char *str)
{
	int res = BOOLEAN_ERROR;
	if(str) {
		if(war_strcasecmp(str, "true") == 0 || war_strcasecmp(str, "1") == 0)
			res = BOOLEAN_TRUE;
		else if(war_strcasecmp(str, "false") == 0 || war_strcasecmp(str, "0") == 0)
			res = BOOLEAN_FALSE;
	}

	return res;
}
