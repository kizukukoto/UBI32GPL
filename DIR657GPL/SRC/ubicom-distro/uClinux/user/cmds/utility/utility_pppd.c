#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#define TMP_DIR "/tmp"

void utility_pppd_status(const char *st_file){

	FILE *fp;
        char __st_file[128], fname[128];
        char tunnel_type[24],local_ip[24],remote_ip[24];
        bzero(fname, sizeof(fname));

	strncpy(fname, st_file, (strlen(st_file)-1));
 	sprintf(__st_file, "%s/vpn_tunnel/%s", TMP_DIR, fname);
        
	if ((fp = fopen(__st_file, "r")) == NULL)
                return;

	//printf("%s:%d\n", __FUNCTION__, __LINE__);
	fscanf(fp, "%[^,],%[^,],%[^,]", tunnel_type, local_ip, remote_ip);
	printf("%s,%s,%s#", tunnel_type, local_ip, remote_ip);
        fclose(fp);
}

int utility_pppd_main(int argc, char *argv[])
{
	char pppd_file[1024];
	FILE *file;
	int rev = -1;
	chdir(TMP_DIR);

	file = popen("ls vpn_tunnel/|grep \"pppd-*\"", "r");
        if (file == NULL)
                goto out;
	//printf("%s:%d\n", __FUNCTION__, __LINE__);
	while(!feof(file)){
	//printf("%s:%d\n", __FUNCTION__, __LINE__);
                if(fgets(pppd_file, sizeof(pppd_file), file) != NULL){
			//printf("%s:%d:%s\n", __FUNCTION__, __LINE__, pppd_file);
                        utility_pppd_status(pppd_file);
                }
        }
        pclose(file);
	chdir("..");
	rev = 0;
out:
	return rev;
}
