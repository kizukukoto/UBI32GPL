#include "ssi.h"
#include "querys.h"
#include "firmware.h"

//=============================================================================
// Public functions used for SSI & SSC checked by Antony "Begin"



void internal_error( char* reason )
{
	char* title = "500 Internal Error";

    	(void) printf( "\
		<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\
		<BODY><H2>%s</H2>\n\
		Something unusual went wrong during a server-side-includes request:\n\
		<BLOCKQUOTE>\n\
		%s\n\
		</BLOCKQUOTE>\n\
		</BODY></HTML>\n", title, title, reason );
}


void not_found( char* filename )
{
    	char* title = "404 Not Found";

	// The MIME type has to be text/html.
	(void) fputs("Content-type: text/html\n\n", stdout);
    	(void) printf( "\
		<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\
		<BODY><H2>%s</H2>\n\
		The requested server-side-includes filename, %s,\n\
		does not seem to exist.\n\
		</BODY></HTML>\n", title, title, filename );
}

void debug_message( char* message )
{
    	char* title = "Debug Message";

    	(void) printf( "\
		<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\
		<BODY><H2>%s</H2>\n\
		The debug message ,( %s ).\n\
		</BODY></HTML>\n", title, "Tony", message );
}


// Public functions used for SSI & SSC checked by Antony "END"
//=============================================================================
