#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include "graphlib.h"

#define AUTH_BMP_LEN (9429 + 25)
#define BASE_BMP_LEN 1434
//#define PIC_PATH "/www"
#define PIC_PATH	"/etc/bmp"
#define SESSION_PATH	"/tmp/graph"
//extern unsigned char *auth_bmp_data;

#if 0
#define DEBUG_MSG printf
#else
#define DEBUG_MSG
#endif

#if 0
static void do_file(char *path, FILE *stream)
{
	int c;
	FILE *fp;
	/*
	char wwwPath[20] = "/www/";
	*/
	/*	Date: 2009-02-03
	*	Name: jimmy huang
	*	Reason: some path is longer than 20
	*			ex: /www/but_management_0.gif
	*	Note:
	*		
	*/
	//char wwwPath[40] = "/www/";
	char wwwPath[40] = "../";
	strcat(wwwPath, path);

	if (!(fp = fopen(wwwPath, "r")))
		return;
	while ((c = getc(fp)) != EOF)
		fputc(c, stream);
	fclose(fp);
}
#endif

/*
 * Init RAW data of BMP.
 * */
static void init_auth_bmp_data(unsigned char *data, int size)
{
	int i;
	
	size -= 54;
	
	for (i=0; i< size; i++)
		*(auth_bmp_data + 54 + i) = 0xff;   
		
	*(auth_bmp_data + 0) = 0x42; *(auth_bmp_data + 1) = 0x4d;*(auth_bmp_data + 2) = 0xcf;
	*(auth_bmp_data + 3) = 0x25; *(auth_bmp_data + 4) = 0x00;*(auth_bmp_data + 5) = 0x00;
	*(auth_bmp_data + 6) = 0x00; *(auth_bmp_data + 7) = 0x00;*(auth_bmp_data + 8) = 0x00;
	*(auth_bmp_data + 9) = 0x00; *(auth_bmp_data + 10) = 0x36;*(auth_bmp_data + 11) = 0x00;
	*(auth_bmp_data + 12) = 0x00; *(auth_bmp_data + 13) = 0x00;*(auth_bmp_data + 14) = 0x28;
	*(auth_bmp_data + 15) = 0x00; *(auth_bmp_data + 16) = 0x00;*(auth_bmp_data + 17) = 0x00;
	*(auth_bmp_data + 18) = 0x7d; *(auth_bmp_data + 19) = 0x00;*(auth_bmp_data + 20) = 0x00;
	*(auth_bmp_data + 21) = 0x00; *(auth_bmp_data + 22) = 0x19;*(auth_bmp_data + 23) = 0x00;
	*(auth_bmp_data + 24) = 0x00; *(auth_bmp_data + 25) = 0x00;*(auth_bmp_data + 26) = 0x01;
	*(auth_bmp_data + 27) = 0x00; *(auth_bmp_data + 28) = 0x18;*(auth_bmp_data + 29) = 0x00;
	*(auth_bmp_data + 30) = 0x00; *(auth_bmp_data + 31) = 0x00;*(auth_bmp_data + 32) = 0x00;
	*(auth_bmp_data + 33) = 0x00; *(auth_bmp_data + 34) = 0x00;*(auth_bmp_data + 35) = 0x00;
	*(auth_bmp_data + 36) = 0x00; *(auth_bmp_data + 37) = 0x00;*(auth_bmp_data + 38) = 0x61;
	*(auth_bmp_data + 39) = 0x0f; *(auth_bmp_data + 40) = 0x00;*(auth_bmp_data + 41) = 0x00;
	*(auth_bmp_data + 42) = 0x61; *(auth_bmp_data + 43) = 0x0f;*(auth_bmp_data + 44) = 0x00;
	*(auth_bmp_data + 45) = 0x00; *(auth_bmp_data + 46) = 0x00;*(auth_bmp_data + 47) = 0x00;
	*(auth_bmp_data + 48) = 0x00; *(auth_bmp_data + 49) = 0x00;*(auth_bmp_data + 50) = 0x00;
	*(auth_bmp_data + 51) = 0x00; *(auth_bmp_data + 52) = 0x00;*(auth_bmp_data + 53) = 0x00;
#if 0
	0		1  2    3    4		5	6	7	8	9	   10	11	12		13 14  15
	0x42,0x4d,0xcf,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00, //16, 
16	  17	18	19	20	  21	22	23	24	  25	26	27	28	 29	   30	31
0x00,0x00,0x7d,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
/* --------------------------------------------------- */
32	 33		34	35	36		37 38	39	40	  41	42	43	44	  45	46	47
0x00,0x00,0x00,0x00,0x00,0x00,0x61,0x0f,0x00,0x00,0x61,0x0f,0x00,0x00,0x00,0x00,
48	 49		50	51	52		53
0x00,0x00,0x00,0x00,0x00,0x00
#endif
}

