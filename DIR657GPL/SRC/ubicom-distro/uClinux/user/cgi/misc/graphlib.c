#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <fcntl.h>
/* jimmy modified , modify auth codes from 4 to 5 */
#include <ctype.h>
/* ---------------------------- */
#include "graphlib.h"

/* x86 pc test used , little ENDIAN */
#ifdef DIRX30
#ifdef __BIG_ENDIAN
#undef __BIG_ENDIAN
#endif
#endif
#if 0
#define DEBUG_MSG printf
#else
	#define DEBUG_MSG while(0) printf
#endif

struct auth_graph_map_s {
/* jimmy modified , modify auth codes from 4 to 5 */
//    unsigned short auth_id;
//    unsigned short auth_code;
	unsigned long auth_id;
	unsigned long auth_code;
/* ---------------------------------------------- */
};
static struct auth_graph_map_s auth_graph_map[AUTH_GRAPH_MAPPING_SIZE];
static int auth_graph_init = 0;
static int auth_graph_index = 0;
unsigned long cticks = 0;// = time( (time_t*) 0 );  /* clock ticks since startup */

//unsigned char auth_bmp_data[AUTH_BMP_LEN];
#if 0
unsigned char auth_bmp_data[AUTH_BMP_LEN] = {
/* jimmy modified , modify auth codes from 4 to 5 */
//0x42,0x4d,0x82,0x1d,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
//0x00,0x00,0x64,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
0x42,0x4d,0xcf,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00, //16, 
0x00,0x00,0x7d,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
/* --------------------------------------------------- */
0x00,0x00,0x00,0x00,0x00,0x00,0x61,0x0f,0x00,0x00,0x61,0x0f,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00
};
#else
/* malloc for pics */
unsigned char *auth_bmp_data = NULL;
#endif

#if 0
unsigned char f1_0_bmp_data[BASE_BMP_LEN];
unsigned char f1_1_bmp_data[BASE_BMP_LEN];
unsigned char f1_2_bmp_data[BASE_BMP_LEN];
unsigned char f1_3_bmp_data[BASE_BMP_LEN];
unsigned char f1_4_bmp_data[BASE_BMP_LEN];
unsigned char f1_5_bmp_data[BASE_BMP_LEN];
unsigned char f1_6_bmp_data[BASE_BMP_LEN];
unsigned char f1_7_bmp_data[BASE_BMP_LEN];
unsigned char f1_8_bmp_data[BASE_BMP_LEN];
unsigned char f1_9_bmp_data[BASE_BMP_LEN];
unsigned char f1_a_bmp_data[BASE_BMP_LEN];
unsigned char f1_b_bmp_data[BASE_BMP_LEN];
unsigned char f1_c_bmp_data[BASE_BMP_LEN];
unsigned char f1_d_bmp_data[BASE_BMP_LEN];
unsigned char f1_e_bmp_data[BASE_BMP_LEN];
unsigned char f1_f_bmp_data[BASE_BMP_LEN];
unsigned char f2_0_bmp_data[BASE_BMP_LEN];
unsigned char f2_1_bmp_data[BASE_BMP_LEN];
unsigned char f2_2_bmp_data[BASE_BMP_LEN];
unsigned char f2_3_bmp_data[BASE_BMP_LEN];
unsigned char f2_4_bmp_data[BASE_BMP_LEN];
unsigned char f2_5_bmp_data[BASE_BMP_LEN];
unsigned char f2_6_bmp_data[BASE_BMP_LEN];
unsigned char f2_7_bmp_data[BASE_BMP_LEN];
unsigned char f2_8_bmp_data[BASE_BMP_LEN];
unsigned char f2_9_bmp_data[BASE_BMP_LEN];
unsigned char f2_a_bmp_data[BASE_BMP_LEN];
unsigned char f2_b_bmp_data[BASE_BMP_LEN];
unsigned char f2_c_bmp_data[BASE_BMP_LEN];
unsigned char f2_d_bmp_data[BASE_BMP_LEN];
unsigned char f2_e_bmp_data[BASE_BMP_LEN];
unsigned char f2_f_bmp_data[BASE_BMP_LEN];
unsigned char f3_0_bmp_data[BASE_BMP_LEN];
unsigned char f3_1_bmp_data[BASE_BMP_LEN];
unsigned char f3_2_bmp_data[BASE_BMP_LEN];
unsigned char f3_3_bmp_data[BASE_BMP_LEN];
unsigned char f3_4_bmp_data[BASE_BMP_LEN];
unsigned char f3_5_bmp_data[BASE_BMP_LEN];
unsigned char f3_6_bmp_data[BASE_BMP_LEN];
unsigned char f3_7_bmp_data[BASE_BMP_LEN];
unsigned char f3_8_bmp_data[BASE_BMP_LEN];
unsigned char f3_9_bmp_data[BASE_BMP_LEN];
unsigned char f3_a_bmp_data[BASE_BMP_LEN];
unsigned char f3_b_bmp_data[BASE_BMP_LEN];
unsigned char f3_c_bmp_data[BASE_BMP_LEN];
unsigned char f3_d_bmp_data[BASE_BMP_LEN];
unsigned char f3_e_bmp_data[BASE_BMP_LEN];
unsigned char f3_f_bmp_data[BASE_BMP_LEN];
unsigned char f4_0_bmp_data[BASE_BMP_LEN];
unsigned char f4_1_bmp_data[BASE_BMP_LEN];
unsigned char f4_2_bmp_data[BASE_BMP_LEN];
unsigned char f4_3_bmp_data[BASE_BMP_LEN];
unsigned char f4_4_bmp_data[BASE_BMP_LEN];
unsigned char f4_5_bmp_data[BASE_BMP_LEN];
unsigned char f4_6_bmp_data[BASE_BMP_LEN];
unsigned char f4_7_bmp_data[BASE_BMP_LEN];
unsigned char f4_8_bmp_data[BASE_BMP_LEN];
unsigned char f4_9_bmp_data[BASE_BMP_LEN];
unsigned char f4_a_bmp_data[BASE_BMP_LEN];
unsigned char f4_b_bmp_data[BASE_BMP_LEN];
unsigned char f4_c_bmp_data[BASE_BMP_LEN];
unsigned char f4_d_bmp_data[BASE_BMP_LEN];
unsigned char f4_e_bmp_data[BASE_BMP_LEN];
unsigned char f4_f_bmp_data[BASE_BMP_LEN];
#endif

