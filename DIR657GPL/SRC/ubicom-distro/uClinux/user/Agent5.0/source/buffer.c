/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file buffer.c
 *
 * \brief Implement a dynamic buffer to hold a message(string)
 */
#include <stdlib.h>
#include <stdio.h>

#include "tr.h"
#include "buffer.h"
#include "log.h"

#include "war_string.h"

//#define BUFFER_INCREASE 512
#define BUFFER_INCREASE 1024

void destroy_buffer(struct buffer *b)
{
    if(b && b->data) {
	free(b->data);
	b->data = NULL;
	b->buf_len = 0;
	b->data_len = 0;
    }
}

void init_buffer(struct buffer *b)
{
    if(b) {
	b->buf_len = 0;
	b->data_len = 0;
	b->data = NULL;
    }
}

void trim_buffer(struct buffer *b, int len)
{
    if(b) {
	if(b->data && b->buf_len >= len && b->data_len >= len) {
	    b->data_len -= len;
	    b->data[b->data_len] = '\0';
	}
    }
}


void reset_buffer(struct buffer *b)
{
    if(b) {
	b->data_len = 0;
	if(b->data)
	    b->data[0] = '\0';
    }
}


int push_buffer(struct buffer *b, const char *fmt, ...)
{
    char *np;
    va_list ap;

    if(b->data == NULL) {
	np = malloc(BUFFER_INCREASE);
	if(np) {
	    b->data = np;
	    b->data_len = 0;
	    b->buf_len = BUFFER_INCREASE;
	    b->data[0] = '\0';
	} else {
	    tr_log(LOG_ERROR, "Out of memory!");
	    return -1;
	}
    }

    for(;;) {
	int n;

	va_start(ap, fmt);
	n = war_vsnprintf(b->data + b->data_len, b->buf_len - b->data_len, fmt, ap);
	va_end(ap);

	/* If that worked, return the string. */
	if (n > -1 && n < b->buf_len - b->data_len) {
	    b->data_len += n;
	    return 0;
	} else {
	    b->data[b->data_len] = '\0';
	}

        /*The buffer space not enough*/
	np = realloc(b->data, b->buf_len + BUFFER_INCREASE);
	if(np) {
	    b->data = np;
	    b->buf_len += BUFFER_INCREASE;
	} else {
	    tr_log(LOG_ERROR, "Out of memory!");
	    return -1;
	}
    }
}
