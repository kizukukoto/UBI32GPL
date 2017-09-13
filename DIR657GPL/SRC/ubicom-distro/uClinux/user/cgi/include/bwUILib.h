#ifndef BW_UI_LIB_H
#define BW_UI_LIB_H


#define BW_ERR		-1
#define BW_OK		0


#define BW_IPS_ON	1
#define BW_IPS_OFF	0

#define MAX_CATG_SIZE	12


/* Application Name Length */
#define APP_NAME_LEN	64


/* Action Block / Non-Block Definition */
#define BW_APP_ACTION_BLOCK			1
#define BW_APP_ACTION_NON_BLOCK		0

/* Application Threshold Type Definition */
#define BW_APP_TH_TYPE_DDOS			0
#define BW_APP_TH_TYPE_PORT_SCAN		1

typedef struct KaTop5_t
{
	char IpAddr[5][16]; 
	int Freq[5];
	
} KaTop5;


typedef struct KaCategory_t
{
	int Num;
	char Category[MAX_CATG_SIZE][32]; 
	int Freq[MAX_CATG_SIZE];
	
} KaCategory;


typedef struct KaRawData_t
{
	char Time[32];
	char Name[64];
	char IpAddr[16];
	
} KaRawData;


typedef struct KaAppStatTH_t
{
	unsigned char Index;
	unsigned char Action;
	unsigned short AppId;
	unsigned int Threshold;
	
} KaAppStatTH;

typedef struct KaAppInfoTH_t
{
	KaAppStatTH Stat;
	char Name[APP_NAME_LEN];
	
} KaAppInfoTH;


int kaSetIPSFunction(int Enable);

int kaGetIPSFunction(int *Enable);

int kaGetSigVer(int *MajorVer, int *MinorVer);

int kaGetUpdateTime(char *UpdateTime);


int kaPaintChart(FILE *fp);

int kaGetTop5(KaTop5 *Top5);

int kaGetRawData(int idx, KaRawData *RawData);


int kaGetKey();

int kaGetCategory(KaCategory *Cate);

int kaUpdateSig(char *PolicyFile);

void kaSemWait(int SemId);

void kaSemSignal(int SemId);

int kaSettingReset();

int kaSetAppInfoTH(KaAppInfoTH *pAppInfoTH, unsigned int AppTHType);

int kaGetAppInfoTH(KaAppInfoTH *pAppInfoTH, unsigned int AppTHType);

#endif