HP_FILE authGraphFiles[MAX_AUTH_GRAPH_FILES] = {
{"auth.bmp", AUTH_BMP_LEN},
{"f1_0.bmp", BASE_BMP_LEN},
{"f1_1.bmp", BASE_BMP_LEN},
{"f1_2.bmp", BASE_BMP_LEN},
{"f1_3.bmp", BASE_BMP_LEN},
{"f1_4.bmp", BASE_BMP_LEN},
{"f1_5.bmp", BASE_BMP_LEN},
{"f1_6.bmp", BASE_BMP_LEN},
{"f1_7.bmp", BASE_BMP_LEN},
{"f1_8.bmp", BASE_BMP_LEN},
{"f1_9.bmp", BASE_BMP_LEN},
{"f1_a.bmp", BASE_BMP_LEN},
{"f1_b.bmp", BASE_BMP_LEN},
{"f1_c.bmp", BASE_BMP_LEN},
{"f1_d.bmp", BASE_BMP_LEN},
{"f1_e.bmp", BASE_BMP_LEN},
{"f1_f.bmp", BASE_BMP_LEN},//, f1_f_bmp_data},
{"f2_0.bmp", BASE_BMP_LEN},//, f2_0_bmp_data},
{"f2_1.bmp", BASE_BMP_LEN},//, f2_1_bmp_data},
{"f2_2.bmp", BASE_BMP_LEN},//, f2_2_bmp_data},
{"f2_3.bmp", BASE_BMP_LEN},//, f2_3_bmp_data},
{"f2_4.bmp", BASE_BMP_LEN},//, f2_4_bmp_data},
{"f2_5.bmp", BASE_BMP_LEN},//, f2_5_bmp_data},
{"f2_6.bmp", BASE_BMP_LEN},//, f2_6_bmp_data},
{"f2_7.bmp", BASE_BMP_LEN},//, f2_7_bmp_data},
{"f2_8.bmp", BASE_BMP_LEN},//, f2_8_bmp_data},
{"f2_9.bmp", BASE_BMP_LEN},//, f2_9_bmp_data},
{"f2_a.bmp", BASE_BMP_LEN},//, f2_a_bmp_data},
{"f2_b.bmp", BASE_BMP_LEN},//, f2_b_bmp_data},
{"f2_c.bmp", BASE_BMP_LEN},//, f2_c_bmp_data},
{"f2_d.bmp", BASE_BMP_LEN},//, f2_d_bmp_data},
{"f2_e.bmp", BASE_BMP_LEN},//, f2_e_bmp_data},
{"f2_f.bmp", BASE_BMP_LEN},//, f2_f_bmp_data},
{"f3_0.bmp", BASE_BMP_LEN},//, f3_0_bmp_data},
{"f3_1.bmp", BASE_BMP_LEN},//, f3_1_bmp_data},
{"f3_2.bmp", BASE_BMP_LEN},//, f3_2_bmp_data},
{"f3_3.bmp", BASE_BMP_LEN},//, f3_3_bmp_data},
{"f3_4.bmp", BASE_BMP_LEN},//, f3_4_bmp_data},
{"f3_5.bmp", BASE_BMP_LEN},//, f3_5_bmp_data},
{"f3_6.bmp", BASE_BMP_LEN},//, f3_6_bmp_data},
{"f3_7.bmp", BASE_BMP_LEN},//, f3_7_bmp_data},
{"f3_8.bmp", BASE_BMP_LEN},//, f3_8_bmp_data},
{"f3_9.bmp", BASE_BMP_LEN},//, f3_9_bmp_data},
{"f3_a.bmp", BASE_BMP_LEN},//, f3_a_bmp_data},
{"f3_b.bmp", BASE_BMP_LEN},//, f3_b_bmp_data},
{"f3_c.bmp", BASE_BMP_LEN},//, f3_c_bmp_data},
{"f3_d.bmp", BASE_BMP_LEN},//, f3_d_bmp_data},
{"f3_e.bmp", BASE_BMP_LEN},//, f3_e_bmp_data},
{"f3_f.bmp", BASE_BMP_LEN},//, f3_f_bmp_data},
{"f4_0.bmp", BASE_BMP_LEN},//, f4_0_bmp_data},
{"f4_1.bmp", BASE_BMP_LEN},//, f4_1_bmp_data},
{"f4_2.bmp", BASE_BMP_LEN},//, f4_2_bmp_data},
{"f4_3.bmp", BASE_BMP_LEN},//, f4_3_bmp_data},
{"f4_4.bmp", BASE_BMP_LEN},//, f4_4_bmp_data},
{"f4_5.bmp", BASE_BMP_LEN},//, f4_5_bmp_data},
{"f4_6.bmp", BASE_BMP_LEN},//, f4_6_bmp_data},
{"f4_7.bmp", BASE_BMP_LEN},//, f4_7_bmp_data},
{"f4_8.bmp", BASE_BMP_LEN},//, f4_8_bmp_data},
{"f4_9.bmp", BASE_BMP_LEN},//, f4_9_bmp_data},
{"f4_a.bmp", BASE_BMP_LEN},//, f4_a_bmp_data},
{"f4_b.bmp", BASE_BMP_LEN},//, f4_b_bmp_data},
{"f4_c.bmp", BASE_BMP_LEN},//, f4_c_bmp_data},
{"f4_d.bmp", BASE_BMP_LEN},//, f4_d_bmp_data},
{"f4_e.bmp", BASE_BMP_LEN},//, f4_e_bmp_data},
{"f4_f.bmp", BASE_BMP_LEN}//, f4_f_bmp_data}
};


