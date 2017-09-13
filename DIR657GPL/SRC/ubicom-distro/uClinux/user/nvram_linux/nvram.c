/*
 * NVRAM variable manipulation (common)
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram.c,v 1.2 2009/08/12 11:17:05 peter_pan Exp $
 */
/*
#include <typedefs.h>
#include <osl.h>
#include <sbutils.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <sbsdram.h>
*/
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "typedefs.h"
#include "nvram.h"

//#define NVRAM_DEBUG
#ifdef NVRAM_DEBUG
#define NVDBG			printk
#else		/* NVRAM_DEBUG */
#define NVDBG(fmt, a...)	do { } while(0)
#endif

#define MALLOC(osh, size)	kmalloc(size, GFP_ATOMIC)
#define MFREE(osh, addr, size)	kfree(addr)
#define bzero(addr, len)	memset(addr, 0, len)

#define      ROUNDUP(x, y)           ((((x)+((y)-1))/(y))*(y))
#define CRC8_INIT_VALUE		0xff 
#define ARRAYSIZE(a)         (sizeof(a)/sizeof(a[0]))
#define htol32(i) (i)	/* Little endian */
/*******************************************************************************
 * crc8
 *
 * Computes a crc8 over the input data using the polynomial:
 *
 *       x^8 + x^7 +x^6 + x^4 + x^2 + 1
 *
 * The caller provides the initial value (either CRC8_INIT_VALUE
 * or the previous returned value) to allow for processing of 
 * discontiguous blocks of data.  When generating the CRC the
 * caller is responsible for complementing the final return value
 * and inserting it into the byte stream.  When checking, a final
 * return value of CRC8_GOOD_VALUE indicates a valid CRC.
 *
 * Reference: Dallas Semiconductor Application Note 27
 *   Williams, Ross N., "A Painless Guide to CRC Error Detection Algorithms", 
 *     ver 3, Aug 1993, ross@guest.adelaide.edu.au, Rocksoft Pty Ltd.,
 *     ftp://ftp.rocksoft.com/clients/rocksoft/papers/crc_v3.txt
 *
 ******************************************************************************/

static uint8 crc8_table[256] = {
    0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
    0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
    0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
    0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
    0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
    0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
    0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
    0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
    0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
    0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
    0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
    0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
    0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
    0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
    0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
    0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
    0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
    0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
    0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
    0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
    0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
    0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
    0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
    0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
    0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
    0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
    0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
    0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
    0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
    0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
    0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
    0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};
extern int NVRAM_SPACE;
#define CRC_INNER_LOOP(n, c, x) \
    (c) = ((c) >> 8) ^ crc##n##_table[((c) ^ (x)) & 0xff]

uint8
hndcrc8(
	uint8 *pdata,	/* pointer to array of data to process */
	uint  nbytes,	/* number of input data bytes to process */
	uint8 crc	/* either CRC8_INIT_VALUE or previous return value */
)
{
	/* hard code the crc loop instead of using CRC_INNER_LOOP macro
	 * to avoid the undefined and unnecessary (uint8 >> 8) operation. */
	while (nbytes-- > 0)
		crc = crc8_table[(crc ^ *pdata++) & 0xff];

	return crc;
}
extern struct nvram_tuple * BCMINIT(_nvram_realloc)(struct nvram_tuple *t, const char *name, const char *value);
extern void BCMINIT(_nvram_free)(struct nvram_tuple *t);
extern int BCMINIT(_nvram_read)(void *buf);

char * BCMINIT(_nvram_get)(const char *name);
int BCMINIT(_nvram_set)(const char *name, const char *value);
int BCMINIT(_nvram_unset)(const char *name);
int BCMINIT(_nvram_getall)(char *buf, int count);
int BCMINIT(_nvram_commit)(struct nvram_header *header);
int BCMINIT(_nvram_init)(void);
void BCMINIT(_nvram_exit)(void);

static struct nvram_tuple * BCMINITDATA(nvram_hash)[257];
static struct nvram_tuple * nvram_dead;

/* Free all tuples. Should be locked. */
static void  
BCMINITFN(nvram_free)(void)
{
	uint i;
	struct nvram_tuple *t, *next;

	/* Free hash table */
	for (i = 0; i < ARRAYSIZE(BCMINIT(nvram_hash)); i++) {
		for (t = BCMINIT(nvram_hash)[i]; t; t = next) {
			next = t->next;
			BCMINIT(_nvram_free)(t);
		}
		BCMINIT(nvram_hash)[i] = NULL;
	}

	/* Free dead table */
	for (t = nvram_dead; t; t = next) {
		next = t->next;
		BCMINIT(_nvram_free)(t);
	}
	nvram_dead = NULL;

	/* Indicate to per-port code that all tuples have been freed */
	BCMINIT(_nvram_free)(NULL);
}

