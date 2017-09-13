#ifndef __CUSTOMER__
#define __CUSTOMER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern http_main(int, char *);
extern int wfprintf (FILE * fp, char *fmt, ...);
extern int wfflush (FILE * fp);

#define __TMPVAR(x) tmpvar ## x
#define _TMPVAR(x) __TMPVAR(x)
#define TMPVAR _TMPVAR(__LINE__)
#define websWrite(wp, fmt, args...) ({ int TMPVAR = wfprintf(wp, fmt, ## args); wfflush(wp); TMPVAR; })
typedef FILE * webs_t;

/* CmoGetCfg */
/* 
*  How do 
*  CmoGetCfg("name")
*  CmoGetCfg("name","none"),CmoGetCfg("name","ascii"),CmoGetCfg("name","hex")
*  ex: added in asp page
*  var cfg0 = "<% CmoGetCfg("customer_variable_00"); %>";
*  var cfg1 = "<% CmoGetCfg("customer_variable_00","none"); %>";
*/
/**************************************************************/
/**************************************************************/
typedef struct mapping_s{
	char *nvram_mapping_name;
}mapping_t;
mapping_t customer_ui_to_nvram[]= {
	{"customer_variable_00"}	
};
int customer_variable_num = sizeof(customer_ui_to_nvram)/sizeof(customer_ui_to_nvram[0]);
/**************************************************************/
/**************************************************************/

/* CmoGetStatus */
/* 
*  How do 
*  CmoGetStatus("name")
*  CmoGetStatus("name","args")
*  ex: added in asp page
*  var sts0 = "<% CmoGetStatus("customer_status_00"); %>";
*  var sts1 = "<% CmoGetStatus("customer_status_00","stats"); %>";
*/
/**************************************************************/
/**************************************************************/
typedef struct status_handler_s {
	char *pattern;
	void (*output)(webs_t wp,char *args);
}status_handler_t;
void return_customer_status_00(webs_t wp,char *args);
status_handler_t customer_status_handlers[]={
	{ "customer_status_00",return_customer_status_00},
	{ NULL, NULL }
};

void return_customer_status_00(webs_t wp,char *args){		
	
	/* do something in here */
	{
		if(args)
			websWrite(wp, "%s", args);
		else
			websWrite(wp, "%s", "none args");	
	}
	return;	
}    
/**************************************************************/
/**************************************************************/

/* CmoGetList */
/* 
*  How do: 
*  CmoGetList("name") 
*  CmoGetList("name","args")
*  ex: added in asp page
*  var lit0= "<% CmoGetList("customer_list_table_00"); %>";
*  var lit1= "<% CmoGetList("customer_list_table_00","list"); %>";
*/
/**************************************************************/
/**************************************************************/
typedef struct list_handler_s {
	char *pattern;
	void (*output)(webs_t wp,char *args);
}list_handler_t;
void return_customer_list_table_00(webs_t wp,char *args);
list_handler_t customer_list_handlers[]={
	{ "customer_list_table_00", return_customer_list_table_00 },
	{ NULL, NULL }
};	

void return_customer_list_table_00(webs_t wp,char *args){
	int i;
	
	/* do something in here */
	{
		if(args){
			for(i=0 ; i<10 ;i++)
				websWrite(wp,"%s%d,",args,i);		
		}	
		else{
			for(i=0 ; i<10 ;i++)
				websWrite(wp, "%s%d,", "none args",i);
		}
	}
	return;		
}

/* CmoCgi */
/* 
*  How do: 
*  action="name.cgi" 
*  ex: added in asp page
*  <form id="form0" name="form0" method="post" action="customer_00.cgi">
*  <input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
*  <input type="hidden" id="html_response_message" name="html_response_message" value="The setting is saved.">
*  <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="test.asp">
*/
/**************************************************************/
/**************************************************************/
const char * const redirecter_footer =
"<html>"
"<head>"
"<script>"
"function redirect(){"
"	document.location.href = '%s';"
"}"
"</script>"
"</head>"
"<script>"
"	redirect();"
"</script>"
"</html>"
;
extern void do_apply_post(char *url, FILE *stream, int len, char *boundary);
extern void do_auth(char *admin_userid, char *admin_passwd, char *user_userid, char *user_passwd, char *realm);
extern int save_html_page_to_nvram(webs_t wp);
extern char *absolute_path(char *page);
extern void init_cgi(char *query);
extern char *get_cgi(char *name);
extern char no_cache[];
#define websGetVar(wp, var, default) (get_cgi(var) ? : default)
struct mime_handler {
	char *pattern;
	char *mime_type;
	char *extra_header;
	void (*input)(char *path, FILE *stream, int len, char *boundary);
	void (*output)(char *path, FILE *stream);
	void (*auth)(char *admin_userid, char *admin_passwd, char *user_userid, char *user_passwd, char *realm);
};
void do_customer_00_cgi(char *url, FILE *stream);
struct mime_handler customer_mime_handlers[] = {
	{ "customer_00.cgi*", "text/html", no_cache, do_apply_post, do_customer_00_cgi, do_auth },
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};	
int customer_mime_handlers_num = sizeof(customer_mime_handlers)/sizeof(customer_mime_handlers[0]);	
void do_customer_00_cgi(char *url, FILE *stream){
	
	char *path=NULL, *query=NULL;
	char *html_response_page=NULL;
	html_response_page = websGetVar(wp, "html_response_page", "");
	
	//assert(url);
	//assert(stream);

	/* Parse path */
	query = url;
	path = strsep(&query, "?") ? : url;
	/* The function will get the summit attatched infomation 
	   html_response_page,html_response_message and html_response_return_page value
	   then save to nvram you can get the value in html_response_page(ex back.asp) by used 
	   value="<% CmoGetCfg("html_response_message","none"); %>">(ex: The setting is saved.)
	   and value="<% CmoGetCfg("html_response_return_page","none"); %>">(ex: test.asp).
	*/   
	save_html_page_to_nvram(stream);
	/* do something in here */
	{
		/* ex */
		system("ifconfig > /var/test.log");		
	}	
	//go to response_page
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
	/* Reset CGI */
	init_cgi(NULL);
}

int main(int argc, char *argv)
{	
    return httpd_main(NULL, NULL);	
}


#endif//__CUSTOMER__
