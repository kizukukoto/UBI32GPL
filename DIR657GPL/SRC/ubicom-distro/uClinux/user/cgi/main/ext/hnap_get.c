#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>

#include "ssi.h"
#include "log.h"
#include "mtd.h"

#include "mime.h"
#include "hnap.h"

char *do_hnap_get()
{
	char xml_resraw[8192];
	char xml_resraw2[8192];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2, get_device_settings_result,
		nvram_safe_get("pure_type"),
		nvram_safe_get("pure_device_name"),
		nvram_safe_get("pure_vendor_name"),
		nvram_safe_get("pure_model_description"),
		nvram_safe_get("hostname"),
		"1.01",
		nvram_safe_get("pure_presentation_url"),
#ifdef AUTHGRAPH
		atoi(nvram_safe_get("graph_auth_enable")) == 0? "false": "true",
#endif
		nvram_safe_get("pure_wireless_url"),
		nvram_safe_get("pure_block_url"),
		nvram_safe_get("pure_parental_url"),
		nvram_safe_get("manufacturer"),
		nvram_safe_get("pure_support_url")
	);
	strcat(xml_resraw, xml_resraw2);

	fputs("HTTP/1.0 200 OK\r\n", stdout);
	fputs(xml_resraw, stdout);

	return NULL;
}