static int auth_graph_function_init(void)
{
    int i;
//    unsigned long cticks = time( (time_t*) 0 );  /* clock ticks since startup */

    //srand(cticks);
    srand((unsigned int)time(NULL));
    
    for(i=0; i<AUTH_GRAPH_MAPPING_SIZE; i++) 
    {
        auth_graph_map[auth_graph_index].auth_id   = (unsigned long) rand();
        auth_graph_map[auth_graph_index].auth_code = (unsigned long) rand() % 0xffff;
    }
    auth_graph_init = 0;
    auth_graph_init = 1;
	return 0;
}

void AuthGraph_GenerateIdCode(void)
{
    if (auth_graph_init == 0)
        auth_graph_function_init();
        
    auth_graph_index++;
    
    if (auth_graph_index == AUTH_GRAPH_MAPPING_SIZE)
        auth_graph_index = 0;

    srand(cticks);
            
    /* jimmy modified , modify auth codes from 4 to 5 */
	//auth_graph_map[auth_graph_index].auth_id   = (unsigned short) rand() % 0xffff;
    //auth_graph_map[auth_graph_index].auth_code = (unsigned short) rand() % 0xffff;
	auth_graph_map[auth_graph_index].auth_id   = (unsigned long) rand() % 0xffffffff;
	auth_graph_map[auth_graph_index].auth_code = (unsigned long) rand() % 0xffffffff;
	//printf("%s, auth_graph_map[%ul].auth_id = %x\n"
	//			,__FUNCTION__
	//			,auth_graph_index
	//			,(unsigned int)auth_graph_map[auth_graph_index].auth_id);
	//printf("%s, auth_graph_map[%ul].auth_code = %x\n"
	//			,__FUNCTION__
	//			,auth_graph_index
	//			,(unsigned int)auth_graph_map[auth_graph_index].auth_code); 
	/* ----------------------------------------------------- */
    
    return;
}