static int copy_bmp2data(const char *dst_data, const char *bmp_file, int total_chars, int nth)
{
	int fd, offset_x, offset_y;
	unsigned char buf[BASE_BMP_LEN];
	
	DEBUG_MSG("%s, using %s to generate number !\n",__FUNCTION__,bmp_file);
	if ((fd = open(bmp_file, O_RDONLY)) == -1) {
		printf("%s,line %d, can't open %s !, errno = %d\n",
				__FUNCTION__, __LINE__, bmp_file,errno);
		return -1;
	}
	read(fd, buf, BASE_BMP_LEN);
	close(fd);
	
	//srand((unsigned int)time(NULL));
	offset_y = rand() % 3;
	offset_x = rand() % 5;
	DEBUG_MSG("offset_x:%d, offset_y:%d\n", offset_x, offset_y);
	AuthGraph_CopyFromA2B((unsigned char *) dst_data, buf,
		25 * nth + offset_x, offset_y);
	return 0;
}
/*
 * Generate @codes[5] numbers and output to @stream
 * OUTPUT: @codes: 5 randed numbers to array
 * */
//static void do_auth_pic(char *path, FILE *stream, uint8_t codes[])
static void do_auth_pic(FILE *stream, uint8_t codes[])
{
	unsigned long auth_id;
	/* jimmy modified , modify auth codes from 4 to 5 */
	//unsigned short auth_code;
	unsigned long auth_code;
	/* ---------------------------------------------- */
	unsigned long afile_size, afile_width, afile_height;
	unsigned long font, n;
	unsigned char *nfile = NULL;
	char bmp_file[64];
	int i;

#if 0
	if(strncmp(path,"auth.bmp",strlen("auth.bmp")) != 0){
		do_file(path,stream);
		return ;
	}
#endif
	DEBUG_MSG("Request coming for graphic authentication pic, auth.bmp ...\n");
	
	auth_id   = AuthGraph_GetId();
	auth_code = AuthGraph_GetCode();
	/* jimmy modified , modify auth codes from 4 to 5 */
	/*
	unsigned long 4 bytes:
	   1 byte    1 byte    1 byte    1 byte
	|----|----|----|----|----|----|----|----|
	   1   c    1    5    5    b    4    2
		we don't need the first 3 bytes
		so we shift the first 3 bytes to
	|----|----|----|----|----|----|----|----|
	  5    5    b    4    2    0    0    0
	*/
	auth_id = auth_id << 12;
	auth_code = auth_code << 12;
	/* ---------------------------------------- */
	/* malloc for pics */
	//auth_bmp_data = malloc((unsigned char *)AUTH_BMP_LEN); //unsigned char *?
	auth_bmp_data = malloc(AUTH_BMP_LEN);
	/* reset the content ==> remove to here to avoid stack overflow ?? */
	init_auth_bmp_data(auth_bmp_data, AUTH_BMP_LEN);

	afile_size = AuthGraph_GetBMPLength((unsigned char *) auth_bmp_data);
	DEBUG_MSG("%s, auth_bmp_data size = %lu bytes\n",__FUNCTION__,afile_size);
	DEBUG_MSG("%s, resetting auth_bmp_data content\n",__FUNCTION__);
	/* reset the content ==> remove to above to avoid stack overflow ?? */
	//AuthGraph_Reset((unsigned char *) auth_bmp_data);

	afile_width = AuthGraph_GetBMPWidth((unsigned char *) auth_bmp_data);
	afile_height = AuthGraph_GetBMPHeight((unsigned char *) auth_bmp_data);
	//size : 9679, 125 x 25
	DEBUG_MSG("size : %lu, %lu x %lu\n", afile_size, afile_width, afile_height);
	
	/* read in base bmp data from file */
	nfile = (unsigned char *)malloc(BASE_BMP_LEN);
	if(nfile == NULL){
		printf("%s,line %d, malloc failed !\n",__FUNCTION__,__LINE__);
		if(auth_bmp_data)
			free(auth_bmp_data);
		return ;
	}

	for (i = 0; i < 5; i++) {

		n = (auth_code >> (28 - i * 4)) & 0x000f;

		if (i == 0 && n == 0) {
			do {
				n = rand() % 16;
			} while (n == 0);
		}

		font = rand() % 4;
		DEBUG_MSG("font:%lu, %d, %X\n", font, rand(), n);
		codes[i] = n;
		sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);

		copy_bmp2data(auth_bmp_data, bmp_file, 5, i); 
	}
	if(nfile)
		free(nfile);

	/* random pixel */
	for(i=0; i<200; i++) {
		unsigned long x, y, c_r, c_g, c_b;
		x = rand() % (afile_width);
		y = rand() % (afile_height);

		c_r = rand() & 0xff;
		c_g = rand() & 0xff;
		c_b = rand() & 0xff;

		AuthGraph_DrawPixel((unsigned char *) auth_bmp_data,  
				x, y, c_r, c_g, c_b);
	}

	for(i = 0 ; i < AUTH_BMP_LEN ; i++){
		fputc(auth_bmp_data[i], stream);
	}
	
	/* malloc for pics */
	if(auth_bmp_data){
		free(auth_bmp_data);
	}
	/* ------------------ */
}