/* String hash */
static INLINE uint
hash(const char *s)
{
	uint hash = 0;

	while (*s)
		hash = 31 * hash + *s++;

	return hash;
}

/* (Re)initialize the hash table. Should be locked. */
static int 
BCMINITFN(nvram_rehash)(struct nvram_header *header)
{
	char buf[] = "0xXXXXXXXX", *name, *value, *end, *eq;

	/* (Re)initialize hash table */
	BCMINIT(nvram_free)();

	/* Parse and set "name=value\0 ... \0\0" */
	name = (char *) &header[1];
	end = (char *) header + NVRAM_SPACE - 2;
	end[0] = end[1] = '\0';
	for (; *name; name = value + strlen(value) + 1) {
		if (!(eq = strchr(name, '=')))
			break;
		*eq = '\0';
		value = eq + 1;
		BCMINIT(_nvram_set)(name, value);
		*eq = '=';
	}

	/* Set special SDRAM parameters */
	if (!BCMINIT(_nvram_get)("sdram_init")) {
		sprintf(buf, "0x%04X", (uint16)(header->crc_ver_init >> 16));
		BCMINIT(_nvram_set)("sdram_init", buf);
	}
	if (!BCMINIT(_nvram_get)("sdram_config")) {
		sprintf(buf, "0x%04X", (uint16)(header->config_refresh & 0xffff));
		BCMINIT(_nvram_set)("sdram_config", buf);
	}
	if (!BCMINIT(_nvram_get)("sdram_refresh")) {
		sprintf(buf, "0x%04X", (uint16)((header->config_refresh >> 16) & 0xffff));
		BCMINIT(_nvram_set)("sdram_refresh", buf);
	}
	if (!BCMINIT(_nvram_get)("sdram_ncdl")) {
		sprintf(buf, "0x%08X", header->config_ncdl);
		BCMINIT(_nvram_set)("sdram_ncdl", buf);
	}

	return 0;
}

/* Get the value of an NVRAM variable. Should be locked. */
char * 
BCMINITFN(_nvram_get)(const char *name)
{
	uint i;
	struct nvram_tuple *t;
	char *value;
	NVDBG("%s: name=%s\n",__func__,name);
	
	if (!name){
		printk("%s: name fail\n",__func__);
		return NULL;
	}	

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(BCMINIT(nvram_hash));

	NVDBG("%s: i=%x\n",__func__,i);
	/* Find the associated tuple in the hash table */
	for (t = BCMINIT(nvram_hash)[i]; t && strcmp(t->name, name); t = t->next);
	
	NVDBG("%s: i=%x\n",__func__,i);
	value = t ? t->value : NULL;
	
	NVDBG("%s: value=%s ok\n",__func__,value);
	return value;
}

/* Get the value of an NVRAM variable. Should be locked. */
int 
BCMINITFN(_nvram_set)(const char *name, const char *value)
{
	uint i;
	struct nvram_tuple *t, *u, **prev;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(BCMINIT(nvram_hash));

	/* Find the associated tuple in the hash table */
	for (prev = &BCMINIT(nvram_hash)[i], t = *prev; t && strcmp(t->name, name); prev = &t->next, t = *prev);

	/* (Re)allocate tuple */
	if (!(u = BCMINIT(_nvram_realloc)(t, name, value)))
		return -12; /* -ENOMEM */

	/* Value reallocated */
	if (t && t == u)
		return 0;

	/* Move old tuple to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}

	/* Add new tuple to the hash table */
	u->next = BCMINIT(nvram_hash)[i];
	BCMINIT(nvram_hash)[i] = u;

	return 0;
}

