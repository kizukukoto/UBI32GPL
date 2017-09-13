#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvram.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <rc.h>

//add Web Access for file transfer
int echo_config_wa()
{
	FILE *fd;
	char cmds[512];
	char tmp[512];
	char *user;
	char *cut_user = NULL, *user_usr, *user_pwd;
	char *delim = "/|";

	system("cp -f /etc/lighttpd_base.conf /var/tmp/lighttpd-wa/lighttpd.conf");
	
/* Copy auth.conf , web file access will be use it. */
	system("cp -f /etc/conf.d/auth_base.conf /var/tmp/lighttpd-wa/auth.conf");


/* Write lighttpd.user for HNAP1 */
	/*if( (fd = fopen("/var/tmp/lighttpd-wa/lighttpd.user", "w+") ) != NULL) {
		fprintf(fd, "%s:%s", nvram_safe_get("admin_username"), nvram_safe_get("admin_password"));
		fclose(fd);
	}*/
	

	/* wirte server port for Web File Access only */
	sprintf(cmds, "echo server.port = %s >> /var/tmp/lighttpd-wa/lighttpd.conf", nvram_safe_get("file_access_remote_port"));
	system(cmds);
	/* Enable http remote */
	if (!nvram_match("file_access_remote", "1")) {
		memset(cmds, '\0', sizeof(cmds));
		sprintf(cmds, "echo "
					"\"\\$SERVER[\\\"socket\\\"] == \\\":%s\\\" {\n"
					"       server.document-root = \\\"/www/webfile_access/\\\"\n"
					"}\""
					">> /var/tmp/lighttpd-wa/lighttpd.conf", nvram_safe_get("file_access_remote_port"));
			system(cmds);
	}

	/* Enable https remote */
	if (!nvram_match("file_access_ssl", "1")) {
		if (nvram_match("feature_vpn", "1")) {
			memset(cmds, '\0', sizeof(cmds));
			strcpy(cmds, "ln -sf /etc/ssl.pem /var/tmp/lighttpd-wa/ssl.pem");
			system(cmds);
	}
	memset(cmds, '\0', sizeof(cmds));
	sprintf(cmds, "echo "
			"\"\\$SERVER[\\\"socket\\\"] == \\\":%s\\\" {\n"
			"	ssl.engine = \\\"enable\\\"\n"
			"	ssl.pemfile = \\\"/var/tmp/lighttpd-wa/ssl.pem\\\"\n"
			"       server.document-root = \\\"/www/webfile_access/\\\"\n"
			"}\""
			">> /var/tmp/lighttpd-wa/lighttpd.conf", nvram_safe_get("file_access_ssl_port"));
		system(cmds);
	}
		
		/* Set connections limit */
		memset(cmds, '\0', sizeof(cmds));
		sprintf(cmds, "echo "
				"\"\\$HTTP[\\\"url\\\"] =~ \\\"^/storage/\\\" {\n"
				"       server.max-connections = 20\n"
				"	evasive.max-conns-per-ip = 2\n"
				"	evasive.silent = \\\"enable\\\"\n"
				"}\""
				">> /var/tmp/lighttpd-wa/lighttpd.conf");
		system(cmds);

		/* Set auth.conf */
		memset(cmds, '\0', sizeof(cmds));
		sprintf(cmds, "echo "
				"\"\\$HTTP[\\\"url\\\"] =~ \\\"^/appaccess\\\" {\n"
				"	auth.backend = \\\"plain\\\"\n"
				"	auth.backend.plain.userfile = \\\"/etc/lighttpd-ws.user\\\"\n"
				"	auth.require = ( \\\"/appaccess\\\" => (\n"
				"			\\\"method\\\"  => \\\"basic\\\",\n"
				"			\\\"realm\\\"   => \\\"Web File Access Authentication\\\",\n"
				"			\\\"require\\\" => \\\"web_access_user\\\"\n"
				"			))\n"
				"}\""
				">> /var/tmp/lighttpd-wa/auth.conf");
		system(cmds);

		memset(cmds, '\0', sizeof(cmds));
		strcpy(cmds, "sed -i 's/web_access_user/user=admin|user=guest");

		user = (char *)malloc(strlen(nvram_safe_get("storage_account")) + 1);
		strcpy(user, nvram_safe_get("storage_account"));
		cut_user = strtok(user, delim);
		while (cut_user != NULL) {
			user_usr = cut_user;
			user_pwd = strtok(NULL, delim);
			sprintf(cmds, "%s|user=%s", cmds, user_usr);
			cut_user = strtok(NULL, delim);
		}

		sprintf(cmds, "%s/g' /var/tmp/lighttpd-wa/auth.conf", cmds);
		system(cmds);
		
		/* Write fileaccess.user */
		if ((fd = fopen("/var/tmp/lighttpd-wa/lighttpd-ws.user", "w+") ) != NULL) {
			memset(cmds,'\0', sizeof(cmds));
			fprintf(fd, "%s:%s\n", nvram_safe_get("admin_username"), nvram_safe_get("admin_password"));
			fprintf(fd, "guest:guest\n");

			strcpy(user, nvram_safe_get("storage_account"));
			cut_user = strtok(user, delim);
			while (cut_user != NULL) {
				user_usr = cut_user;
				user_pwd = strtok(NULL, delim);
				fprintf(fd, "%s:%s\n", user_usr, user_pwd);
				cut_user = strtok(NULL, delim);
			}
			fclose(fd);
		}
		free(user);

	

	return 0;
}

int lighttpd_start()
{
	
		
	if(access("/var/tmp/run", F_OK) != 0)
		mkdir("/var/tmp/run", 0755);
	if(access("/var/log/lighttpd", F_OK) != 0)
		mkdir("/var/log/lighttpd", 0755);
	if(access("/var/tmp/lighttpd-wa", F_OK) != 0)
		mkdir("/var/tmp/lighttpd-wa", 0755);
	
	echo_config_wa();	
	system("echo 1 > /var/tmp/web_file");		
	system("ln -s /mnt/shared/ /www/webfile_access/storage");
	
	
	system("lighttpd -f /etc/lighttpd.conf -A &");
  
	return 0;
}

int lighttpd_restart()
{
	echo_config_wa();	
	
	system("killall -SIGHUP lighttpd-angel");
	return 0;
}


int lighttpd_stop(int argc, char *argv[])
{
	return system("killall lighttpd");
}

