/*
* Name Albert Chen
* Date 2009-03-04
* Modify for independant with http
*/

//#include <nvram.h>
#include <sutil.h>
//#include "log.h"
//#include "httpd_util.h"

/*
 * Mail-On-Schedule Debug Messgae Define
 */

#ifdef MAILOSD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
