#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "libdb.h"
#include "ssc.h"

char *online_firmware_check()
{
    setenv("html_response_message","Connecting with the server for firmware information",1);
    setenv("html_response_page","tools_fw_chk_result.asp",1);
    setenv("html_response_return_page","tools_fw_chk_result.asp",1);
    return getenv("html_response_page");
}
