extern void do_ajax_action(char *url, FILE *stream, int len, char *boundary);
extern void send_ajax_response(char *url, FILE *stream);
extern void ajax_init(int conn_fd, char *protocol);
extern void do_ping_action(char *url, FILE *stream, int len, char *boundary);
extern void add_ajax_headers(int status, char* title, char* extra_header, char* mime_type, long byte, char *mod );
extern void ajax_add_to_response( char* str, int len );
/*
* 	Date: 2009-2-24
* 	Name: Ken_Chiang
* 	Reason: modify for show upnp dynamic rule can up to 200 rules.
* 	Notice :
*/
#define MAX_RES_LEN_XML 15496
