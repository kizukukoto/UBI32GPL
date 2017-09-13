#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_cpu_loadavg(int argc, char *argv[])
{
	float f1=0, f2=0, f3=0, avg=0;
        FILE *file = NULL;

        file = fopen("/proc/loadavg", "r");
        if(file==NULL)
        {
                perror("fopen fail\n");
                return -1;
        }

	fscanf(file, "%f %f %f", &f1, &f2, &f3);

	fclose(file);
	avg = (f1+f2+f3)/3;
	printf("%3.2f\n", avg);
	return 0;
}

int cpuinfo_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "loadavg", get_cpu_loadavg},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