static int authgraph_sessionid(size_t range_min, size_t range_max)
{
	if (range_min > range_max)
		range_min ^= range_max ^= range_min ^= range_max;

	return rand() % (range_max - range_min + 1) + range_min;
}

#define GRAPH_CHARS_LEN         5
static int authgraph_session_record(int session_id, const char *graph_code)
{
	FILE *fp;
	int ret = -1;
	char fname[128], session_contain[16];

	sprintf(session_contain, "%d.%X%X%X%X%X",
				 session_id,
				 graph_code[0],
				 graph_code[1],
				 graph_code[2],
				 graph_code[3],
				 graph_code[4]);
	sprintf(fname, "%s/%s", SESSION_PATH, getenv("REMOTE_ADDR"));

	/* create folder SESSION_PATH if SESSION_PATH not exists */
	if (access(SESSION_PATH, F_OK) != 0)
		mkdir(SESSION_PATH, 0755);

	if ((fp = fopen(fname, "w")) == NULL)
		goto out;

	fprintf(fp, "%s", session_contain);
	fclose(fp);

	sprintf(fname, "%s_allow", fname);
	if (access(fname, F_OK) == 0)
		unlink(fname);

	ret = 0;
out:
	return ret;
}

/* in cgi/lib/libutil.c */
extern int base64_encode(char *, char *, int);

static int authgraph_gcode_base64(uint8_t *gcode, char *gcode_base64)
{
	char org[GRAPH_CHARS_LEN];

        sprintf(org, "%X%X%X%X%X",
			gcode[0],
			gcode[1],
			gcode[2],
			gcode[3],
			gcode[4]);

	return base64_encode(org, gcode_base64, strlen(org));
}

/* Function authgraph_main() is a misc type cgi,
 * this function can create a .bmp image file into @argv[1].
 * This .bmp image contains five random charactors or numbers.
 * It used for http authentication.
 * */
int authgraph_main(int argc, char *argv[])
{
	int session_id;
        uint8_t codes[GRAPH_CHARS_LEN], codes_base64[GRAPH_CHARS_LEN];
	
	srand((unsigned int)time(NULL));

	if (argc != 2)
		goto out;

	do_auth_pic(fopen(argv[1], "wb"), codes);

	/* XXXX */
	DEBUG_MSG("%X,%X,%X,%X,%X\n", codes[0], codes[1], codes[2], codes[3], codes[4]);
	DEBUG_MSG("remote addr: %s\n", getenv("REMOTE_ADDR"));

	session_id = authgraph_sessionid(0, 65536);
	authgraph_session_record(session_id, codes);
	/* FIXME: After calling function 'authgraph_gcode_base64', @codes is
	 * original 6 bytes graphic charactors, and @codes_base64 is base64
	 * encoded @code. @codes_base64 will displayed on login_pic.asp.
	 *
	 *   In login_pic.asp, when user click 'Login' button, javascript will
	 * encode graph code. This encoded graph code we call
	 * @user_codes_base64 now. Javascript will compare @codes_base64 and
	 * @user_codes_base64. If @codes_base64 != @user_codes_base64, pop-up
	 * the warning message to user, otherwise commit to cgi and continue
	 * to check username and password.
	 * */
	authgraph_gcode_base64(codes, codes_base64);

	/* display on UI: <img src=auth.bmp>
	 * 		  <div align="left">                  </div>
	 * 		  <input type=hidden id="session_id" name="session_id" value="...">
	 * */
	printf("<img src=\"auth.bmp\">\n\t\t\t\t\t\t"
	        "<div align=\"left\">                  </div>\n");
	printf("\t\t\t\t\t\t<input type=hidden id=\"session_id\" "
	       "name=\"session_id\" value=\"%d\">\n", session_id);
	printf("\t\t\t\t\t\t<input type=hidden id=\"gcode_base64\" "
	       "name=\"gcode_base64\" value=\"%s\">\n", codes_base64);

	/* unset current_user here */
	nvram_unset("current_user");
out:
	return 0;
}
