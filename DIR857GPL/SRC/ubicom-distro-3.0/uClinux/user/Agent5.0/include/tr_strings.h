/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file strings.h
 *
 */
#ifndef __HEADER_STRINGS__
#define __HEADER_STRINGS__

char *skip_blanks(const char *str);
char *skip_non_blanks(const char *str);
char *trim_blanks(char *str);
int string_is_digits(const char *str);
int string2boolean(const char *str);

#endif
