#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvram.h>

#ifdef NVRAM_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static void usage(void)
{
	fprintf(stderr, "usage: nvram [get name] [set name=value] [unset name] [show] [save name file_name] [restore name file_name] [readfile file_name] [writefile file_name] [WiPNP]\n");
	exit(0);
}

int main(int argc, char* argv[])
{
	char *value, *name;
#ifdef BCM5352	
	char file[50];
	char nvram[50];
	char buf[NVRAM_SPACE];
	int size;
#else
	char buf[384];
#endif

	--argc;
	++argv;

	if (!*argv) 
		usage();

	for (; *argv; argv++) {
#ifdef SET_GET_FROM_ARTBLOCK
		if (!strncmp(*argv, "artblock_get", 12)) {
			if (*++argv) {
				if ((value = artblock_safe_get(*argv)))
					printf("%s = %s\n", *argv, value);
			}			
		}
/*
* Name Ken Chiang
* Date 2009-04-07
* Detail: Added for direct to get value from SettingInFlash[].
*/            
		if (!strncmp(*argv, "artblock_fast_get", 18)) {
			if (*++argv) {
				if ((value = artblock_fast_get(*argv)))
					printf("%s = %s\n", *argv, value);
				else
					printf("%s = Null\n", *argv);	
			}			
		}
#endif		
		if (!strncmp(*argv, "get", 3)) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					printf("%s = %s\n", *argv, value);
			}
		} else if ( strncmp(*argv, "set", 3) == 0) {
			if (*++argv) {
				strncpy(value = buf, *argv, sizeof(buf));
				name = strsep(&value, "=");
				nvram_set(name, value);
			}
		} else if ( strncmp(*argv, "readfile", 8) == 0) {
			if (*++argv) {
				strncpy(name = buf, *argv, sizeof(buf));
				nvram_readfile(name);
			}
		} else if ( strncmp(*argv, "writefile", 9) == 0) {
			if (*++argv) {
				strncpy(name = buf , *argv, sizeof(buf));
				nvram_writefile(name);
			}
		} else if ( strncmp(*argv, "initfile", 8) == 0) {
				nvram_initfile();
		} else if ( strncmp(*argv, "show", 4) == 0) {
#ifdef BCM5352
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			fprintf(stderr, "size: %d bytes (%d left)\n", size, NVRAM_SPACE - size);
#else
			nvram_show();
#endif
		} else if ( strncmp(*argv, "upgrade", 7) == 0) {
			nvram_upgrade();
		} else if ( strncmp(*argv, "commit", 6) == 0 ) {
			nvram_commit();
		} else if (strncmp(*argv, "unset", 5) == 0 ) {
			if (*++argv)
				nvram_unset(*argv);
		} else if (strncmp(*argv, "restore_default", 15) == 0) {
			nvram_restore_default();
		}		
#ifdef BCM5352
		else if(strncmp(*argv, "save", 4) == 0 ) {
			strcpy(nvram,argv[1]);
			strcpy(file,argv[2]);
			nvram_file_save(nvram,file);
			printf("save finish\n");
		} else if(strncmp(*argv, "restore", 7) == 0) {
			strcpy(nvram,argv[1]);
			strcpy(file,argv[2]);
			nvram_file_restore(nvram,file);
			printf("restore finish \n");
		}
#endif		
		else if(strncmp(*argv, "WiPNP", 5) == 0) {
			nvram_WiPNP();
			printf("Write WiPNP to flash \n");
		}
		if (!*argv)
			break;
	}
	return 0;
}	

