/*
Read /proc/gpio file.
read wps_push_count ; first line 
If the last read of wps_push_count is different  lauch the call to wps softbutton
otherwise sleep for 1/2 sec and wakeup
*/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define BUF_LEN 10

int wps_push_count_prev = 0;

int invoke_soft_wps()
{
	/* launch wpatalk first time */
	if (wps_push_count_prev == 1) {
		if (vfork() == 0) {
			printf("Launching wpa_supplicant .. \n");
			const char *argk[] = {"/etc/wps_wrapper", NULL};
			execve("/etc/wps_wrapper", argk, NULL);
		}
		sleep(5);
	}
		
	if (vfork() == 0) {
		printf("Launching wpatalk... \n");

		const char *argv[4] = { "wpatalk", "ath0", "configme", NULL };
		execve("/sbin/wpatalk", argv, NULL);	
	} 
	return 0;

}

int main()
{
  int cur_read = 0;
  char buf[BUF_LEN];
  printf("Listening on /dev/input/event0 ...\n");
  FILE *fd = open("/dev/input/event0", O_RDONLY);

  if (fd) {  
    while (1) {
      printf("reading...\n");
      //printf(" Opend /proc/gpio \n");
      if(read(fd, buf, BUF_LEN))
	printf("got input crap: %x",buf);
      sleep(1);
    }
  } 
  
  return(0);
}
