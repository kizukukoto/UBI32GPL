/* $Id: upnpdescstrings.h,v 1.5 2007/02/09 10:12:52 nanard Exp $ */
/* miniupnp project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the coditions detailed in
 * the LICENCE file provided within the distribution */
#ifndef __UPNPDESCSTRINGS_H__
#define __UPNPDESCSTRINGS_H__

#include "config.h"

/* strings used in the root device xml description */
#define ROOTDEV_FRIENDLYNAME		friendlyname // OS_NAME " router"
#define ROOTDEV_MANUFACTURER		manufacturername // OS_NAME
#define ROOTDEV_MANUFACTURERURL		manufacturerurl // OS_URL
#define ROOTDEV_MODELNAME			modelname // OS_NAME " router"
//#define ROOTDEV_MODELDESCRIPTION	modelname // OS_NAME " router"
#define ROOTDEV_MODELDESCRIPTION	friendlyname
#define ROOTDEV_MODELURL			modelurl // OS_URL
#define ROOTDEV_MODELNUMBER			modelnumber // MODEL_NUMBER

#define WANDEV_FRIENDLYNAME			friendlyname // "WANDevice"
#define WANDEV_MANUFACTURER			manufacturername //"MiniUPnP"
#define WANDEV_MANUFACTURERURL		manufacturerurl // "http://miniupnp.free.fr/"
#define WANDEV_MODELNAME			modelname // "WAN Device"
#define WANDEV_MODELDESCRIPTION		friendlyname // "Internet Access Router" // "WAN Device"
#define WANDEV_MODELNUMBER			modelnumber // UPNP_VERSION
#define WANDEV_MODELURL				modelurl // "http://miniupnp.free.fr/"
#define UPC	"0001"
#define WANDEV_UPC				UPC	//"MINIUPNPD"

#define WANCDEV_FRIENDLYNAME		friendlyname // "WANConnectionDevice"
#define WANCDEV_MANUFACTURER		manufacturername // WANDEV_MANUFACTURER
#define WANCDEV_MANUFACTURERURL		manufacturerurl // WANDEV_MANUFACTURERURL
#define WANCDEV_MODELNAME			modelname // "WANConnectionDevice" // "MiniUPnPd"
#define WANCDEV_MODELDESCRIPTION	friendlyname // "WANConnectionDevice" // "MiniUPnP daemon"
#define WANCDEV_MODELNUMBER			modelnumber // UPNP_VERSION
#define WANCDEV_MODELURL			modelurl // "http://miniupnp.free.fr/"
#define WANCDEV_UPC				UPC	//"MINIUPNPD"

#endif