int AuthGraph_ValidateIdCode(unsigned short auth_id, unsigned short auth_code)
{
    int i;
    /* jimmy modified , modify auth codes from 4 to 5 */
	//FixMe:
	/* This function not support 5 auth codes !!*/
	/* ---------------------------------------------- */
    if (auth_graph_init == 0)
        auth_graph_function_init();
            
    for(i=0; i<AUTH_GRAPH_MAPPING_SIZE; i++)
    {
        if (auth_graph_map[i].auth_id == auth_id && auth_graph_map[i].auth_code == auth_code)
            return 0;
    }
    
    return -1;
}

int AuthGraph_ValidateIdCodeByStr(unsigned char *auth_id, unsigned char *auth_code)
{
    int i;//,j;
    //printf("%s with auth_id = %s, auth_code = %s\n",__FUNCTION__,auth_id,auth_code);
    if (auth_id == NULL)
        return -1;
    if (auth_code == NULL)
        return -1;
        
    if (auth_graph_init == 0)
        auth_graph_function_init();
	
	DEBUG_MSG("%s,go search\n",__FUNCTION__);
    for(i=0; i<AUTH_GRAPH_MAPPING_SIZE; i++)
    {
	/* jimmy modified , modify auth codes from 4 to 5 */
        //unsigned char auth_id_str[5];
        //unsigned char auth_code_str[5];
        //sprintf(auth_id_str, "%04X", auth_graph_map[i].auth_id);
        //sprintf(auth_code_str, "%04X", auth_graph_map[i].auth_code);
		unsigned char auth_id_str[6];
        unsigned char auth_code_str[6];
		unsigned long auth_id_tmp;
		unsigned long auth_code_tmp;

		auth_id_tmp = auth_graph_map[i].auth_id << 12;
		sprintf(auth_id_str,"%x%x%x%x%x",(unsigned char )((auth_id_tmp >> 28)  & 0x000f)
			,(unsigned char )((auth_id_tmp >> 24)  & 0x000f)
			,(unsigned char )((auth_id_tmp >> 20)  & 0x000f)
			,(unsigned char )((auth_id_tmp >> 16)  & 0x000f)
			,(unsigned char )((auth_id_tmp >> 12)  & 0x000f));

		auth_code_tmp = auth_graph_map[i].auth_code << 12;
		sprintf(auth_code_str,"%x%x%x%x%x",(unsigned char )((auth_code_tmp >> 28)  & 0x000f)
			,(unsigned char )((auth_code_tmp >> 24)  & 0x000f)
			,(unsigned char )((auth_code_tmp >> 20)  & 0x000f)
			,(unsigned char )((auth_code_tmp >> 16)  & 0x000f)
			,(unsigned char )((auth_code_tmp >> 12)  & 0x000f));

		/* because we don't have pics with character in lower case , so .... */
		//for(j=0;j<6;j++){
			//auth_code_str[j] = toupper(auth_code_str[j]);
		//}
	/* ---------------------------------------------- */
        DEBUG_MSG("%s, [%d]: auth_id = %s, auth_code = %s\n",__FUNCTION__,i,auth_id_str,auth_code_str);
		//if(stricmp(auth_id_str, auth_id) == 0 && 
		//	stricmp(auth_code_str, auth_code) == 0){
		//if(strcmp(auth_id_str, auth_id) == 0 && strcmp(auth_code_str, auth_code) == 0){
		if(strcasecmp(auth_id_str, auth_id) == 0 && strcasecmp(auth_code_str, auth_code) == 0){
			DEBUG_MSG("%s, id and code matched !! \n",__FUNCTION__);
			return 0;
		}
    }
    
    return -1;
}

