/* $Id: upnpdescgen.h,v 1.18 2008/03/06 18:09:10 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPDESCGEN_H__
#define __UPNPDESCGEN_H__

#include "config.h"

/* for the root description 
 * The child list reference is stored in "data" member using the
 * INITHELPER macro with index/nchild always in the
 * same order, whatever the endianness */
struct XMLElt {
	const char * eltname;	/* begin with '/' if no child */
	const char * data;	/* Value */
};

/* for service description */
struct serviceDesc {
	const struct action * actionList;
	const struct stateVar * serviceStateTable;
};

struct action {
	const char * name;
	const struct argument * args;
};

struct argument {	/* the name of the arg is obtained from the variable */
	const char * name;
	unsigned char dir;		/* 1 = in, 2 = out */
	unsigned char relatedVar;	/* index of the related variable */
};

struct stateVar {
	const char * name;
	unsigned char itype;	/* MSB: sendEvent flag, 7 LSB: index in upnptypes */
	unsigned char idefault;	/* default value */
	unsigned char iallowedlist;	/* index in allowed values list */
	unsigned char iallowedrange;    /* index in allowed values range */ // Added for certification purpose 
	unsigned char ieventvalue;	/* fixed value returned or magical values */
};

static const char presentationpage_begin[] =
"<! DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"
	"<head>"
		"<title>miniUPnP IGD's presentation page</title>"
		"<style type=\"text/css\">"
			"#menu"
			"{"
				"float: left;"
				"width: 120px;"
			"}"
			"#body"
			"{"
				"margin-left: 140px;"
				"margin-right: 140px;"
				"margin-bottom: 20px;"
				"padding: 5px;"
				"color: #B3B3B3;"
				"background-color: #626262;"
			"}"
			"table"
			"{"
				"margin: auto;"
				"border: 0px;"
				"background-color: #D6D7DE;"
			"}"
			"th"
			"{"
				"background-color: #000000;"
				"color: #B3B3B3;"
			"}"
			"td"
			"{"
				"background-color: #FFFFFF;"
				"color: #B3B3B3;"
			"}"
			"p"
			"{"
				"text-indent: 30px;"
				"text-align: justify;"
			"}"
			"body"
			"{"
				"margin: auto;"
				"background-color: #626262;"
			"}"
		"</style>"
	"</head>"
	"<div id=\"menu\">"
	"</div>"
	"<div id=\"body\">"
		"<h1>Welcome on the miniUPnP IGD's presentation page </h1>"
		"<p>This page enables you to have information about the different services available in the miniUPnP Internet Gatewat Device (IGD).</p>"
		"<h2>About the miniUPnP IGD</h2>"
		"<p>Version: </p>"
		"<p>Based on the <acronym title=\"Universal Plug and Play\">UPnP</acronym> Device Architecture 1.0<p>"
		"<h2>Available services</h2>"
		"<p>Here is the list of the available service on the miniUPnP IGD:<br /></p>";

static const char presentationpage_table_begin[] =
		"<h3>%s</h3>"
		"<p> Click on the following link to have the <a href=\"%s\">service's description</a>.</p>"
		"<p> Service's state variables status: <br />"
		"<table valign=top align=left cellpadding=1 cellspacing=3 height=\"157\">"
			"<tr>"
				"<th valign=center align=center width=120 height=\"17\"><b>Variable</b></td>"
				"<th valign=middle align=center width=580 height=\"17\"><b>Value</b></td>"
			"</tr>";

static const char presentationpage_table_content[] =
			"<tr>"
				"<td valign=center align=center height=\"59\"><b>%s</b></td>"
				"<td valign=top height=\"59\"><b>%s</b></td>"
			"</tr>";

static const char presentationpage_table_end[] =
		"</table>"
		"<br /></p>";

static const char presentationpage_end[] =
	"</div>"
"</html>";


/* little endian 
 * The code has now be tested on big endian architecture */
#define INITHELPER(i, n) ((char *)((n<<16)|i))

/* char * genRootDesc(int *);
 * returns: NULL on error, string allocated on the heap */
char *
genRootDesc(int * len);

/* for the two following functions */
char *
genWANIPCn(int * len);

char *
genWANCfg(int * len);

#ifdef ENABLE_6FC_SERVICE
char *
genWANIP6FC(int * len);
#endif

#ifdef ENABLE_L3F_SERVICE
char *
genL3F(int * len);
#endif

#ifdef ENABLE_EVENTS
char *
getVarsWANIPCn(int * len);

char *
getVarsWANCfg(int * len);

char *
getVarsWANIP6FC(int * len);

char *
getVarsL3F(int * len);
#endif

#endif

char *
genPresentationPage();