/* Unset the value of an NVRAM variable. Should be locked. */
int 
BCMINITFN(_nvram_unset)(const char *name)
{
	uint i;
	struct nvram_tuple *t, **prev;

	if (!name)
		return 0;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(BCMINIT(nvram_hash));

	/* Find the associated tuple in the hash table */
	for (prev = &BCMINIT(nvram_hash)[i], t = *prev; t && strcmp(t->name, name); prev = &t->next, t = *prev);

	/* Move it to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}

	return 0;
}

/* Get all NVRAM variables. Should be locked. */
int 
BCMINITFN(_nvram_getall)(char *buf, int count)
{
	uint i;
	struct nvram_tuple *t;
	int pre_len=0,len = 0;
	
	NVDBG("%s: count=%x\n",__func__,count);
	bzero(buf, count);

	/* Write name=value\0 ... \0\0 */
	for (i = 0; i < ARRAYSIZE(BCMINIT(nvram_hash)); i++) {
		for (t = BCMINIT(nvram_hash)[i]; t; t = t->next) {
			if ((count - len) > (strlen(t->name) + 1 + strlen(t->value) + 1)){
				NVDBG("%s: t->name=%s,t->value=%s\n",__func__,t->name,t->value);
				pre_len = len;
				len += sprintf(buf + len, "%s=%s", t->name, t->value) + 1;	
				NVDBG("%s: len=%d,buf=%s\n",__func__,len,buf + pre_len);			
			}	
			else
				break;
		}
	}
	NVDBG("%s: buf=%s\n",__func__,buf);
	return len;
}

/* Regenerate NVRAM. Should be locked. */
int
BCMINITFN(_nvram_commit)(struct nvram_header *header)
{
	//char *init, *config, *refresh, *ncdl;
	char *ptr, *end;
	int i;
	struct nvram_tuple *t;
	struct nvram_header tmp;
	uint8 crc;

	/* Regenerate header */
	header->magic = NVRAM_MAGIC;
	header->crc_ver_init = (NVRAM_VERSION << 8);
	/*
	if (!(init = BCMINIT(_nvram_get)("sdram_init")) ||
	    !(config = BCMINIT(_nvram_get)("sdram_config")) ||
	    !(refresh = BCMINIT(_nvram_get)("sdram_refresh")) ||
	    !(ncdl = BCMINIT(_nvram_get)("sdram_ncdl"))) {
		header->crc_ver_init |= SDRAM_INIT << 16;
		header->config_refresh = SDRAM_CONFIG;
		header->config_refresh |= SDRAM_REFRESH << 16;
		header->config_ncdl = 0;
	} else {
		header->crc_ver_init |= (bcm_strtoul(init, NULL, 0) & 0xffff) << 16;
		header->config_refresh = bcm_strtoul(config, NULL, 0) & 0xffff;
		header->config_refresh |= (bcm_strtoul(refresh, NULL, 0) & 0xffff) << 16;
		header->config_ncdl = bcm_strtoul(ncdl, NULL, 0);
	}
	*/
	/* Clear data area */
	ptr = (char *) header + sizeof(struct nvram_header);
	bzero(ptr, NVRAM_SPACE - sizeof(struct nvram_header));

	/* Leave space for a double NUL at the end */
	end = (char *) header + NVRAM_SPACE - 2;

	/* Write out all tuples */
	for (i = 0; i < ARRAYSIZE(BCMINIT(nvram_hash)); i++) {
		for (t = BCMINIT(nvram_hash)[i]; t; t = t->next) {
			if ((ptr + strlen(t->name) + 1 + strlen(t->value) + 1) > end)
				break;
			ptr += sprintf(ptr, "%s=%s", t->name, t->value) + 1;
		}
	}

	/* End with a double NUL */
	ptr += 2;

	/* Set new length */
	header->len = ROUNDUP(ptr - (char *) header, 4);

	/* Little-endian CRC8 over the last 11 bytes of the header */
	tmp.crc_ver_init = htol32(header->crc_ver_init);
	tmp.config_refresh = htol32(header->config_refresh);
	tmp.config_ncdl = htol32(header->config_ncdl);
	crc = hndcrc8((char *) &tmp + 9, sizeof(struct nvram_header) - 9, CRC8_INIT_VALUE);

	/* Continue CRC8 over data bytes */
	crc = hndcrc8((char *) &header[1], header->len - sizeof(struct nvram_header), crc);

	/* Set new CRC8 */
	header->crc_ver_init |= crc;

	/* Reinitialize hash table */
	return BCMINIT(nvram_rehash)(header);
}

/* Initialize hash table. Should be locked. */
int 
BCMINITFN(_nvram_init)(void)
{
	struct nvram_header *header;
	int ret;

	if (!(header = (struct nvram_header *) MALLOC(NULL, NVRAM_SPACE))) {
		printk("nvram_init: out of memory\n");
		return -12; /* -ENOMEM */
	}

	if ((ret = BCMINIT(_nvram_read)(header)) == 0 &&
	    header->magic == NVRAM_MAGIC)
		BCMINIT(nvram_rehash)(header);

	MFREE(NULL, header, NVRAM_SPACE);
	return ret;
}

/* Free hash table. Should be locked. */
void 
BCMINITFN(_nvram_exit)(void)
{
	BCMINIT(nvram_free)();
}
