/*
 * options.c -- DHCP server option packet tools
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <stdlib.h>
#include <string.h>

#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "common.h"
char dns_name[64] = {0};

/* supported options are easily added here */
struct dhcp_option dhcp_options[] = {
	/* name[10]	flags					code */
	{"subnet",	OPTION_IP | OPTION_REQ,			0x01},
	{"timezone",	OPTION_S32,				0x02},
	{"router",	OPTION_IP | OPTION_LIST | OPTION_REQ,	0x03},
	{"timesvr",	OPTION_IP | OPTION_LIST,		0x04},
	{"namesvr",	OPTION_IP | OPTION_LIST,		0x05},
	{"dns",		OPTION_IP | OPTION_LIST | OPTION_REQ,	0x06},
	{"logsvr",	OPTION_IP | OPTION_LIST,		0x07},
	{"cookiesvr",	OPTION_IP | OPTION_LIST,		0x08},
	{"lprsvr",	OPTION_IP | OPTION_LIST,		0x09},
	{"hostname",	OPTION_STRING | OPTION_REQ,		0x0c},
	{"bootsize",	OPTION_U16,				0x0d},
	{"domain",	OPTION_STRING | OPTION_REQ,		0x0f},
	{"swapsvr",	OPTION_IP,				0x10},
	{"rootpath",	OPTION_STRING,				0x11},
	{"ipttl",	OPTION_U8,				0x17},
	{"mtu",		OPTION_U16,				0x1a},
	{"broadcast",	OPTION_IP | OPTION_REQ,			0x1c},
/* jimmy added 20080423 for option 33, Static Route */
#ifdef UDHCPC_STATIC_ROUTE
	{"fixroute",OPTION_IP | OPTION_REQ,			0x21},
#endif
/* ------------------------------------ */
	{"nisdomain",	OPTION_STRING | OPTION_REQ,		0x28},
	{"nissrv",	OPTION_IP | OPTION_LIST | OPTION_REQ,	0x29},
	{"ntpsrv",	OPTION_IP | OPTION_LIST | OPTION_REQ,	0x2a},
	{"wins",	OPTION_IP | OPTION_LIST | OPTION_REQ,		0x2c},
#ifdef UDHCPD_NETBIOS
	{"nodetype",	OPTION_U8 | OPTION_REQ,		0x2e},
	{"scope",	OPTION_STRING | OPTION_REQ ,		0x2f},
#endif
	{"requestip",	OPTION_IP,				0x32},
	{"lease",	OPTION_U32,				0x33},
	{"dhcptype",	OPTION_U8,				0x35},
	{"serverid",	OPTION_IP,				0x36},
	{"message",	OPTION_STRING,				0x38},
	{"tftp",	OPTION_STRING,				0x42},
	{"bootfile",	OPTION_STRING,				0x43},
/* jimmy added 20080411 for option 121 */
#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
	{"fixroute1",OPTION_IP | OPTION_REQ,			0x79},
#endif
/* Joe Huang : Add option 212 for IPv6 Spec 1.13 */
	{"6rd",		OPTION_STRING | OPTION_REQ,		0xd4},
/* ----------------------------------- */
/* jimmy added 20080423 for option 249 */
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
	{"fixroute2",OPTION_IP | OPTION_REQ,			0xF9},
#endif
/* ----------------------------------- */
	{"",		0x00,				0x00}
};

/* Lengths of the different option types */
int option_lengths[] = {
	[OPTION_IP] =		4,
	[OPTION_IP_PAIR] =	8,
	[OPTION_BOOLEAN] =	1,
	[OPTION_STRING] =	1,
	[OPTION_U8] =		1,
	[OPTION_U16] =		2,
	[OPTION_S16] =		2,
	[OPTION_U32] =		4,
	[OPTION_S32] =		4
};


/* get an option with bounds checking (warning, not aligned). */
uint8_t *get_option(struct dhcpMessage *packet, int code)
{
	int i, length;
	uint8_t *optionptr;
	int over = 0, done = 0, curr = OPTION_FIELD;

	optionptr = packet->options;
	i = 0;
/*	Date: 2009-12-17
*	Name: jimmy huang
*	Reason:	Modified option length from 308 to 408
*	Note:	
		1.	TEW-634GRU RUssia has the case, dhcpd offer both option 121 and 249, each contains 15 rules
*			And the total dhcp OFFER packet size is 601, which will cause our udhcpc can not handle the packet
*			thus we increase to 408 to let udhcpc can hanlde these kinds of packet
*		2.	We defined new definition "DHCP_OPTION_SIZE" in common.h, all related variable
*			will be replaced with this definition
*/
	//length = 308;
	length = DHCP_OPTION_SIZE;
	while (!done) {
		if (i >= length) {
			LOG(LOG_WARNING, "bogus packet, option fields too long.");
			return NULL;
		}
		if (optionptr[i + OPT_CODE] == code) {
			if (i + 1 + optionptr[i + OPT_LEN] >= length) {
				LOG(LOG_WARNING, "bogus packet, option fields too long.");
				return NULL;
			}
		/* jimmy modified 20080505 , avoid option incorrect domain */
			/* modify by ken for hoat name issue return hostname option same as domain name */
			//if (code == DHCP_DOMAIN_NAME || code == DHCP_HOST_NAME)
			if (code == DHCP_HOST_NAME)
				return optionptr + i + 1;
		/* ------------------------------------------------------- */
			else
			return optionptr + i + 2;
		}
		switch (optionptr[i + OPT_CODE]) {
		case DHCP_PADDING:
			i++;
			break;
		case DHCP_OPTION_OVER:
			if (i + 1 + optionptr[i + OPT_LEN] >= length) {
				LOG(LOG_WARNING, "bogus packet, option fields too long.");
				return NULL;
			}
			over = optionptr[i + 3];
			i += optionptr[OPT_LEN] + 2;
			break;
		case DHCP_END:
			if (curr == OPTION_FIELD && over & FILE_FIELD) {
				optionptr = packet->file;
				i = 0;
				length = 128;
				curr = FILE_FIELD;
			} else if (curr == FILE_FIELD && over & SNAME_FIELD) {
				optionptr = packet->sname;
				i = 0;
				length = 64;
				curr = SNAME_FIELD;
			} else done = 1;
			break;
		default:
			i += optionptr[OPT_LEN + i] + 2;
		}
	}
	return NULL;
}


