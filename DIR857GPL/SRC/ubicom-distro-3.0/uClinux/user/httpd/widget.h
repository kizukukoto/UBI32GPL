extern void do_widget_action(char *url, FILE *stream, int len, char *boundary);
extern void send_widget_response(char *url, FILE *stream);
extern void do_agentLogin_action(char *url, FILE *stream, int len, char *boundary);
extern void send_agentLogin_response(char *url, FILE *stream);
/*
* 	Date: 2010-9-30
* 	Name: Yufa Huang
* 	Reason: Fixed: All static route rules are setting will over buffer size.
* 	Notice :
*/			
#define  MAX_RES_LEN_XML  9216

struct xml_cache_t {
	char *name;
	char data[MAX_RES_LEN_XML];
	int use_cache;
	int count;
};
