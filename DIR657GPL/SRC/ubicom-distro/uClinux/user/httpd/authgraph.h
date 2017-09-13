/* jimmy modified , modify auth codes from 4 to 5 */
//#define AUTH_BMP_LEN 7554 // 1875 * 4 + 54

/*
	NickChou modify it from 9429 to 9454
	To fix that Mac Safari can not show auth pic 
*/
#define AUTH_BMP_LEN 9454//9429  //1875 * 5 +54
/* ------------------------------------------------------- */
#define BASE_BMP_LEN 1434
#define MAX_AUTH_GRAPH_FILES 65
#define AUTH_GRAPH_MAPPING_SIZE 100

#define PIC_PATH "/www"


typedef struct hp_file{
	char pic_name[20];
	int len;
	//unsigned char *array_name;
}HP_FILE;

extern unsigned long cticks ;// = time( (time_t*) 0 );  /* clock ticks since startup */

extern HP_FILE authGraphFiles[];

/* malloc for pics */
extern unsigned char *auth_bmp_data;

#if 0
extern unsigned char f1_0_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_1_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_2_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_3_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_4_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_5_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_6_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_7_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_8_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_9_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_a_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_b_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_c_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_d_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_e_bmp_data[BASE_BMP_LEN];
extern unsigned char f1_f_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_0_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_1_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_2_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_3_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_4_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_5_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_6_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_7_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_8_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_9_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_a_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_b_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_c_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_d_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_e_bmp_data[BASE_BMP_LEN];
extern unsigned char f2_f_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_0_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_1_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_2_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_3_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_4_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_5_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_6_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_7_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_8_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_9_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_a_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_b_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_c_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_d_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_e_bmp_data[BASE_BMP_LEN];
extern unsigned char f3_f_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_0_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_1_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_2_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_3_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_4_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_5_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_6_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_7_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_8_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_9_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_a_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_b_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_c_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_d_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_e_bmp_data[BASE_BMP_LEN];
extern unsigned char f4_f_bmp_data[BASE_BMP_LEN];
#endif

extern void AuthGraph_GenerateIdCode(void);
extern int AuthGraph_ValidateIdCode(unsigned short auth_id, unsigned short auth_code);
/* jimmy modified , modify auth codes from 4 to 5 */
//extern unsigned short AuthGraph_GetId(void);
//extern unsigned short AuthGraph_GetCode(void);
extern unsigned long AuthGraph_GetId(void);
extern unsigned long AuthGraph_GetCode(void);
/* ---------------------------------------------- */
extern unsigned long AuthGraph_GetBMPLength(unsigned char *data);
extern unsigned long AuthGraph_GetBMPWidth(unsigned char *data);
extern unsigned long AuthGraph_GetBMPHeight(unsigned char *data);
extern void AuthGraph_Reset(unsigned char *data);
extern void AuthGraph_CopyFromA2B(unsigned char *data_B, unsigned char *data_A, unsigned long x, unsigned long y);
extern void AuthGraph_DrawPixel(unsigned char *data, unsigned long x, unsigned long y, 
    unsigned char color_b, unsigned char color_g, unsigned char color_r);

/* do we need these ?? */
#if 0
enum {
    FONT1_0 = 100,
    FONT1_1,
    FONT1_2,
    FONT1_3,
    FONT1_4,
    FONT1_5,
    FONT1_6,
    FONT1_7,
    FONT1_8,
    FONT1_9,
    FONT1_A,
    FONT1_B,
    FONT1_C,
    FONT1_D,
    FONT1_E,
    FONT1_F,
    
    FONT2_0 = 200,
    FONT2_1,
    FONT2_2,
    FONT2_3,
    FONT2_4,
    FONT2_5,
    FONT2_6,
    FONT2_7,
    FONT2_8,
    FONT2_9,
    FONT2_A,
    FONT2_B,
    FONT2_C,
    FONT2_D,
    FONT2_E,
    FONT2_F,
    
    FONT3_0 = 300,
    FONT3_1,
    FONT3_2,
    FONT3_3,
    FONT3_4,
    FONT3_5,
    FONT3_6,
    FONT3_7,
    FONT3_8,
    FONT3_9,
    FONT3_A,
    FONT3_B,
    FONT3_C,
    FONT3_D,
    FONT3_E,
    FONT3_F,
    
    FONT4_0 = 400,
    FONT4_1,
    FONT4_2,
    FONT4_3,
    FONT4_4,
    FONT4_5,
    FONT4_6,
    FONT4_7,
    FONT4_8,
    FONT4_9,
    FONT4_A,
    FONT4_B,
    FONT4_C,
    FONT4_D,
    FONT4_E,
    FONT4_F,
};

enum {
    hp_file_auth_bmp = 0,
    hp_file_f1_0_bmp = 100,
    hp_file_f1_1_bmp,
    hp_file_f1_2_bmp,
    hp_file_f1_3_bmp,
    hp_file_f1_4_bmp,
    hp_file_f1_5_bmp,
    hp_file_f1_6_bmp,
    hp_file_f1_7_bmp,
    hp_file_f1_8_bmp,
    hp_file_f1_9_bmp,
    hp_file_f1_a_bmp,
    hp_file_f1_b_bmp,
    hp_file_f1_c_bmp,
    hp_file_f1_d_bmp,
    hp_file_f1_e_bmp,
    hp_file_f1_f_bmp,
    hp_file_f2_0_bmp = 200,
    hp_file_f2_1_bmp,
    hp_file_f2_2_bmp,
    hp_file_f2_3_bmp,
    hp_file_f2_4_bmp,
    hp_file_f2_5_bmp,
    hp_file_f2_6_bmp,
    hp_file_f2_7_bmp,
    hp_file_f2_8_bmp,
    hp_file_f2_9_bmp,
    hp_file_f2_a_bmp,
    hp_file_f2_b_bmp,
    hp_file_f2_c_bmp,
    hp_file_f2_d_bmp,
    hp_file_f2_e_bmp,
    hp_file_f2_f_bmp,
    hp_file_f3_0_bmp = 300,
    hp_file_f3_1_bmp,
    hp_file_f3_2_bmp,
    hp_file_f3_3_bmp,
    hp_file_f3_4_bmp,
    hp_file_f3_5_bmp,
    hp_file_f3_6_bmp,
    hp_file_f3_7_bmp,
    hp_file_f3_8_bmp,
    hp_file_f3_9_bmp,
    hp_file_f3_a_bmp,
    hp_file_f3_b_bmp,
    hp_file_f3_c_bmp,
    hp_file_f3_d_bmp,
    hp_file_f3_e_bmp,
    hp_file_f3_f_bmp,
    hp_file_f4_0_bmp = 400,
    hp_file_f4_1_bmp,
    hp_file_f4_2_bmp,
    hp_file_f4_3_bmp,
    hp_file_f4_4_bmp,
    hp_file_f4_5_bmp,
    hp_file_f4_6_bmp,
    hp_file_f4_7_bmp,
    hp_file_f4_8_bmp,
    hp_file_f4_9_bmp,
    hp_file_f4_a_bmp,
    hp_file_f4_b_bmp,
    hp_file_f4_c_bmp,
    hp_file_f4_d_bmp,
    hp_file_f4_e_bmp,
    hp_file_f4_f_bmp
};

#endif