/* return the position of the 'end' option (no bounds checking) */
int end_option(uint8_t *optionptr)
{
	int i = 0;

	while (optionptr[i] != DHCP_END) {
		if (optionptr[i] == DHCP_PADDING) i++;
		else i += optionptr[i + OPT_LEN] + 2;
	}
	return i;
}


/* add an option string to the options (an option string contains an option code,
 * length, then data) */
int add_option_string(uint8_t *optionptr, uint8_t *string)
{
	int end = end_option(optionptr);
/*	Date: 2009-12-17
*	Name: jimmy huang
*	Reason:	Modified option length from 308 to 408
*	Note:	
*		1.	TEW-634GRU RUssia has the case, dhcpd offer both option 121 and 249, each contains 15 rules
*			And the total dhcp OFFER packet size is 601, which will cause our udhcpc can not handle the packet
*			thus we increase to 408 to let udhcpc can hanlde these kinds of packet
*		2.	We defined new definition "DHCP_OPTION_SIZE" in common.h, all related variable
*			will be replaced with this definition
*/
	/* end position + string length + option code/length + end option */
	//if (end + string[OPT_LEN] + 2 + 1 >= 308) {
	if (end + string[OPT_LEN] + 2 + 1 >= DHCP_OPTION_SIZE) {
		LOG(LOG_ERR, "Option 0x%02x did not fit into the packet!", string[OPT_CODE]);
		return 0;
	}
	DEBUG(LOG_INFO, "adding option 0x%02x", string[OPT_CODE]);
	memcpy(optionptr + end, string, string[OPT_LEN] + 2);
	optionptr[end + string[OPT_LEN] + 2] = DHCP_END;
	return string[OPT_LEN] + 2;
}


/* add a one to four byte option to a packet */
int add_simple_option(uint8_t *optionptr, uint8_t code, uint32_t data)
{
	char length = 0;
	int i;
/*
*	Date: 2009-7-31
*   Name: Ken Chiang
*   Reason: modify for fixed DHCP_DOMAIN_NAME option is over size issue.
*   Notice :
*/		
	uint8_t option[2 + 64];
	//uint8_t option[2 + 4];
	uint8_t *u8;
	uint16_t *u16;
	uint32_t *u32;
	uint32_t aligned;
	u8 = (uint8_t *) &aligned;
	u16 = (uint16_t *) &aligned;
	u32 = &aligned;

	for (i = 0; dhcp_options[i].code; i++)
		if (dhcp_options[i].code == code) {
			if (code == DHCP_DOMAIN_NAME)
				length = strlen(data);	
			else
			length = option_lengths[dhcp_options[i].flags & TYPE_MASK];
		}

	if (!length) {
		DEBUG(LOG_ERR, "Could not add option 0x%02x", code);
		return 0;
	}

	option[OPT_CODE] = code;
	option[OPT_LEN] = length;

	switch (length) {
		case 1: *u8 =  data; break;
		case 2: *u16 = data; break;
		case 4: *u32 = data; break;
	}
	
	if (code == DHCP_DOMAIN_NAME)
		memcpy(option + 2, data, length);
	else
	memcpy(option + 2, &aligned, length);
		
	return add_option_string(optionptr, option);
}


/* find option 'code' in opt_list */
struct option_set *find_option(struct option_set *opt_list, char code)
{
	while (opt_list && opt_list->data[OPT_CODE] < code)
		opt_list = opt_list->next;

	if (opt_list && opt_list->data[OPT_CODE] == code) return opt_list;
	else return NULL;
}


/* add an option to the opt_list */
void attach_option(struct option_set **opt_list, struct dhcp_option *option, char *buffer, int length)
{
	struct option_set *existing, *new, **curr;

	/* add it to an existing option */
	if ((existing = find_option(*opt_list, option->code))) {
		DEBUG(LOG_INFO, "Attaching option %s to existing member of list", option->name);
		if (option->flags & OPTION_LIST) {
			if (existing->data[OPT_LEN] + length <= 255) {
				existing->data = realloc(existing->data,
						existing->data[OPT_LEN] + length + 2);
				memcpy(existing->data + existing->data[OPT_LEN] + 2, buffer, length);
				existing->data[OPT_LEN] += length;
			} /* else, ignore the data, we could put this in a second option in the future */
		} /* else, ignore the new data */
	} else {
		DEBUG(LOG_INFO, "Attaching option %s to list", option->name);

		/* make a new option */
		new = xmalloc(sizeof(struct option_set));
		new->data = xmalloc(length + 2);
		new->data[OPT_CODE] = option->code;
		new->data[OPT_LEN] = length;
		memcpy(new->data + 2, buffer, length);

		curr = opt_list;
		while (*curr && (*curr)->data[OPT_CODE] < option->code)
			curr = &(*curr)->next;

		new->next = *curr;
		*curr = new;
	}
}