/* jimmy modified , modify auth codes from 4 to 5 */
//unsigned short AuthGraph_GetId(void)
unsigned long AuthGraph_GetId(void)
/* ----------------------------------------------- */
{
    if (auth_graph_init == 0)
        auth_graph_function_init();    

    return auth_graph_map[auth_graph_index].auth_id;
}    

/* jimmy modified , modify auth codes from 4 to 5 */
//unsigned short AuthGraph_GetCode(void)
unsigned long AuthGraph_GetCode(void)
/* ----------------------------------------------- */
{
    if (auth_graph_init == 0)
        auth_graph_function_init();

    return auth_graph_map[auth_graph_index].auth_code;
}



#if defined(__BIG_ENDIAN) || defined(__BigEndian)
#define value_l_swap(x) ((((x) & 0xff000000) >> 24) | \
                  (((x) & 0x00ff0000) >>  8) | \
                  (((x) & 0x0000ff00) <<  8) | \
                  (((x) & 0x000000ff) << 24))
#else
#define value_l_swap(x) (x)
#endif

unsigned long AuthGraph_GetBMPLength(unsigned char *data)
{
    unsigned long len;
    memcpy(&len, data+2, 4);
    return value_l_swap(len);
}

unsigned long AuthGraph_GetBMPWidth(unsigned char *data)
{
    unsigned long len;
    memcpy(&len, data+18, 4);
    return value_l_swap(len);    
}

unsigned long AuthGraph_GetBMPHeight(unsigned char *data)
{
    unsigned long len;
    memcpy(&len, data+22, 4);
    return value_l_swap(len);    
}
void AuthGraph_Reset(unsigned char *data)
{
    unsigned long i;
    for (i=0; i<(AuthGraph_GetBMPLength(data)-54); i++){
		*(data + 54 + i) = 0xff;   
	}
}

void AuthGraph_CopyFromA2B(unsigned char *data_B, unsigned char *data_A, unsigned long x, unsigned long y)
{
//    unsigned long a_size   = AuthGraph_GetBMPLength((unsigned char *) data_A);
    unsigned long a_width  = AuthGraph_GetBMPWidth((unsigned char *) data_A);
    unsigned long a_height = AuthGraph_GetBMPHeight((unsigned char *) data_A);
    
//    unsigned long b_size   = AuthGraph_GetBMPLength((unsigned char *) data_B);
    unsigned long b_width  = AuthGraph_GetBMPWidth((unsigned char *) data_B);
    unsigned long b_height = AuthGraph_GetBMPHeight((unsigned char *) data_B);    
    
    unsigned long pA_x, pA_y, pB_x, pB_y, pA, pB;
    
    for (pA_x=1; pA_x<=a_width; pA_x++)
    for (pA_y=1; pA_y<=a_height; pA_y++)
    {
        pA = a_width * (pA_y-1) + pA_x;
        pB_x = x + pA_x;
        pB_y = y + pA_y;
        
        if (pB_x>=b_width || pB_y>=b_height)
            continue;
            
        pB = b_width * (pB_y-1) + pB_x;
        
        *(data_B + 54 + pB * 3    ) = *(data_A + 54 + pA * 3    );
        *(data_B + 54 + pB * 3 + 1) = *(data_A + 54 + pA * 3 + 1);
        *(data_B + 54 + pB * 3 + 2) = *(data_A + 54 + pA * 3 + 2);
    }
    
}

void AuthGraph_DrawPixel(unsigned char *data, unsigned long x, unsigned long y, 
    unsigned char color_b, unsigned char color_g, unsigned char color_r)
{
    unsigned long width  = AuthGraph_GetBMPWidth(data);
    unsigned long height = AuthGraph_GetBMPHeight(data);
    unsigned long position;
    
    if (x>width)  x = width;
    if (y>height) y = height;
    if (x==0) x = 1;
    if (y==0) y = 1;
    
    position = width * (y-1) + x;
    
    *(data + 54 + position * 3    ) = color_r;
    *(data + 54 + position * 3 + 1) = color_g;
    *(data + 54 + position * 3 + 2) = color_b;
    
    return;
}



