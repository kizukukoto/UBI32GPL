#define NULL (void *)0

static const char data_404_html[] =
"<html>\n"
"<body bgcolor=\"white\">\n"
"<center>\n"
"  <h1>404 - file not found</h1>\n"
"</center>\n"
"</body>\n"
"</html>\n";

static const char data_error_html[] =
	/*  */
"<html>\n"
"<head>\n"
"<title>backup loader</title>\n"
"<meta http-equiv=Content-Type content=\"text/html; charset=iso-8859-1\">\n"
"</head>\n"
"<body>\n"
"  <center><font color=blue face=verdana size=5><b>D-Link Router Recovery Mode</b></font>\n"
"  <p></p>\n"
"  <br></br>\n"
"  <table cellPadding=0 cellSpacing=10>\n"
"    <tr>\n"
"	<td>\n"
"	  <font color=red face=verdana size=5><b>Upgrade Failed !</b></font>\n"
"	</td>\n"
"    </tr>\n"
" </table>\n"
" </center>\n"
" <br></br>\n"
" <center><hr></hr></center>\n"
" <center><font face=Verdana color=red size=5>WARNING !!</font></center>\n"
" <br></br>\n"
" <font face=Verdana size=3><li>Do not interrupt the update process, as if may damage the device</font>\n"
" <hr></hr>\n"
"</body>\n"
"</html>\n";

static const char data_counter_html[] =
"<html>\n"
"<head>\n"
"<title>backup loader</title>\n"
"<script language=\"javascript\">\n"
"var count = 0;\n"
"function count_down()\n"
"{\n"
"	if (count == 201) {\n"
"		get_by_id(\"show_sec\").innerHTML = \"<font color=red face=verdana size=5> Upgrade Successfully !!</font>\";\n"
"		return;\n"
"	}\n"
"\n"
"	get_by_id(\"show_sec\").innerHTML = \"<font face=verdana size=4> Device is upgrading the firmware ... \" + Math.ceil(count / 2) + \" %</font>\";\n"
"	if (count < 201) {\n"
"		count++;\n"
"		setTimeout('count_down()', 1000);\n"
"	}\n"
"}\n"
"\n"
"function get_by_id(id)\n"
"{\n"
"	with(document) {\n"
"		return getElementById(id);\n"
"	}\n"
"}\n"
"</script>\n"
"</head>\n"
"<body>\n"
" <center>\n"
"	<font color=blue face=verdana size=5><b>D-Link Router Recovery Mode</b></font>\n"
"	<br></br>\n"
" 	<br></br>\n"
"	<table>\n"
"	 <tr>\n"
"  	  <td><span id=\"show_sec\"></span></td>\n"
"	</tr>\n"
"	</table>\n"
" </center>\n"
" <br></br>\n"
" <br></br>\n"
" <center><hr><ul><br><font face=Verdana color=red size=5>WARNING !!</font></br></ul></hr>&nbsp;</center>\n"
" <li>Do not interrupt the update process, as it may damage the device !</li>\n"
"<center><hr><center>\n"
"<script>\n"
"count_down();\n"
"</script>\n"
"</body>\n"
"</html>\n";

static const char data_index_html[] = 
"<html>\n"
"<head>\n"
" <meta content=\"text/html; charset=iso-8859-1\" http-equiv=Content-Type>\n"
"</head>\n"
"<body>\n"
" <form action=\"/cgi/index\" enctype=\"multipart/form-data\" method=post>\n"
"  <center>\n"
"   <p><font color=blue face=verdana size=5><b>D-Link Router Recovery Mode</b></font></p>\n"
"   <br></br>\n"
"    <table cellPadding=0 cellSpacing=10>\n"
"     <tr>\n"
"     <td align=right>Firmware Image :</td>\n"
"     <td><input type=file name=files value=""></td>\n"
"     <td><input type=\"submit\" value=\"Upload\"></td>\n"
"     <p></P>\n"
"     </tr>\n"
"    </table>\n"
"  </center>\n"
"  </form>\n"
"   <br></br>\n"
"  <center><hr></center>\n"
"  <center><br><font face=Verdana color=red size=5>WARNING !!</font></br></center>&nbsp;&nbsp;\n"
"  <font face=Verdana size=3><li>If a wrong firmware image is upgraded, the device may not work properly or even could not boot-up again.<hr></font>\n"
"</body>\n"
"</html>\n";

static const char data_cgi_index[] = {
	/* /cgi/index */
	0x2f, 0x63, 0x67, 0x69, 0x2f, 0x69, 0x6e, 0x64, 0x65, 0x78, 0,
	0x63, 0x20, 0x61, 0xa, 0x2e, 0xa, };


const struct fsdata_file file_404_html[] = {{NULL, "/404.html", data_404_html, sizeof(data_404_html)}};

const struct fsdata_file file_error_html[] = {{file_404_html, "/error.html", data_error_html, sizeof(data_error_html)}};

const struct fsdata_file file_counter_html[] = {{file_error_html, "/counter.html", data_counter_html, sizeof(data_counter_html)}};

const struct fsdata_file file_index_html[] = {{file_counter_html, "/index.html", data_index_html, sizeof(data_index_html)}};

const struct fsdata_file file_cgi_index[] = {{file_index_html, data_cgi_index, data_cgi_index + 11, sizeof(data_cgi_index) - 11}};

#define FS_ROOT file_cgi_index

#define FS_NUMFILES 5