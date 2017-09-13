/* app.h
 *
 * $Id: app.h,v 1.2 2007/09/04 09:29:13 roy Exp $
 */
  

struct demand{
	char *name;
	int reStartbyRc;	
	int (*stop)(int flag);	// stop function
	int (*start)(void);	// start function
	int (*psmon)(void);	// process monitor action
};

