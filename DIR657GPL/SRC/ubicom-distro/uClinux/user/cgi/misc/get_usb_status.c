#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

/* If usb device has be inserted, the act will be "add".
 * If no, it will be "remove".
*/
static void show_usbstatus(const char *act)
{
       if(strcmp(act, "add") == 0)
		printf("1");
        else
		printf("0");
}

/* The function will auto detect usb device status.
 * It maybe insert or not. And it detects by /tmp/usb-detect.txt file.
 * When usb device could be inserted the usb port, the DUT will create the
 * tmpefile. If no usb device could be inserted at booting, the file 
 * will not create.
*/
int get_usb_status_main(int argc, char *argv[])
{
        int ret = -1;
        FILE *fp;
        char cmd[256], tmp[256];
        char *p;

        bzero(cmd, sizeof(cmd));
        bzero(tmp, sizeof(tmp));

        sprintf(cmd, "cat /tmp/usb-detect.txt|grep ACTION");
        fp = popen(cmd, "r");
	
        bzero(cmd, sizeof(cmd));
        if(fp == NULL){
	/* Maybe usb device not be inserted. 
	 * So must give it a default value 
        */
		printf("0");
                goto out;
        }
        if(fgets(tmp, sizeof(tmp), fp) == NULL){
	/* Maybe usb detect file has no useful information.
         * So must give it a default value 
        */
		printf("0");
                goto out;
        }else {
                p = strstr(tmp, "'");
                if(p == NULL)
                        goto out;
                p += 1;
                strcpy(cmd, p);
                cmd[strlen(p)-2] = '\0';
        }

        show_usbstatus(cmd);
        pclose(fp);
        ret = 0;
out:
        return ret;
}
