#define _printf(dev, fmt, args...) do { \
	FILE *cfp = fopen(dev, "a"); \
	if (cfp) { \
		fprintf(cfp, fmt, ## args); \
		fclose(cfp); \
	} \
} while (0)

#define cprintf(fmt, args...) _printf("/dev/console", fmt, ## args);

