#include <stdio.h>
#include <string.h>
#include <linux_vct.h>

#ifndef NOVCT
#include <vct.h>
#else
 #ifdef CONFIG_UBICOM_SWITCH_BCM539X
  #define SWITCH_DEVICE "bcm539x"
  #define WAN_PORT		4
  #define LAN_PORT0		0
  #define LAN_PORT1		1
  #define LAN_PORT2		2
  #define LAN_PORT3		3
 #endif
 
 #ifdef CONFIG_UBICOM_SWITCH_AR8316
  #define SWITCH_DEVICE "ar8316-smi"
  #define WAN_PORT		5
  #define LAN_PORT0		2
  #define LAN_PORT1		3
  #define LAN_PORT2		4
  #define LAN_PORT3		1
 #endif

 #ifdef CONFIG_UBICOM_SWITCH_AR8327
  #define SWITCH_DEVICE "ar8327-smi"
  #define WAN_PORT		5
  #define LAN_PORT0		1
  #define LAN_PORT1		2
  #define LAN_PORT2		3
  #define LAN_PORT3		4
 #endif
#endif

/* the linux_vct would check vct.c which in platform's vct driver
  * the vct.c must include three function and some define
  *	1. int setPortSpeed(char *ifname, int  portspeed);
  *	2. int getPortConnectState(char *ifname, int portNum);
  *	3. int getPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode);
  *	4. define VCTSETPORTSPEED_AUTO, VCTSETPORTSPEED_10HALF ...
  */

#ifdef LIBPLATFORM_DEBUG_VCT
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef NOVCT
int setPortSpeed(char *ifname, int portspeed)
{
	return 0;
}

int getPortConnectState(char *ifname, int portNum)
{
	return 0;
}

int getPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	return 0;
}

int VCTSetPortSpeed(char *ifname, char *portspeed)
{
	FILE *fp;
	char path[128] = {0};
	char speedmode[16] = {0};
	int port=WAN_PORT;
	
	DEBUG_MSG("VCTSetPortSpeed ifname=%s,portspeed=%s\n",ifname,portspeed);	
	if( !strcmp(portspeed, "auto"))
		strcpy(speedmode,"AUTO");
	else if( !strcmp(portspeed, "10half"))
		strcpy(speedmode,"10HD");
	else if( !strcmp(portspeed, "10full"))
		strcpy(speedmode,"10FD");
	else if( !strcmp(portspeed, "100half"))
		strcpy(speedmode,"100HD");
	else if( !strcmp(portspeed, "100full"))
		strcpy(speedmode,"100FD");
	else if( !strcmp(portspeed, "giga"))
		strcpy(speedmode,"1000FD");
	else{
		strcpy(speedmode,"AUTO");
		DEBUG_MSG("VCTSetPortSpeed unknow portspeed=%s\n",portspeed);
	}		
	DEBUG_MSG("VCTSetPortSpeed speedmode=%s\n",speedmode);
	sprintf(path,"/proc/switch/%s/port/%x/media",SWITCH_DEVICE, port);
	DEBUG_MSG("VCTSetPortSpeed path=%s\n",path);
	fp = fopen(path,"w");
    if(fp)
    {
        fputs(speedmode,fp);
		fclose(fp);
    }
    else{
    	DEBUG_MSG("VCTSetPortSpeed fopen fail\n");    	
    }	
    //fclose(fp);    
		
	return 0;
}

int VCTGetPortConnectState(char *ifname, int portNum, char *connect_state)
{
	FILE *fp;
	char path[128] = {0};
	int port=0,connect=0;	
	
	DEBUG_MSG("VCTGetPortConnectState ifname=%s,portNum=%x\n",ifname,portNum);
	switch(portNum){
		case VCTWANPORT0:
			port = WAN_PORT;
			break;
		case VCTLANPORT0:
			port = LAN_PORT0;
			break;
		case VCTLANPORT1:
			port = LAN_PORT1;
			break;
		case VCTLANPORT2:
			port = LAN_PORT2;
			break;
		case VCTLANPORT3:
			port = LAN_PORT3;
			break;
		default:
			DEBUG_MSG("VCTGetPortConnectState unknow portNum=%x\n",portNum);
			strcpy(connect_state,"disconnect");
			return 0;
	}
	sprintf(path,"/proc/switch/%s/port/%x/state",SWITCH_DEVICE, port);
	DEBUG_MSG("VCTGetPortConnectState path=%s\n",path);
	fp = fopen(path,"r");
    if(fp)
    {
        	fscanf(fp,"%d",&connect);
		fclose(fp);
        	DEBUG_MSG("VCTGetPortConnectState buf=%d\n",connect);
        	if(connect != 0){
        		DEBUG_MSG("VCTGetPortConnectState buf=%d connect\n",connect);
        	strcpy(connect_state,"connect");
        }	
        else{
        		DEBUG_MSG("VCTGetPortConnectState buf=%d disconnect\n",connect);
        	strcpy(connect_state,"disconnect");
        }
    }
    else{
    		DEBUG_MSG("VCTGetPortConnectState disconnect - no file\n");
    	strcpy(connect_state,"disconnect");
    }	
	
	return 0;
}

int VCTGetPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	FILE *fp;
	char path[128] = {0};
	char buf[16] = {0};
	int port=0;	
	
	DEBUG_MSG("VCTGetPortSpeedState ifname=%s,portNum=%x\n",ifname,portNum);
	switch(portNum){
		case VCTWANPORT0:
			port = WAN_PORT;
			break;
		case VCTLANPORT0:
			port = LAN_PORT0;
			break;
		case VCTLANPORT1:
			port = LAN_PORT1;
			break;
		case VCTLANPORT2:
			port = LAN_PORT2;
			break;
		case VCTLANPORT3:
			port = LAN_PORT3;
			break;
		default:
			DEBUG_MSG("VCTGetPortConnectState unknow portNum=%x\n",portNum);
			strcpy(portSpeed, "100");
			strcpy(duplexMode, "full");	
			return 0;
	}
	
	sprintf(path,"/proc/switch/%s/port/%x/media",SWITCH_DEVICE, port);
	DEBUG_MSG("VCTGetPortSpeedState path=%s\n",path);
	fp = fopen(path,"r");
    if(fp)
    {
        fgets(buf,16,fp);
        DEBUG_MSG("VCTGetPortSpeedState buf=%s\n",buf);
        if(strstr(buf,"1000FD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 1000FD\n",buf);
        	strcpy(portSpeed, "giga");
			strcpy(duplexMode, "full");	
        }
        else if(strstr(buf,"1000HD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 1000HD\n",buf);
        	strcpy(portSpeed, "giga");
			strcpy(duplexMode, "half");	
        }	
        else if(strstr(buf,"100FD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 100FD\n",buf);
        	strcpy(portSpeed, "100");
			strcpy(duplexMode, "full");	
        }
        else if(strstr(buf,"100HD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 100HD\n",buf);
        	strcpy(portSpeed, "100");
			strcpy(duplexMode, "half");	
        }	
        else if(strstr(buf,"10FD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 10FD\n",buf);
        	strcpy(portSpeed, "10");
			strcpy(duplexMode, "full");	
        }
        else if(strstr(buf,"10HD")){
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s 10HD\n",buf);
        	strcpy(portSpeed, "10");
			strcpy(duplexMode, "half");	
        }	
        else{
        	DEBUG_MSG("VCTGetPortSpeedState buf=%s disconnect\n",buf);
        	strcpy(portSpeed, "100");
			strcpy(duplexMode, "full");
        }
		fclose(fp);
    }
    else{
    	DEBUG_MSG("VCTGetPortSpeedState 100 full\n");
    	strcpy(portSpeed, "100");
		strcpy(duplexMode, "full");	
    }	
    //fclose(fp);    	
		
	return 0;
}

#else
int VCTSetPortSpeed(char *ifname, char *portspeed)
{
	int ret=0;
	int speedmode= -1;

	if( !strcmp(portspeed, "auto"))
		speedmode =  VCTSETPORTSPEED_AUTO;
	if( !strcmp(portspeed, "10half"))
		speedmode =  VCTSETPORTSPEED_10HALF;
	if( !strcmp(portspeed, "10full"))
		speedmode =  VCTSETPORTSPEED_10FULL;
	if( !strcmp(portspeed, "100half"))
		speedmode =  VCTSETPORTSPEED_100HALF;
	if( !strcmp(portspeed, "100full"))
		speedmode =  VCTSETPORTSPEED_100FULL;
	if( !strcmp(portspeed, "giga"))
		speedmode =  VCTSETPORTSPEED_1000FULL;

	if( speedmode >=0)
		ret = setPortSpeed(ifname, speedmode);	
	return ret;
}

int VCTGetPortConnectState(char *ifname, int portNum, char *connect_state)
{
	int state;
	int port=0;

	switch(portNum){
		case VCTWANPORT0:
			port = WAN_PORT;
			break;
		case VCTLANPORT0:
			port = LAN_PORT0;
			break;
		case VCTLANPORT1:
			port = LAN_PORT1;
			break;
		case VCTLANPORT2:
			port = LAN_PORT2;
			break;
		case VCTLANPORT3:
			port = LAN_PORT3;
			break;

	}
	
	state = getPortConnectState(ifname, port);

	if(state == 1)
		strcpy(connect_state,"connect");
	else 
		strcpy(connect_state,"disconnect");

	return 0;
}

int VCTGetPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	int port=0;
	switch(portNum){
		case VCTWANPORT0:
			port = WAN_PORT;
			break;
		case VCTLANPORT0:
			port = LAN_PORT0;
			break;
		case VCTLANPORT1:
			port = LAN_PORT1;
			break;
		case VCTLANPORT2:
			port = LAN_PORT2;
			break;
		case VCTLANPORT3:
			port = LAN_PORT3;
			break;

	}
	return getPortSpeedState(ifname, port, portSpeed, duplexMode);
}
#endif
