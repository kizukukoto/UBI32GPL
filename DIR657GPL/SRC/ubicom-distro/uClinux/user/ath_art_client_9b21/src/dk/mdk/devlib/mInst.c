#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#ifdef WIN32
#include <direct.h>
#include "decl-32.h"
#endif
#ifdef LINUX
#include "ugpib.h"
#include "termios.h"
#include <fcntl.h>
#endif
#include <stdarg.h>
#include <ctype.h>
#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "manlibInst.h"
#include "mInst.h"
#include "mConfig.h"
#ifndef LINUX
#include "addnl_inst/usb_pm.h"
#endif
#ifdef LINUX
#define sleep Sleep // redifining sleep to avoid conflict with unix sleep call
#endif

double* spa_get_trace_11b(const int ud, int iNumSwpPts);
int spa_compare_mask_11b(double *trace, 
                         double *mask, 
                         const double f0, 
                         const double df, 
                         const int verbose, 
                         const int output,
                         int iNumSwpPts);

// Globals
#ifdef LINUX
#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"
struct termios oldtio;
#else
HANDLE       hCom;
DCB          dcb;
COMMTIMEOUTS timeouts;
#endif
A_BOOL         fConnected                    = FALSE;
int          nComErr;

#define SPA_REF_LEVEL 10.0
#define NEW_SWP_PTS 401
// device numbers
int          spa, pm, att, ps;

// model numbers
int          tmp_model, spa_model, pm_model, att_model, ps_model;

static A_INT16 customMask = 0;
static A_INT16 maskLegacy[4][2];
static A_INT16 mask11nHt20[4][2];
static A_INT16 mask11nHt40[4][2];


void sleep( clock_t wait ) {
   clock_t goal;
   goal = wait + clock();
   while( goal > clock() );
}

MANLIB_API void gpibSendIFC(const int board)
{
#ifndef _IQV
	SendIFC( board );
#endif
}

MANLIB_API long gpibRSC(const int board, const int v)
{
#ifndef _IQV
	return(ibrsc(board, v));
#else
	return 1;
#endif
}

MANLIB_API long gpibSIC(const int board)
{
#ifndef _IQV
	return(ibsic(board));
#else
	return 1;
#endif
}


MANLIB_API long gpibClear(const int ud) {
#ifndef _IQV
	return(ibclr ( ud ));
#else
	return 1;
#endif
}

MANLIB_API long gpibRSP(const int ud, char *spr) {
#ifndef _IQV
	return(ibrsp ( ud, spr ));
#else
	return 1;
#endif
}

MANLIB_API long gpibONL(const int ud, const int v) {
#ifndef _IQV
	return(ibonl ( ud, v ));
#else
	return 1;
#endif

}

MANLIB_API long gpibWrite(const int ud, char *wrt) {
#ifndef _IQV
    ibwrt ( ud, wrt, strlen(wrt) );
    return ibcntl;
#else
	return 1;
#endif
}

MANLIB_API char *gpibRead(const int ud, ... ) {
#ifndef _IQV
    static char rd[512];
    va_list vl;
    long length;

    va_start( vl, ud );
    length = va_arg( vl, long );
    va_end( vl );

    ibrd ( ud, rd, ( length == 0L ? 511L : length ) );
    rd[ibcntl] = '\0';

    return rd;
#else
	return "";
#endif
}

MANLIB_API char *gpibQuery(const int ud, char *wrt, ... ) {
#ifndef _IQV
    static char rd[512];
    va_list vl;
    long length;

    gpibWrite ( ud, wrt );
    va_start( vl, wrt );
    length = va_arg( vl, long );
    va_end( vl );
    ibrd ( ud, rd, ( length == 0L ? 511L : length ) );
	// trap gpib error and invalid ibcntl
	if (((ibsta & 0x8000) != 0) && (( ibcntl > 509)||(ibcntl < 0)))  {
		rd[510] = '\0';
	} else {
		rd[ibcntl] = '\0';
	}
    return rd;
#else
	return "";
#endif
}

char *qq( char *format, ... ) {

    static char buf[512];
    va_list vl;

    va_start( vl, format );
    vsprintf( buf, format, vl );
    va_end( vl );
    return buf;

}

#ifndef LINUX
HANDLE com_open(char *port, const int model) {

    // reset error byte
    nComErr = 0;

    // device handle
    hCom = CreateFile( port,                             // port name
                       GENERIC_READ | GENERIC_WRITE,     // allow r/w access
                       0,                                // always no sharing
                       0,                                // no security atributes for file
                       OPEN_EXISTING,                    // always open existing
                       FILE_ATTRIBUTE_NORMAL,            // nonoverlapped operation   
                       0);                               // always no file template

    if (hCom == INVALID_HANDLE_VALUE) {
       nComErr = nComErr | COM_ERROR_GETHANDLE;
       return 0;
    }

    // port configuration
    FillMemory (&dcb, sizeof(dcb),0);
    dcb.DCBlength = sizeof(dcb);
    if (!BuildCommDCB("9600,n,8,1", &dcb)) {
       nComErr = nComErr | COM_ERROR_BUILDDCB;
       return 0;
    }
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    if (!SetCommState(hCom, &dcb)) {
       nComErr = nComErr | COM_ERROR_CONFIGDEVICE;
       return 0;
    }
    if (!SetupComm(hCom, READ_BUF_SIZE, WRITE_BUF_SIZE)) {
       nComErr = nComErr | COM_ERROR_CONFIGBUFFERS;
       return 0;
    }
    if (!EscapeCommFunction(hCom, SETDTR)) {
       nComErr = nComErr | COM_ERROR_SETDTR;
       return 0;
    }

    // connection flag
    fConnected = 1;

    tmp_model = model;

    // set mask to notify thread if a character was received
    if (!SetCommMask(hCom, EV_RXCHAR)) {
       // error setting communications event mask
       nComErr = nComErr | COM_ERROR_READ;
       return 0;
    }
    return hCom;
}

int com_close(const HANDLE ud) { 

    // reset error byte
    nComErr = 0;

    if (!fConnected) return 0;

    // connection flag
    fConnected = 0;

    // port configuration
    if (!EscapeCommFunction(ud, CLRDTR)) {
       nComErr = nComErr | COM_ERROR_CLEARDTR;
       return 0;
    }
    if (!PurgeComm(ud, PURGE_RXABORT | PURGE_RXCLEAR |
                       PURGE_TXABORT | PURGE_TXCLEAR)) {
       nComErr = nComErr | COM_ERROR_PURGEBUFFERS;
       return 0;
    }

    // device handle
    CloseHandle(ud);

    return 1;

}

unsigned long com_write(const HANDLE ud, char *wrt) {
    unsigned long cnt;

    // reset error byte
    nComErr = 0;

    // write the com port
    if (!WriteFile(ud, wrt, strlen(wrt), &cnt, NULL))    nComErr = nComErr | COM_ERROR_WRITE;
    return cnt;

}

char *com_read(const HANDLE ud) {

    static char   rd[50];
    unsigned long dwCommEvent;
    unsigned long dwRead, pos = 0;
    char          chRead;

    // reset error byte
    nComErr = 0;

    // wait for comm event
    if (WaitCommEvent(ud, &dwCommEvent, NULL)) {
       do {
          // read receive buffer
          if (ReadFile(ud, &chRead, 1, &dwRead, NULL)) {
             rd[pos] = chRead;
             pos++;
          }
          else {
             nComErr = nComErr | COM_ERROR_READ;
             return '\0';
          }
       } while (chRead != 0x06); // stop reading at termination character
    }
    else {
       nComErr = nComErr | COM_ERROR_READ;
       return '\0';
    }
    rd[pos] = '\0';
    return rd;
}
#else
HANDLE com_open(char *port, const int model) 
{
	HANDLE fd;
	struct termios newtio;

	nComErr = 0;
	
	fd = -1;
	if (strcmp(port,"COM1") == 0) {
			fd = open(COM1, O_RDWR | O_NOCTTY);
	}
	if (strcmp(port,"COM2") == 0) {
		   	fd = open(COM2, O_RDWR | O_NOCTTY);
	}		
	
	if (fd < 0) {
		nComErr = nComErr | COM_ERROR_GETHANDLE;
		return 0;
	}	

	tcgetattr(fd,&oldtio);
	memset(&newtio,0,sizeof(newtio));

	/*
	 * BAUD 9600
	 * CS8 8 Bit, no parity, 1 stop nit
	 * CLOCAL local connection 
	 * CREAD enable receiving 
	 */
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

	/*
	 * Ignore parity and map CR to NL
	 */ 
	newtio.c_iflag = IGNPAR | ICRNL;

	newtio.c_oflag = 0;

	/*
	 * enable canonical input 
	 */
	newtio.c_lflag = ICANON;

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	
    /*
	 * connection flag
	 */ 
    fConnected = 1;

    tmp_model = model;
	   
	return fd;
}	

int com_close(const HANDLE fd) 
{ 
	// reset error byte
	nComErr = 0;
	if (!fConnected) return 0;
	// connection flag
	fConnected = 0;
	
	tcsetattr(fd,TCSANOW,&oldtio);
	
	close(fd);	
	
	return 0;
}

unsigned long com_write(const HANDLE fd, char *wrt) 
{
	unsigned long cnt;

	// reset error byte
	nComErr = 0;

	// write the com port
	cnt = write(fd,wrt,strlen(wrt));
	if (!cnt) {
		nComErr = nComErr | COM_ERROR_WRITE;
	}
	
	return cnt;
}

char *com_read(HANDLE fd) 
{
	static char   rd[50];
	unsigned long dwCommEvent;
	unsigned long dwRead, pos = 0;
	char          chRead;

	// reset error byte
	nComErr = 0;

	do {
			// read receive buffer
			if (read(fd, &chRead, 1)) {
				rd[pos] = chRead;
				pos++;
			} else {
				nComErr = nComErr | COM_ERROR_READ;
				return '\0';
			}
	} while (chRead != 0x06); // stop reading at termination character
    rd[pos] = '\0';
    return rd;
}		
#endif
char *temp_ctrl_checksum(char *buf) {

    unsigned long i, num = 0;
    static char cksm[3];

    for (i = 0; i < strlen(buf); i++) {
        num += buf[i];
    }

    sprintf(cksm, "%2X", 0xff & num);
    return cksm;

}

MANLIB_API unsigned long tempOpen(char *port, const int model) {
    return (unsigned long)com_open(port, model);
}

MANLIB_API int tempClose(const unsigned long ud) {
    return com_close((HANDLE)ud);
}

MANLIB_API double tempMeasTemp(const unsigned long ud) {
    char hdr[5];
    char seed[5];
    char *cksm;
    char pkt[10];
    char *rsp;
    unsigned long cnt, pos;
    double point;
    int sign;
    char value[5]={0,0,0,0,0};

    // determine the header to be transmitted
    sprintf( hdr, "%c%c%02d", STX,
                              TEMP_CTRL_PREAMBLE,
                              TEMP_CTRL_ADDRESS);

    // create seed for checksum string
    sprintf( seed, "%02d%02d", TEMP_CTRL_ADDRESS, TEMP_CTRL_MEAS_TEMP );

    // calculate checksum
    cksm = temp_ctrl_checksum(seed);

    // create final packet
    sprintf( pkt, "%4s%02d%2s%c", hdr, 
                                  TEMP_CTRL_MEAS_TEMP,
                                  cksm,
                                  ETX );

    // send final packet
    cnt = com_write((HANDLE)ud, pkt);

    // check for errors
    sleep(200);
    rsp = com_read((HANDLE)ud);

    pos = strlen(hdr);

	if ((rsp == NULL) || ( rsp[pos] == 'N' )) return 0;
    //if ( rsp[pos] == 'N' ) return 0;
    else {
#ifndef _IQV
        point = pow(10, -1 * (rsp[pos + 2] & 0x3) );
#else // to avoid compiler error at LP
        point = pow(10.0, -1.0 * (rsp[pos + 2] & 0x3) );
#endif
        sign  = ( rsp[pos + 3] & 0x1 ? -1 : 1 );
        strncpy( value, rsp + pos + 4, 4);

        return sign * atof(value) * point;
    }
}

MANLIB_API unsigned long tempSet(const unsigned long ud, const int setpoint) {
    
    char hdr[5];
    char data[7];
    char seed[13];
    char *cksm;
    char pkt[18];
    char *rsp;
    unsigned long cnt, pos;
    // determine the header to be transmitted
    sprintf( hdr, "%c%c%02d", STX,
                              TEMP_CTRL_PREAMBLE,
                              TEMP_CTRL_ADDRESS);
    // determine pieces of packet to be transmitted
    sprintf( data, "%04d%02d", abs(setpoint) * 10, ( setpoint < 0 ? 11 : 0 ) );

    // create seed for checksum string
    sprintf( seed, "%02d%04d%6s", TEMP_CTRL_ADDRESS, TEMP_CTRL_SET_TEMP, data );

    // calculate checksum
    cksm = temp_ctrl_checksum(seed);

    // create final packet
    sprintf( pkt, "%4s%04d%6s%2s%c", hdr, 
                                     TEMP_CTRL_SET_TEMP,
                                     data,
                                     cksm,
                                     ETX );

    // send final packet
    cnt = com_write((HANDLE)ud, pkt);

    // check for errors
    sleep(200);
    rsp = com_read((HANDLE)ud);
    pos = strlen(hdr);

    if ((rsp == NULL) || ( rsp[pos] == 'N' )) return 0;
    else return cnt;
    
}

char *ps_channel(const int channel) {

    // convert enum to string
    switch (channel) {
        case PS_P25V :
            return "P25V";
        case PS_N25V :
            return "N25V";
        case PS_P6V :
        default :
            return "P6V";
    }

}

MANLIB_API int psInit(const int adr, const int model) {
#ifndef _IQV
    ps = ibdev (0, adr, 0, T30s, 1, 0);
    ps_model = model;
    return ps;
#else
	return 0;
#endif
}

MANLIB_API int psSetOutputState(const int ud, const int on_off) {

    gpibWrite( ud, qq( "OUTP %d", ( on_off == 0 ? 0 : 1 ) ) );
    return 1;

}

MANLIB_API int psSetVoltage(const int ud, 
                          const int channel, 
                          const double voltage,
                          const double current_limit) {

    gpibWrite( ud, qq("APPL %s,%.2f,%.3f", 
                     ps_channel(channel), voltage, current_limit) );
    return 1;

}

MANLIB_API double psMeasCurrent(const int ud, const int channel) { 

    return atof( gpibQuery( ud, qq("INST:NSEL %d;:MEAS:CURR?",channel), 20L ) );

}

char *att_code_110(const int value) {

    int i, yatten, xatten, extra;
    static char on[20]   = "A";        // on bits
    char        off[10]  = "B",        // off bits 
                digit[9] = "12345678"; // digits for strncat
    int         atten    = value;      // copy input

    // initialize string
    on[1] = '\0';

    // check for out of bounds
    if (atten <  0) atten = 0;
    if (atten > 121) atten = 121;

    // extract tens and ones place
    yatten = atten / 10;
    xatten = atten % 10;
    extra  = ( yatten >= 12 ? yatten - 11 : 0 );

    // carry y extra 
    yatten -= extra;
    xatten += extra * 10;

    // handle extra 40 dB with 110 dB switch
    if ( yatten > 7 ) { yatten -= 4 ; strncat(on, strchr(digit,'8'),1); }
    else                              strncat(off,strchr(digit,'8'),1);

    // build y string
    for (i = '7'; i >= '5'; i--) {

        ( yatten & 4 ? strncat(on, strchr(digit,i),1)
                     : strncat(off,strchr(digit,i),1) );
        yatten <<= 1;

    }

    // handle y extra carried over + 4th bit weighting
    if ( xatten > 7 ) { xatten -= 4 ; strncat(on, strchr(digit,'4'),1); }
    else                              strncat(off,strchr(digit,'4'),1);
    
    // build remaining x string
    for (i = '3'; i >= '1'; i--) {

        ( xatten & 4 ? strncat(on, strchr(digit,i),1)
                     : strncat(off,strchr(digit,i),1) );
        xatten <<= 1;

    }

    return strcat(on,off);

}

char *att_code(const int value) {

    int i, yatten, xatten, extra;
    static char on[20]   = "A";        // on bits
    char        off[10]  = "B",        // off bits 
                digit[9] = "12345678"; // digits for strncat
    int         atten    = value;      // copy input

    if (att_model == ATT_11713A_110) {
      return(att_code_110(value));
    }

    // initialize string
    on[1] = '\0';

    // check for out of bounds
    if (atten <  0) atten = 0;
    if (atten > 81) atten = 81;

    // extract tens and ones place
    yatten = atten / 10;
    xatten = atten % 10;
    extra  = ( yatten >= 8 ? yatten - 7 : 0 );

    // carry y extra 
    yatten -= extra;
    xatten += extra * 10;

    // build y string
    for (i = '8'; i >= '5'; i--) {

        ( yatten & 8 ? strncat(on, strchr(digit,i),1)
                     : strncat(off,strchr(digit,i),1) );
        yatten <<= 1;

    }

    // handle y extra carried over + 4th bit weighting
    if ( xatten > 7 ) { xatten -= 4 ; strncat(on, strchr(digit,'4'),1); }
    else                              strncat(off,strchr(digit,'4'),1);
    
    // build remaining x string
    for (i = '3'; i >= '1'; i--) {

        ( xatten & 4 ? strncat(on, strchr(digit,i),1)
                     : strncat(off,strchr(digit,i),1) );
        xatten <<= 1;

    }

    return strcat(on,off);

}

MANLIB_API int dmmInit(const int adr, const int model) {

	int dmm;
	int dmm_model;

#ifndef _IQV
	dmm = ibdev( 0, adr, 0, T30s, 1, 0 );
	dmm_model = model;
	return dmm;
#else
	return 0;
#endif
}

MANLIB_API double dmmMeasCurr(const int ud, const int model) {
	
	char *buf;
	int bytes2Rd;

	bytes2Rd = 20L;
	switch(model)
	{
		case DMM_FLUKE_45:
			gpibWrite(ud, "*RST; ADC; RATE F; TRIGGER 2;" );
			buf = gpibQuery(ud, "*TRG; VAL1?", bytes2Rd);
			
			return atof(buf);
			break;
		default:
			gpibWrite(ud, "*RST; ADC; RATE F; TRIGGER 2;" );
			buf = gpibQuery(ud, "*TRG; VAL1?", bytes2Rd);
			return atof(buf);//temporary
		
	}

}

/* return a handle (device descriptor) for the specified attenuator */
MANLIB_API int attInit(const int adr, const int model) {
#ifndef _IQV
    att = ibdev (0, adr, 0, T30s, 1, 0);
    att_model = model;
    return att;
#else
	return 0;
#endif
}

MANLIB_API int attSet(const int ud, const int value) {

    gpibWrite(ud, att_code(value));
    return 1;

}

MANLIB_API int pmInit(const int ud, const int model) {

	if (model == NRP_Z11) {
		pm_model = model;
#ifndef LINUX
		reset_nrp();
#endif
		return 0;
	} else {
#ifndef _IQV
#if (1) /* improved path */               
        pm = ibdev (0, ud, 0, T30s, 1, 0);
        gpibWrite(ud, "*RST\n"); /* reset */      

        pm_model = model;
        return(pm);
#else   /* super FAST path */ /* doesn't work yet, readings are inaccurate */          
        pm = ibdev (0, ud, 0, T30s, 1, 0);
        gpibWrite(ud, "*RST\n"); /* reset */      
        gpibWrite(ud, "CONF1:POW DEF, 2, (@1)\n"); /* config to defaults */
        gpibWrite(ud, "SENS1:MRAT FAST\n");        /* set # readings/sec */
        //gpibWrite(ud, "TRIG1:DEL:AUTO OFF\n");   /* set trigger source */
        gpibWrite(ud, "TRIG1:SOUR IMM\n");         /* set trigger source */
        pm_model = model;
        return(pm);
#endif
#else
	return 0;
#endif
	}
}

MANLIB_API int pmPreset(const int ud,
                        const double trigger_level,
                        const double trigger_delay,
                        const double gate_start,
                        const double gate_length,
                        const double trace_display_length) {

    switch (pm_model) {
        case PM_E4416A :
            gpibWrite( ud, qq(":disp:wind1:form dnum;num1:res 2;:disp:wind1:num2:res 2;:disp:wind2:form trac;num1:res 1;:calc1:feed \x22pow:aver\x22;:calc3:feed \x22pow:peak\x22;:aver:stat 1;coun 1;coun:auto 0;:aver2:stat 0;:pow:ac:rang:auto 0;:swe:time %f;offs:time %f;:sens:trac:time %f;:init:cont:seq 0;:trig:sour int;del %f;lev %f;lev:auto 0",
                             gate_length, gate_start, trace_display_length, trigger_delay, trigger_level) );
            return 1;

        default:

            return 0;

    }
                    
}

MANLIB_API double pmMeasAvgPower(const int ud, const int reset) {

    char power[10];                 // 436A
    int i = 0, err = 0x0;
    char *buf, *channel, *reading;  // E4416A
	double retval = -120.;	

    switch (pm_model) {
        case PM_436A :
			buf = gpibQuery(ud, "9D-V", 14L);
			if (strlen(buf)==14) {
				strncpy(power, buf + 3, 9L);
					// 9        range             = auto
					// D        units             = dBm
					// -        disable calfactor = false
					// V        rate              = delayed freerun
        
				// check for garbage
				for (i = 1; i <=4 ; i++) { err = err | ( isdigit(power[i]) ? 0x0 : 0x1); }
				// return average power
				retval = atof(power);
				return (err ? -120 : retval);

			} else {
				printf("SNOOP: buf = --|%s|--\n", buf);
				return -120;
			}
        case PM_E4416A :
#if (1)     /* improved path */  
            gpibWrite(ud, "CONF1:POW DEF, 2, (@1)\n"); /* config to defaults */
            gpibWrite(ud, "SENS1:MRAT FAST\n");        /* set # readings/sec */

            // take a reading
            buf = gpibQuery(ud, "READ1:POW? DEF, 2, (@1)\n");

            // parse output
            reading = strtok(buf,",");
            
            return(reading == NULL ? -120. : atof(reading));
#else   /* super FAST path */ /* doesn't work yet, readings are inaccurate */         
            // take a reading
            //gpibQuery(ud, "INIT1:IMM\n");
            buf = gpibQuery(ud, "FETC1:POW? DEF, 2, (@1)\n");

            // parse output
            reading = strtok(buf,",");

            return(reading == NULL ? -120. : atof(reading));
#endif            
        case PM_4531 :

            if (reset) {
                gpibWrite(ud, "MEM:SYS1:LOAD;\n");
                gpibWrite(ud, "SENS1:AVER4096;\n");
            }

            // take a reading
            buf = gpibQuery(ud, "MEAS1:POW?\n", 20L);

            // parse output
            channel = strtok(buf,",");
            reading = strtok(NULL, ",");

            return (reading == NULL ? -120. : atof(reading));

		case NRP_Z11 :

#ifndef LINUX
                retval = aquire_data();
#endif
			return retval;

        default:
            return -120.;

    }

}

int spa_sweep_points() {
    switch(spa_model) {
	    case SPA_E4404B :
            return SPA_E4404B_MAX_SWEEP_POINTS;
        case SPA_R3162 :
            return SPA_R3162_MAX_SWEEP_POINTS;
        case SPA_8595E :
            return SPA_8595E_MAX_SWEEP_POINTS;
	    case SPA_FSL :
            return SPA_FSL_MAX_SWEEP_POINTS;
        default:
            return SPA_E4404B_MAX_SWEEP_POINTS;

    }

}

MANLIB_API int spaInit(const int adr, const int model) {
#ifndef _IQV
    spa = ibdev (0, adr, 0, T30s, 1, 0);
    spa_model = model;

    switch(spa_model) {
        case SPA_E4404B :
        case SPA_FSL :
            atoi(gpibQuery(adr, "*RST;*OPC?", 5L)); /* reset */
        break;
        default:
            /* model unknown */
        break;
    }

    return spa;
#else
	return 0;
#endif

}

MANLIB_API char *spaMeasPhaseNoise(const int ud, 
                                   const double center, 
                                   const double ref_level,
                                   const int reset) {

    int n, rsp = 0;
    double ns0, ns1, ns2, ns3, ns4, ns5, ns6, ns7, ns8, ns9, ns10, ns11;
    static char ph_noise[107] =
                         "+999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99, +999.99";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));      // center freq
    gpibWrite(ud, ":FREQ:SPAN 3E5;");                 // set span
    gpibWrite(ud, ":BAND:RES 1E3;VID 1E3;");          // resolution / video bandwidth
    gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
    gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");      // scale
    gpibWrite(ud, ":INIT:CONT 0;");                   // single sweep
    gpibWrite(ud, ":AVER 0;");                        // averaging off
    gpibWrite(ud, ":DET SAMP;");                      // detector mode (sample)
    gpibWrite(ud, ":CALC:MARK:MODE POS;");            // marker mode (position)
    gpibWrite(ud, ":CALC:MARK:FUNC OFF;");            // marker function (off)
    gpibWrite(ud, ":FREQ:CENT:STEP:AUTO 0;");         // center freq step size manual
    gpibWrite(ud, ":TRAC:MODE WRIT;");                // max hold off
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find max peak
    gpibWrite(ud, ":CALC:MARK:CENT;");                // center marker
    gpibWrite(ud, ":FREQ:SPAN 1E5;");                 // change span
    gpibWrite(ud, ":AVER:COUN 10;STATE 1;");          // take 10 averages
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find max peak
    gpibWrite(ud, ":CALC:MARK:CENT;");                // recenter marker
    gpibWrite(ud, ":CALC:MARK:MODE DELT;");           // marker mode (delta)
    gpibWrite(ud, ":CALC:MARK:FUNC NOIS;");           // marker function (noise measurement)
    gpibWrite(ud, ":CALC:MARK:X 1E4;");               // move marker to 10 kHz
    ns0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":CALC:MARK:X 3E4;");               // move marker to 30 kHz
    ns1 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 6E4;");            // center freq step size 60 kHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 6E4;");               // move marker to 60 kHz
    ns2 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 4E4;");            // center freq step size 40 kHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 1E5;");               // move marker to 100 kHz
    ns3 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 2E5;");            // center freq step size 200 kHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 3E5;");               // move marker to 300 kHz
    ns4 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 3E5;");            // center freq step size 300 kHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 6E5;");               // move marker to 600 kHz
    ns5 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 4E5;");            // center freq step size 400 kHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 1E6;");               // move marker to 1 MHz
    ns6 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 2E6;");            // center freq step size 2 MHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 3E6;");               // move marker to 3 MHz
    ns7 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 3E6;");            // center freq step size 3 MHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 6E6;");               // move marker to 6 MHz
    ns8 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 4E6;");            // center freq step size 4 MHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 1E7;");               // move marker to 10 MHz
    ns9 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 2E7;");            // center freq step size 20 MHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 3E7;");               // move marker to 30 MHz
    ns10 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude delta
    gpibWrite(ud, ":FREQ:CENT:STEP 3E7;");            // center freq step size 30 MHz
    gpibWrite(ud, ":FREQ:CENT UP;");                  // increment center freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // trigger sweep
    gpibWrite(ud, ":CALC:MARK:X 6E7;");               // move marker to 60 MHz
    ns11 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude delta


    n = sprintf(ph_noise, "%06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f, %06.2f",
                          ns0, ns1, ns2, ns3, ns4, ns5, ns6, ns7, ns8, ns9, ns10, ns11);
    ph_noise[n] = '\0';

    return ph_noise;
}

MANLIB_API char *spaMeasSpectralFlatness(const int ud, 
                                         const double center, 
                                         const double ref_level,
                                         const int reset) {

    int n, sc, worst_car_low = 0, worst_car_high = 0, rsp = 0;
    double max_peak, thresh, sub_car, total_amp, avg_amp, max_dev, deviation_low = 0, deviation_high = 0;
    double sc_amp[52];
    static char flat_data[25] = "-99.99, -99, -99.99, -99";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));

    if (reset) {
       gpibWrite(ud, ":FREQ:SPAN 17E6;");                          // span
       gpibWrite(ud, ":BAND:RES 1E5;VID 1E3;");                    // resolution / video bandwidth
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
       gpibWrite(ud, ":DET SAMP;");                                // detector mode (sample)
       gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
    }

    // Make Spectral Flatness Measurement
    gpibWrite(ud, ":AVER:COUN 10;STATE 1;");                   // take 10 averages
    gpibWrite(ud, ":INIT:IMM;*WAI;");                          // trigger sweep

    gpibWrite(ud, ":CALC:MARK:MAX;");                          // find max peak
    max_peak = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));      // measure peak amplitude
    thresh = max_peak - 3;                                     // calculate peak threshold
    gpibWrite(ud, qq(":CALC:MARK:PEAK:THR %f;", thresh));      // define peak threshold
    gpibWrite(ud, ":CALC:MARK:PEAK:EXC 1;");                   // define peak excursion
    gpibWrite(ud, ":CALC:MARK:MODE POS;");                     // marker mode (position)

    gpibWrite(ud, qq(":CALC:MARK:X %f;", center));             // marker to center frequency
    gpibWrite(ud, ":CALC:MARK:MAX:RIGH;");                     // find subcarrier +1
    sub_car = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));       // measure frequency of subcarrier +1
    sc_amp[0] = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));     // measure amplitude of subcarrier +1
    for (sc=1; sc<26; sc++) {
       sub_car = sub_car + 312500;                             // calculate frequency of next subcarrier
       gpibWrite(ud, qq(":CALC:MARK:X %f;", sub_car));         // marker to next subcarrier
       sc_amp[sc] = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude of next subcarrier
    }
    gpibWrite(ud, qq(":CALC:MARK:X %f;", center));             // marker to center frequency
    gpibWrite(ud, ":CALC:MARK:MAX:LEFT;");                     // find subcarrier -1
    sub_car = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));       // measure frequency of subcarrier -1
    sc_amp[26] = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));    // measure amplitude of subcarrier -1
    for (sc=27; sc<52; sc++) {
       sub_car = sub_car - 312500;                             // calculate frequency of next subcarrier
       gpibWrite(ud, qq(":CALC:MARK:X %f;", sub_car));         // marker to next subcarrier
       sc_amp[sc] = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude of next subcarrier
    }
    total_amp = 0;
    for (sc=0; sc<16; sc++) {
       total_amp = total_amp + sc_amp[sc];
    }
    for (sc=26; sc<42; sc++) {
       total_amp = total_amp + sc_amp[sc];
    }
    avg_amp = total_amp/32;
    max_dev = 0;
    for (sc=0; sc<16; sc++) {
       if (fabs(sc_amp[sc] - avg_amp) > max_dev) {
          max_dev = fabs(sc_amp[sc] - avg_amp);
          deviation_low = sc_amp[sc] - avg_amp;
          worst_car_low = sc + 1;
       }
    }
    for (sc=26; sc<42; sc++) {
       if (fabs(sc_amp[sc] - avg_amp) > max_dev) {
          max_dev = fabs(sc_amp[sc] - avg_amp);
          deviation_low = sc_amp[sc] - avg_amp;
          worst_car_low = sc + 1;
       }
    }
    if (worst_car_low > 26)
       worst_car_low = 26 - worst_car_low;
    max_dev = 0;
    for (sc=16; sc<26; sc++) {
       if (fabs(sc_amp[sc] - avg_amp) > max_dev) {
          max_dev = fabs(sc_amp[sc] - avg_amp);
          deviation_high = sc_amp[sc] - avg_amp;
          worst_car_high = sc + 1;
       }
    }
    for (sc=42; sc<52; sc++) {
       if (fabs(sc_amp[sc] - avg_amp) > max_dev) {
          max_dev = fabs(sc_amp[sc] - avg_amp);
          deviation_high = sc_amp[sc] - avg_amp;
          worst_car_high = sc + 1;
       }
    }
    if (worst_car_high > 26)
       worst_car_high = 26 - worst_car_high;

    n = sprintf(flat_data, "%06.2f, %03d, %06.2f, %03d", deviation_low, worst_car_low, deviation_high, worst_car_high);
    flat_data[n] = '\0';

    return flat_data;
}

/* Should only be used if tx is in continuous mode */
MANLIB_API double spaMeasChannelPower(const int ud, 
                                      const double center, 
                                      const double span, 
                                      const double ref_level,
                                      const double rbw,
                                      const double vbw,
                                      const double int_bw,
                                      const int det_mode,
                                      const int averages, 
                                      const int max_hold, 
                                      const int reset) {

    double sum = 0, avg = 0, scale = 0;
    double *trace;
    const double kn = 0.9;
    int rsp, i, n = 0, iSwpPnts = spa_sweep_points();

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));

    if (reset) {
        gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                  // span
        gpibWrite(ud, qq(":BAND:RES %f;VID %f;", rbw, vbw));        // resolution / video bandwidth
        gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
        gpibWrite(ud,    ":DISP:WIND:TRAC:Y:PDIV 5;");              // scale
        gpibWrite(ud, (averages == 0 ?  (char *)":AVER 0;" :                // num averages
                      qq(":AVER:COUN %u;STATE 1;", averages)));
        gpibWrite(ud, qq(":DET %s;", (det_mode == 1 ? "NEG" :       // detector mode
                                     (det_mode == 2 ? "POS" : 
                                               "SAMP"))));
        gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
        gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
    }

    if (max_hold) {
       gpibWrite(ud, ":TRAC:MODE MAXH;");              // max hold on
    }
    gpibWrite(ud, ":INIT:IMM;*WAI;");                  // trigger sweep

    // return the channel power
    trace = spa_get_trace(spa);
    for (i = 0; i < iSwpPnts ; i++) {
        sum += pow(10,(trace[i]/10));
        n++;
    }
    avg = sum / n;
    scale = int_bw / (rbw * kn);

    gpibWrite(ud, ":TRAC:MODE WRIT;");                 // max hold off

    return ( 10 * log10( scale * avg ) );
}

MANLIB_API double spaMeasSpectralDensity(const int ud, 
                                         const double center, 
                                         const double ref_level,
                                         const int reset) {

    int rsp = 0;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));

    if (reset) {
       gpibWrite(ud, ":FREQ:SPAN 4E7;");                           // span
       gpibWrite(ud, ":BAND:RES 1E6;VID 3E6;");                    // resolution / video bandwidth
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
       gpibWrite(ud, ":DET SAMP;");                                // detector mode (sample)
       gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
    }

    // Make Power Spectral Density Measurement
    gpibWrite(ud, ":AVER:COUN 100;STATE 1;");         // take 100 averages
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find peak
    return atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // return amplitude
}

MANLIB_API char *spaMeasOutOfBandEm(const int ud, 
                                    const double ref_level,
                                    const int reset) {

    int n, rsp = 0;
    double pwr0, pwr1, pwr2, pwr3, freq0, freq1, freq2, freq3;
    static char emissions[83] = "-120.00, 0.000E+000, -120.00, 0.000E+000, -120.00, 0.000E+000, -120.00, 0.000E+000";

    if (reset) {
       rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
       gpibWrite(ud, ":FREQ:STAR 5.13E9;");                        // start frequency
       gpibWrite(ud, ":FREQ:STOP 5.142E9;");                       // stop frequency
       gpibWrite(ud, ":BAND:RES 1E6;VID 1E4;");                    // resolution / video bandwidth
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
    }

    // Make measurement
    // 5.13 to 5.142 GHz
    gpibWrite(ud, ":FREQ:STAR 5.13E9;");              // start frequency
    gpibWrite(ud, ":FREQ:STOP 5.142E9;");             // stop frequency
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find peak
    pwr0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude
    freq0 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L)); // measure frequency

    // 5.142 to 5.15 GHz
    gpibWrite(ud, ":FREQ:STAR 5.142E9;");             // start frequency
    gpibWrite(ud, ":FREQ:STOP 5.15E9;");              // stop frequency
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find peak
    pwr1 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude
    freq1 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L)); // measure frequency

    // 5.25 to 5.258 GHz
    gpibWrite(ud, ":FREQ:STAR 5.25E9;");              // start frequency
    gpibWrite(ud, ":FREQ:STOP 5.258E9;");             // stop frequency
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find peak
    pwr2 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude
    freq2 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L)); // measure frequency

    // 5.258 to 5.27 GHz
    gpibWrite(ud, ":FREQ:STAR 5.258E9;");             // start frequency
    gpibWrite(ud, ":FREQ:STOP 5.27E9;");              // stop frequency
    gpibWrite(ud, ":INIT:IMM;*WAI;");                 // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                 // find peak
    pwr3 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure amplitude
    freq3 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L)); // measure frequency

    n = sprintf(emissions, "%07.2f, %10.3E, %07.2f, %10.3E, %07.2f, %10.3E, %07.2f, %10.3E",
                            pwr0, freq0, pwr1, freq1, pwr2, freq2, pwr3, freq3);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API char *spaGetModelNo(const int ud, 
                                         const int reset) {

    static char *sa_model;
    int rsp = 0;

    if (reset) {
       rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));                // reset
    }

    // Read model number
    sa_model = gpibQuery(ud, "*IDN?", 50L);

    return sa_model;
}

//++JC++
MANLIB_API double spaMeasHarmonics(const int ud,
                                        const double center,
                                        const double vbw,
                                        const double ref_level,
                                        const int reset) {

    int n, rsp = 0;
    double pwr0, freq0;
    static char emissions[21] = "-120.00, 0.000E+000";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                   // center frequency

    if (reset) {
       gpibWrite(ud, ":FREQ:SPAN 50E6;");                         // span
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 1E6;");                            // resolution bandwidth
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
    }

    gpibWrite(ud, qq(":BAND:VID %f;", vbw));                       // video bandwidth
    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":TRAC:MODE MAXH;");                             // max hold on
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak
    pwr0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude
    freq0 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));             // measure frequency


    n = sprintf(emissions, "%07.2f, %10.3E",
                            pwr0, freq0);
    emissions[n] = '\0';

    return pwr0;
}

MANLIB_API double spaMeasTelecEmission2(const int ud,
                                        const double freq_start,
                                        const double freq_stop,
                                        const double ref_level,
                                        const int reset) {

    int n, rsp = 0;
    double pwr0, freq0;
    static char emissions[21] = "-120.00, 0.000E+000";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

printf("Start: %f\n", freq_start);
printf("Stop: %f\n", freq_stop);
    gpibWrite(ud, qq(":FREQ:STAR %f;", freq_start));                          // start frequency
    gpibWrite(ud, qq(":FREQ:STOP %f;", freq_stop));                           // stop frequency

    if (reset) {
       gpibWrite(ud, ":POW:ATT 0;");                               // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 30E3;VID 30E3;");                    // resolution / video bandwidth
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":DET SAMP;");                                   // detector mode (sample)
    }

    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":AVER:COUN 10;STATE 1;");                       // take 10 averages
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak

    pwr0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude
    freq0 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));             // measure frequency


    n = sprintf(emissions, "%07.2f, %10.3E",
                            pwr0, freq0);
    emissions[n] = '\0';

    return pwr0;
}

MANLIB_API double spaMeasTelecSpurious(const int ud,
                                        const double freq_start,
                                        const double freq_stop,
                                        const double ref_level,
                                        const int reset) {

    int n, rsp = 0;
    double pwr0, freq0;
    static char emissions[21] = "-120.00, 0.000E+000";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

printf("Start: %f\n", freq_start);
printf("Stop: %f\n", freq_stop);
    gpibWrite(ud, qq(":FREQ:STAR %f;", freq_start));                          // stop frequency
    gpibWrite(ud, qq(":FREQ:STOP %f;", freq_stop));                           // stop frequency

    if (reset) {
       gpibWrite(ud, ":POW:ATT 0;");                               // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 1E6;VID 1E6;");                    // resolution / video bandwidth
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
    }

    gpibWrite(ud, ":DET POS;");                                    // detector mode (peak)
    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak

    gpibWrite(ud, ":CALC:MARK:SET:CENT;");                         // set center to the marker position
    gpibWrite(ud, ":FREQ:SPAN 0;");                                // span
    gpibWrite(ud, ":DET SAMP;");                                   // detector mode (sample)
    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak

    pwr0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude
    freq0 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));             // measure frequency


    n = sprintf(emissions, "%07.2f, %10.3E",
                            pwr0, freq0);
    emissions[n] = '\0';

    return pwr0;
}

MANLIB_API double spaMeasBandEdge(const int ud,
                                        const double freq_start,
                                        const double freq_stop,
                                        const double vbw,
                                        const double ref_level,
                                        const int reset) {

    int n, rsp = 0;
    double pwr0, freq0;
    static char emissions[21] = "-120.00, 0.000E+000";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:STAR %f;", freq_start));                          // stop frequency
    gpibWrite(ud, qq(":FREQ:STOP %f;", freq_stop));                           // stop frequency

    if (reset) {
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 1E6;");                            // resolution bandwidth
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
    }

    gpibWrite(ud, qq(":BAND:VID %f;", vbw));                       // video bandwidth
    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":TRAC:MODE MAXH;");                             // max hold on
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak
    pwr0 = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude
    freq0 = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));             // measure frequency


    n = sprintf(emissions, "%07.2f, %10.3E",
                            pwr0, freq0);
    emissions[n] = '\0';

    return pwr0;
}

MANLIB_API double spaMeasPpsdUnii(const int ud,
                                        const double center,
                                        const double span, 
                                        const double ref_level,
                                        const int reset) {

    int rsp = 0;
    //++JC++ int avg_count = 100;
    int avg_count = 20;                                            // 100 averages takes too much time

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                   // center frequency

    if (reset) {
       gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                  // span
       gpibWrite(ud, ":POW:ATT 20;");                              // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 1E6;VID 3E6;");                    // resolution / video bandwidth
       gpibWrite(ud, ":DET SAMP;");                                // detector mode (sample)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":AVER:TYPE POW;");                           // power averaging 
    }

    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, qq(":AVER:COUN %i;STATE 1;", avg_count));        // take 20 averages
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // single sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak
    return atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude
}

MANLIB_API double spaMeasPpsdDts(const int ud,
                                        const double center,
                                        const double span, 
                                        const double ref_level,
                                        const int reset) {

    int rsp = 0;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                   // center frequency

    if (reset) {
       gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                  // span
       gpibWrite(ud, ":POW:ATT 20;");                              // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 3E3;VID 10E3;");                   // resolution / video bandwidth
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
    }

    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":TRAC:MODE MAXH;");                             // max hold on
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak
    return atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));              // measure amplitude

}

MANLIB_API double spaMeasEbw(const int ud,
                                        const double center,
                                        const double span, 
                                        const double ref_level,
                                        const int reset) {

    int rsp = 0;
    double delta, pos;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                   // center frequency

    if (reset) {
       gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                  // span
       gpibWrite(ud, ":POW:ATT 20;");                              // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":BAND:RES 100E3;VID 1E3;");                  // resolution / video bandwidth
                                                                   // VBW in spec is 1MHz but as this is a
                                                                   // relative measurement it does not matter
                                                                   // VBW=1KHz gives more accurate measurement
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
    }

    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    gpibWrite(ud, ":INIT:IMM;*WAI;");                              // trigger sweep
    gpibWrite(ud, ":TRAC:MODE VIEW;");                             // view
    gpibWrite(ud, ":CALC:MARK:MODE POS;");                         // marker mode (position)
    gpibWrite(ud, ":CALC:MARK:MAX;");                              // find peak
    gpibWrite(ud, ":CALC:MARK:MODE DELT;");                        // marker mode (delta)
    gpibWrite(ud, qq(":CALC:MARK:X %f;", -span));                  // move marker to one end
    delta = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));             // measure amplitude delta
    pos = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));               // measure position delta

    while (delta < -26) {
       pos = pos + 500E3;
       gpibWrite(ud, qq(":CALC:MARK:X %f;", pos));                 // move marker forward by 500KHz
       delta = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));          // measure amplitude delta
    }

    gpibWrite(ud, ":CALC:MARK:MODE DELT;");                        // marker mode (delta)
    gpibWrite(ud, qq(":CALC:MARK:X %f;", span));                   // move marker to other end
    pos = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));               // measure position delta
    delta = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));             // measure amplitude delta

    while (delta < 0) {
       pos = pos - 500E3;
       gpibWrite(ud, qq(":CALC:MARK:X %f;", pos));                 // move marker backward by 500KHz
       delta = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));          // measure amplitude delta
    }

    return atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));              // measure EB

}

MANLIB_API double spaMeasPkPwr(const int ud,
                                        const double center,
                                        const double vbw, 
                                        const double ebw, 
                                        const double ref_level,
                                        const int reset) {

    int rsp = 0;
    double ch_pwr;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                   // center frequency

    if (reset) {
       gpibWrite(ud, ":POW:ATT 30;");                              // Attenuation
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
    }

    gpibWrite(ud, ":TRAC:MODE WRIT;");                             // clear write
    ch_pwr = atof(gpibQuery(ud, ":MEAS:CHP:CHP?", 55L));           // Set the instrument mode
    gpibWrite(ud, ":DET POS;");                                    // detector mode (peak)
    gpibWrite(ud, qq(":CHP:BWID:INT %f;", ebw));                   // Integration BW
    gpibWrite(ud, qq(":CHP:FREQ:SPAN %f;", 2*ebw));                // span
    gpibWrite(ud, ":BAND:RES 1E6;");                               // resolution bandwidth
    gpibWrite(ud, qq(":BAND:VID %f;", vbw));                       // video bandwidth
    sleep(10000);
    ch_pwr = atof(gpibQuery(ud, ":READ:CHP:CHP?", 55L));           // Read channel power
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    return ch_pwr;                                                 // Return channel power

}

MANLIB_API double pmMeasPeakPower(const int ud, const int reset) {

   char *buf, /*string_holder,*/ *reading;  // E4416A

   if (reset) gpibWrite(ud, "*RCL 5;\n");
   // take a reading
   gpibWrite(ud, ":INIT:CONT ON;");
   gpibWrite(ud, ":TRIG:SOUR INT;");
   gpibWrite(ud, ":TRIG:LEV:AUTO OFF;");
   gpibWrite(ud, ":TRIG:LEV -20.00DBM;");
   gpibWrite(ud, ":DISP:WIND2:FORM DNUM;");
   gpibWrite(ud, "CALC2:FEED1 'POW:PEAK';");

   buf = gpibQuery(ud, "FETCH2:POW?\n", 20L);
			
   // parse output
   reading = strtok(buf,",");

   return (reading == NULL ? -120. : atof(reading));

}

// This subroutine returns the frequency and levels of upto 5 spurs in the specified band which are above the
// specified threshold
MANLIB_API char *spaMeasTxSpurs(const int ud, 
                                   const double freq_start, 
                                   const double freq_stop,
                                   const double spur_thresh,
                                   double *spur_level,
                                   double *spur_freq,
                                   const int reset) 
{

    int n, rsp = 0;
    int i = 0;
    int over = 0;
//    double spur_level[5] = {-120.00, -120.00, -120.00, -120.00, -120.00};
//    double spur_freq[5] = {0.000E+000, 0.000E+000, 0.000E+000, 0.000E+000, 0.000E+000};
    double current_spur_level, current_spur_freq;

    static char tx_spurs[105];

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:STAR %f;", freq_start));                          // stop frequency
    gpibWrite(ud, qq(":FREQ:STOP %f;", freq_stop));                           // stop frequency
    gpibWrite(ud, ":BAND:RES 1E6;VID 1E3;");                                  // resolution / video bandwidth
    gpibWrite(ud, ":POW:ATT 0;");                                             // Attenuation
    gpibWrite(ud, ":DISP:WIND:TRAC:Y:RLEV 0;");                               // reference level
    gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                              // scale
    gpibWrite(ud, ":INIT:CONT 0;");                                           // single sweep
    gpibWrite(ud, ":AVER 0;");                                                // averaging off
    gpibWrite(ud, ":DET SAMP;");                                              // detector mode (sample)

    gpibWrite(ud, ":CALC:MARK:MODE POS;");                                    // marker mode (position)
    gpibWrite(ud, ":TRAC:MODE WRIT;");                                        // max hold off
    gpibWrite(ud, ":INIT:IMM;*WAI;");                                         // trigger sweep
    gpibWrite(ud, ":CALC:MARK:MAX;");                                         // find max peak
    current_spur_level = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));           // measure amplitude
    current_spur_freq = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));            // measure position
    
    while ((i < 5) && (current_spur_level > spur_thresh) && (over != 1)) {                   // Measure upto 5 spurs that are above the threshold
       spur_level[i] = current_spur_level;
       spur_freq[i] = current_spur_freq;
       gpibWrite(ud, ":CALC:MARK:MAX:NEXT;");                                 // find next peak
       current_spur_level = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));        // measure amplitude
       current_spur_freq = atof(gpibQuery(ud, ":CALC:MARK:X?", 55L));         // measure position

       if (fabs(current_spur_freq - spur_freq[i]) < 10E6) {                    // No more new peaks
           over = 1;
       }
       if (fabs(current_spur_freq - spur_freq[i]) < 100E6) {                   // Skip nearby peaks
           continue;
       }
       i++;
    }

    n = sprintf(tx_spurs, "%07.2f, %10.3E; %07.2f, %10.3E; %07.2f, %10.3E; %07.2f, %10.3E; %07.2f, %10.3E",
                            spur_level[0], spur_freq[0], spur_level[1], spur_freq[1], spur_level[2], spur_freq[2], spur_level[3], spur_freq[3], spur_level[4], spur_freq[4]);
    tx_spurs[n] = '\0';

    return tx_spurs;
}
//++JC++

MANLIB_API double spaMeasPkAvgRatio(const int ud, 
                                    const double center, 
                                    const double ref_level,
                                    const int reset) {

    int rsp = 0;
    double peak = 0, avg = 0;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));

    if (reset) {
       gpibWrite(ud, ":FREQ:SPAN 4E7;");                           // span
       gpibWrite(ud, ":BAND:RES 1E6;VID 1E6;");                    // resolution / video bandwidth
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
    }

    // Make Peak Measurement
    gpibWrite(ud, ":INIT:CONT 0;");                    // single sweep
    gpibWrite(ud, ":BAND:VID 1E6;");                   // video bandwidth
    gpibWrite(ud, ":TRAC:MODE MAXH;");                 // max hold on
    gpibWrite(ud, ":INIT:CONT 1;");                    // continuous sweep
    sleep (10000);                                     // wait for trace to build up
    gpibWrite(ud, ":CALC:MARK:MAX;");                  // find peak
    peak = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));  // read amplitude
    gpibWrite(ud, ":INIT:CONT 0;");                    // single sweep
    gpibWrite(ud, ":TRAC:MODE WRIT;");                 // max hold off

    // Make Average Measurement
    gpibWrite(ud, ":BAND:VID 3E4;");                   // video bandwidth
    gpibWrite(ud, ":TRAC:MODE MAXH;");                 // max hold on
    gpibWrite(ud, ":INIT:CONT 1;");                    // continuous sweep
    sleep (10000);                                     // wait for trace to build up
    gpibWrite(ud, ":CALC:MARK:MAX;");                  // find peak
    avg = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L));   // read amplitude
    gpibWrite(ud, ":TRAC:MODE WRIT;");                 // max hold off
    return (peak - avg);                               // return peak to average ratio
}

MANLIB_API double spaMeasFreqDev(const int ud, 
                                 const double center, 
                                 const double ref_level,
                                 const int reset) {
    int rsp = 0;
    double meas_freq;


    switch (spa_model) {
		case SPA_FSL:
            rsp=atoi(gpibQuery(ud, "*RST;*OPC?", 5));
            gpibWrite(ud,"INIT;*WAI");  // Perform sweep with sync
            gpibWrite(ud, ":FREQ:CENT $center;");
            gpibWrite(ud, ":FREQ:SPAN 10E6;");     // span
            gpibWrite(ud, ":DISP:WIND:TRAC:Y:RLEV $ref_level;"); 
            gpibWrite(ud, ":BAND:RES 3E5;VID 3E3;");    // resolution/video
            gpibWrite(ud,"INIT:CONT OFF");              // Single sweep on
            gpibWrite(ud,"INIT;*WAI");                  //Perform sweep with sync
            gpibWrite(ud,"CALC:MARK:PEXC 6DB");         //Peak excursion
            gpibWrite(ud,"CALC:MARK:STAT ON");          //Marker 1 on
            gpibWrite(ud,"CALC:MARK:TRAC 1");           //Assign marker 1 to trace 1
            //gpibWrite(ud,"CALC:MARK:X $center");      //Set marker 1 to center MHz
            gpibWrite(ud,"CALC:MARK:COUNT:RES 1HZ");            // Frequency counter 1 Hz
            gpibWrite(ud,"CALC:MARK:COUNT ON");                   // Frequency counter on
            gpibWrite(ud,"INIT;*WAI");                             // Perform sweep with sync
            meas_freq = atof(gpibQuery(ud,":CALC:MARK:COUN:FREQ?",100)); // Query measured frequency

            break;

        default:
        // reset prior to setting the center frequency
        if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

        // change to channel frequency
        gpibWrite(ud, qq(":FREQ:CENT %f;", center));

        if (reset) {
#if (0) 	       
                gpibWrite(ud, ":FREQ:SPAN 10E6;");                           // span
                gpibWrite(ud, ":BAND:RES 3E5;VID 3E3;");                    // resolution / video bandwidth
                gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
                gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
                gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
//           gpibWrite(ud, ":AVER 0;");                                  // averaging off
                gpibWrite(ud,    ":AVER:COUN 7;STATE 1");              // average 1 times /* test time reduction, was 7 */
//           gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
                gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
                gpibWrite(ud, ":CALC:MARK:CPE ON;");                        // continuous peaking
                gpibWrite(ud, ":CALC:MARK:FCO:RES 100;");                   // frequency counter resolution
                gpibWrite(ud, ":CALC:MARK:FCO 1;");                         // frequency counter on
#else /* test time reduction */          
                gpibWrite(ud, ":FREQ:SPAN 10E6;");                           // span
                gpibWrite(ud, "SENS:BAND:RES:AUTO ON");                    // resolution bandwidth: auto
                gpibWrite(ud, "SENS:BAND:VID:AUTO ON");                    // resolution bandwidth: auto
   //           gpibWrite(ud, ":BAND:RES 3E5;VID 3E3;");                    // resolution / video bandwidth
                gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
                gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
                gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
//           gpibWrite(ud, ":AVER 0;");                                  // averaging off
                gpibWrite(ud,    ":AVER:COUN 1;STATE 1");              // average 1 times /* test time reduction, was 7 */
//           gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
                gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
                gpibWrite(ud, ":CALC:MARK:CPE ON;");                        // continuous peaking
                gpibWrite(ud, ":CALC:MARK:FCO:RES 100;");                   // frequency counter resolution
                gpibWrite(ud, ":CALC:MARK:FCO 1;");                         // frequency counter on
#endif           
        }

       // Make Frequency Measurement
        gpibWrite(ud, ":INIT:IMM;*WAI;");                          // single sweep
        meas_freq = atof(gpibQuery(ud, ":CALC:MARK:FCO:X?", 55L)); // get frequency
        break;
    }
    
    return ((meas_freq/center)-1)*1E6;                         // return ppm
}

MANLIB_API char *spaMeasTxSpuriousEm(const int ud, 
                                     const double ref_level,
                                     const int reset) {

    int rsp = 0, n, cf_index;
    double pwr, freq, pwr_max0, pwr_max1, pwr_max2, freq_max0, freq_max1, freq_max2;
    static char emissions[62] = "-120.00, 0.000E+000, -120.00, 0.000E+000, -120.00, 0.000E+000";

    if (reset) {
       rsp = atoi(gpibQuery(ud, "IP;DONE?", 5L));   // reset
       gpibWrite(ud, "SNGLS;");                     // single sweep
       gpibWrite(ud, "AT AUTO;");                   // auto attenuation
       gpibWrite(ud, qq("RL %fDB;", ref_level));    // reference level
       gpibWrite(ud, "LG 5DB;");                    // scale
       gpibWrite(ud, "FA 10MHZ;");                  // start freq
       gpibWrite(ud, "FB 1GHZ;");                   // stop freq
       gpibWrite(ud, "RB 1MHZ;");                   // resolution bandwidth
       gpibWrite(ud, "VB 10KHZ;");                  // video bandwidth
       gpibWrite(ud, "DET POS;");                   // detector mode (peak)
       gpibWrite(ud, "SS MAN;");                    // freq step size manual
       gpibWrite(ud, "SS 1GHZ;");                   // freq step size 1 GHz
    }

    // Make measurement
    // Search between 10 MHz and 1 GHz
    gpibWrite(ud, "FA 10MHZ;");
    gpibWrite(ud, "FB 1GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr_max0 = atof(gpibQuery(ud, "MKA?",55L));
    freq_max0 = atof(gpibQuery(ud, "MKF?",55L));

    // Search between 1 and 2 GHz
    gpibWrite(ud, "FA 1GHZ;");
    gpibWrite(ud, "FB 2GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 2 and 2.8 GHz
    gpibWrite(ud, "FA 2GHZ;");
    gpibWrite(ud, "FB 2.8GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 2.8 and 3 GHz
    gpibWrite(ud, "FA 2.8GHZ;");
    gpibWrite(ud, "FB 3GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 3 and 4 GHz
    gpibWrite(ud, "FA 3GHZ;");
    gpibWrite(ud, "FB 4GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 4 and 5 GHz
    gpibWrite(ud, "CF UP;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 5 and 5.13 GHz
    gpibWrite(ud, "FA 5GHZ;");
    gpibWrite(ud, "FB 5.13GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 5.27 and 5.342 GHz
    gpibWrite(ud, "FA 5.27GHZ;");
    gpibWrite(ud, "FB 5.342GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr_max1 = atof(gpibQuery(ud, "MKA?",55L));
    freq_max1 = atof(gpibQuery(ud, "MKF?",55L));

    // Search between 5.342 and 6 GHz
    gpibWrite(ud, "FA 5.342GHZ;");
    gpibWrite(ud, "FB 6GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr_max2 = atof(gpibQuery(ud, "MKA?",55L));
    freq_max2 = atof(gpibQuery(ud, "MKF?",55L));

    // Search between 6 and 7 GHz
    gpibWrite(ud, "FA 6GHZ;");
    gpibWrite(ud, "FB 7GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max2) {
       pwr_max2 = pwr;
       freq_max2 = freq;
    }

    // Search between 7 and 13 GHz
    for (cf_index=0;cf_index<6;cf_index++) {
       gpibWrite(ud, "CF UP;");
       gpibWrite(ud, "TS;");
       gpibQuery(ud, "DONE?",5L);
       gpibWrite(ud, "MKPK HI;");
       pwr = atof(gpibQuery(ud, "MKA?",55L));
       freq = atof(gpibQuery(ud, "MKF?",55L));
       if (pwr > pwr_max2) {
          pwr_max2 = pwr;
          freq_max2 = freq;
       }
    }

    // Search between 13 and 16 GHz
    for (cf_index=0;cf_index<3;cf_index++) {
       gpibWrite(ud, "CF UP;");
       gpibWrite(ud, "TS;");
       gpibQuery(ud, "DONE?",5L);
       gpibWrite(ud, "MKPK HI;");
       pwr = atof(gpibQuery(ud, "MKA?",55L));
       freq = atof(gpibQuery(ud, "MKF?",55L));
       if (pwr > pwr_max2) {
          pwr_max2 = pwr;
          freq_max2 = freq;
       }
    }

    n = sprintf(emissions, "%07.2f, %10.3E, %07.2f, %10.3E, %07.2f, %10.3E",
                            pwr_max0, freq_max0, pwr_max1, freq_max1, pwr_max2, freq_max2);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API char *spaMeasTxSpuriousEmLite(const int ud, 
                                         const double ref_level,
                                         const int reset) {

    int rsp = 0, n;
    double pwr, freq, pwr_max0, pwr_max1, pwr_max2, freq_max0, freq_max1, freq_max2;
    static char emissions[62] = "-120.00, 0.000E+000, -120.00, 0.000E+000, -120.00, 0.000E+000";

    if (reset) {
       rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));                // reset
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":BAND:RES 1E6;VID 1E4;");                    // resolution / video bandwidth
       gpibWrite(ud, ":FREQ:STAR 1E7;");                           // start freq
       gpibWrite(ud, ":FREQ:STOP 1E9;");                           // stop freq
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":CALC:MARK:PEAK:SEAR:MODE MAX");             // set peak search mode
       gpibWrite(ud, ":FREQ:CENT:STEP:AUTO 0;");                   // freq step auto off
       gpibWrite(ud, ":FREQ:CENT:STEP 1E9;");                      // freq step 1 GHz
    }

    // Make measurement
    // Search between 10 MHz and 1 GHz
    gpibWrite(ud, ":FREQ:STAR 1E7;");                         // start freq
    gpibWrite(ud, ":FREQ:STOP 1E9;");                         // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                         // trigger sweep
    pwr_max0 = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq_max0 = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak

    // Search between 1 GHz and 2 GHz
    gpibWrite(ud, ":FREQ:STAR 1E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 2E9;");                    // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 2 GHz and 2.8 GHz
    gpibWrite(ud, ":FREQ:STAR 2E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 2.8E9;");                  // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 2.8 GHz and 3 GHz
    gpibWrite(ud, ":FREQ:STAR 2.8E9;");                  // start freq
    gpibWrite(ud, ":FREQ:STOP 3E9;");                    // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 3 GHz and 4 GHz
    gpibWrite(ud, ":FREQ:STAR 3E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 4E9;");                    // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 4 GHz and 5 GHz
    gpibWrite(ud, ":FREQ:CENT UP;");                     // increment center freq 1 GHz
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 5 and 5.13 GHz
    gpibWrite(ud, ":FREQ:STAR 5E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 5.13E9;");                 // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max0) {
       pwr_max0 = pwr;
       freq_max0 = freq;
    }

    // Search between 5.27 and 5.342 GHz
    gpibWrite(ud, ":FREQ:STAR 5.27E9;");                      // start freq
    gpibWrite(ud, ":FREQ:STOP 5.342E9;");                     // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                         // trigger sweep
    pwr_max1 = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq_max1 = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak

    // Search between 5.342 and 6 GHz
    gpibWrite(ud, ":FREQ:STAR 5.342E9;");                     // start freq
    gpibWrite(ud, ":FREQ:STOP 6E9;");                         // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                         // trigger sweep
    pwr_max2 = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq_max2 = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak

    // Search between 6 and 6.7 GHz
    gpibWrite(ud, ":FREQ:STAR 6E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 6.7E9;");                  // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max2) {
       pwr_max2 = pwr;
       freq_max2 = freq;
    }

    n = sprintf(emissions, "%07.2f, %10.3E, %07.2f, %10.3E, %07.2f, %10.3E",
                            pwr_max0, freq_max0, pwr_max1, freq_max1, pwr_max2, freq_max2);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API double spaMeasTxPwrDev(const int ud, 
                                  const double center, 
                                  const double ref_level,
                                  const int reset) {

    int rsp = 0;
    double pwr_dev = 0;

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    // change to channel frequency
    gpibWrite(ud, qq(":FREQ:CENT %f;", center));

    if (reset) {
       gpibWrite(ud, ":FREQ:SPAN 25E6;");                          // span
       gpibWrite(ud, ":BAND:RES 1E6;VID 3E6;");                    // resolution / video bandwidth
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":AVER 0;");                                  // averaging off
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak) 
       gpibWrite(ud, ":TRAC:MODE WRIT;");                          // max hold off
    }

    // Make Power Measurement
    gpibWrite(ud, ":INIT:CONT 0;");                      // single sweep
    gpibWrite(ud, ":FREQ:SPAN 25E6;");                   // span
    gpibWrite(ud, ":BAND:VID 3E6;");                     // video bandwidth
    gpibWrite(ud, ":DET POS;");                          // detector mode (peak) 
    gpibWrite(ud, ":TRAC:MODE MAXH;");                   // max hold on
    gpibWrite(ud, ":INIT:CONT 1;");                      // continuous sweep
    sleep(10000);
    gpibWrite(ud, ":CALC:MARK:MAX;");                    // find peak
    gpibWrite(ud, ":CALC:MARK:CENT;");                   // center peak
    gpibWrite(ud, ":INIT:CONT 0;");                      // single sweep
    gpibWrite(ud, ":TRAC:MODE WRIT;");                   // max hold off
    gpibWrite(ud, ":FREQ:SPAN 0;");                      // span
    gpibWrite(ud, ":BAND:VID 1E6;");                     // video bandwidth
    gpibWrite(ud, ":DET SAMP;");                         // detector mode (sample) 
    gpibWrite(ud, ":TRAC:MODE MAXH;");                   // max hold on
    gpibWrite(ud, ":INIT:CONT 1;");                      // continuous sweep
    sleep(10000);
    gpibWrite(ud, ":CALC:MARK:MAX;");                    // find peak
    pwr_dev = atof(gpibQuery(ud, ":CALC:MARK:Y?", 55L)); // measure power
    gpibWrite(ud, ":TRAC:MODE WRIT;");                   // max hold off
    return pwr_dev;
}

MANLIB_API char *spaMeasRxSpuriousEm(const int ud, 
                                     const double ref_level,
                                     const int reset) {

    int rsp = 0, n, cf_index;
    double pwr, freq, pwr_max0, pwr_max1, freq_max0, freq_max1;
    static char emissions[41] = "-120.00, 0.000E+000, -120.00, 0.000E+000";

    if (reset) {
       rsp = atoi(gpibQuery(ud, "IP;DONE?", 5L));   // reset
       gpibWrite(ud, "SNGLS;");                     // single sweep
       gpibWrite(ud, "AT AUTO;");                   // auto attenuation
       gpibWrite(ud, qq("RL %fDB;", ref_level));    // reference level
       gpibWrite(ud, "LG 5DB;");                    // scale
       gpibWrite(ud, "FA 10MHZ;");                  // start freq
       gpibWrite(ud, "FB 1GHZ;");                   // stop freq
       gpibWrite(ud, "RB 100KHZ;");                 // resolution bandwidth
       gpibWrite(ud, "VB 100KHZ;");                 // video bandwidth
       gpibWrite(ud, "DET POS;");                   // detector mode (peak)
       gpibWrite(ud, "SS MAN;");                    // freq step size manual
       gpibWrite(ud, "SS 1GHZ;");                   // freq step size 1 GHz
    }

    // Make measurement
    // Search between 10 MHz and 1 GHz
    gpibWrite(ud, "FA 10MHZ;");
    gpibWrite(ud, "FB 1GHZ;");
    gpibWrite(ud, "RB 100KHZ;");                 // resolution bandwidth
    gpibWrite(ud, "VB 100KHZ;");                 // video bandwidth
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr_max0 = atof(gpibQuery(ud, "MKA?",55L));
    freq_max0 = atof(gpibQuery(ud, "MKF?",55L));

    // Search between 1 and 2 GHz
    gpibWrite(ud, "FA 1GHZ;");
    gpibWrite(ud, "FB 2GHZ;");
    gpibWrite(ud, "RB 1MHZ;");                 // resolution bandwidth
    gpibWrite(ud, "VB 1MHZ;");                 // video bandwidth
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr_max1 = atof(gpibQuery(ud, "MKA?",55L));
    freq_max1 = atof(gpibQuery(ud, "MKF?",55L));

    // Search between 2 and 2.8 GHz
    gpibWrite(ud, "FA 2GHZ;");
    gpibWrite(ud, "FB 2.8GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 2.8 and 3 GHz
    gpibWrite(ud, "FA 2.8GHZ;");
    gpibWrite(ud, "FB 3GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 3 and 4 GHz
    gpibWrite(ud, "FA 3GHZ;");
    gpibWrite(ud, "FB 4GHZ;");
    gpibWrite(ud, "TS;");
    gpibQuery(ud, "DONE?",5L);
    gpibWrite(ud, "MKPK HI;");
    pwr = atof(gpibQuery(ud, "MKA?",55L));
    freq = atof(gpibQuery(ud, "MKF?",55L));
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 4 and 6 GHz
    for (cf_index=0;cf_index<2;cf_index++) {
       gpibWrite(ud, "CF UP;");
       gpibWrite(ud, "TS;");
       gpibQuery(ud, "DONE?",5L);
       gpibWrite(ud, "MKPK HI;");
       pwr = atof(gpibQuery(ud, "MKA?",55L));
       freq = atof(gpibQuery(ud, "MKF?",55L));
       if (pwr > pwr_max1) {
          pwr_max1 = pwr;
          freq_max1 = freq;
       }
    }

    // Search between 6 and 13 GHz
    for (cf_index=0;cf_index<7;cf_index++) {
       gpibWrite(ud, "CF UP;");
       gpibWrite(ud, "TS;");
       gpibQuery(ud, "DONE?",5L);
       gpibWrite(ud, "MKPK HI;");
       pwr = atof(gpibQuery(ud, "MKA?",55L));
       freq = atof(gpibQuery(ud, "MKF?",55L));
       if (pwr > pwr_max1) {
          pwr_max1 = pwr;
          freq_max1 = freq;
       }
    }

    // Search between 13 and 16 GHz
    for (cf_index=0;cf_index<3;cf_index++) {
       gpibWrite(ud, "CF UP;");
       gpibWrite(ud, "TS;");
       gpibQuery(ud, "DONE?",5L);
       gpibWrite(ud, "MKPK HI;");
       pwr = atof(gpibQuery(ud, "MKA?",55L));
       freq = atof(gpibQuery(ud, "MKF?",55L));
       if (pwr > pwr_max1) {
          pwr_max1 = pwr;
          freq_max1 = freq;
       }
    }

    n = sprintf(emissions, "%07.2f, %10.3E, %07.2f, %10.3E", pwr_max0, freq_max0, pwr_max1, freq_max1);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API char *spaMeasRxSpuriousEmLite(const int ud, 
                                         const double ref_level,
                                         const int reset) {

    int rsp = 0, n, cf_index;
    double pwr, freq, pwr_max0, pwr_max1, freq_max0, freq_max1;
    static char emissions[41] = "-120.00, 0.000E+000, -120.00, 0.000E+000";

    if (reset) {
       rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));                // reset
       gpibWrite(ud, ":INIT:CONT 0;");                             // single sweep
       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
       gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 5;");                 // scale
       gpibWrite(ud, ":BAND:RES 1E5;VID 1E5;");                    // resolution / video bandwidth
       gpibWrite(ud, ":FREQ:STAR 1E7;");                           // start freq
       gpibWrite(ud, ":FREQ:STOP 1E9;");                           // stop freq
       gpibWrite(ud, ":DET POS;");                                 // detector mode (peak)
       gpibWrite(ud, ":CALC:MARK:PEAK:SEAR:MODE MAX");             // set peak search mode
       gpibWrite(ud, ":FREQ:CENT:STEP:AUTO 0;");                   // freq step auto off
       gpibWrite(ud, ":FREQ:CENT:STEP 1E9;");                      // freq step 1 GHz
    }

    // Make measurement
    // Search between 10 MHz and 1 GHz
    gpibWrite(ud, ":FREQ:STAR 1E7;");                         // start freq
    gpibWrite(ud, ":FREQ:STOP 1E9;");                         // stop freq
    gpibWrite(ud, ":BAND:RES 1E5;VID 1E5;");                  // resolution / video bandwidth
    gpibWrite(ud, ":INIT:IMM;*WAI;");                         // trigger sweep
    pwr_max0 = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq_max0 = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak

    // Search between 1 and 2 GHz
    gpibWrite(ud, ":FREQ:STAR 1E9;");                         // start freq
    gpibWrite(ud, ":FREQ:STOP 2E9;");                         // stop freq
    gpibWrite(ud, ":BAND:RES 1E6;VID 1E6;");                  // resolution / video bandwidth
    gpibWrite(ud, ":INIT:IMM;*WAI;");                         // trigger sweep
    pwr_max1 = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq_max1 = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak

    // Search between 2 and 2.8 GHz
    gpibWrite(ud, ":FREQ:STAR 2E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 2.8E9;");                  // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 2.8 and 3 GHz
    gpibWrite(ud, ":FREQ:STAR 2.8E9;");                  // start freq
    gpibWrite(ud, ":FREQ:STOP 3E9;");                    // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 3 and 4 GHz
    gpibWrite(ud, ":FREQ:STAR 3E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 4E9;");                    // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    // Search between 4 and 6 GHz
    for (cf_index=0;cf_index<2;cf_index++) {
       gpibWrite(ud, ":FREQ:CENT UP;");                     // increment center freq 1 GHz
       gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
       pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
       freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
       if (pwr > pwr_max1) {
          pwr_max1 = pwr;
          freq_max1 = freq;
       }
    }

    // Search between 6 and 6.7 GHz
    gpibWrite(ud, ":FREQ:STAR 6E9;");                    // start freq
    gpibWrite(ud, ":FREQ:STOP 6.7E9;");                  // stop freq
    gpibWrite(ud, ":INIT:IMM;*WAI;");                    // trigger sweep
    pwr = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 55L)); // find max peak and measure amplitude
    freq = atof(gpibQuery(ud, ":CALC:MARK:X?",55L));     // measure frequency of peak
    if (pwr > pwr_max1) {
       pwr_max1 = pwr;
       freq_max1 = freq;
    }

    n = sprintf(emissions, "%07.2f, %10.3E, %07.2f, %10.3E", pwr_max0, freq_max0, pwr_max1, freq_max1);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API double spaMeasOBW(const int ud, 
                             const double center, 
                             const double span,
                             const double ref_level,
                             const int reset) {

    int rsp = 0, i = 0, n = 0, iSwpPnts = spa_sweep_points();
    double *trace;
    double sum = 0, tot = 0, obwl = 0, obwu = 0;

    switch (spa_model) {

        case SPA_E4404B :
		case SPA_FSL:

            // reset prior to setting the center frequency
            if (reset) {

                rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
                gpibWrite(ud,    ":AVER 0;");                               // averaging off
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                  // span
                gpibWrite(ud, qq(":FREQ:CENT %f;", center));                // center frequency
                gpibWrite(ud,    ":BAND:RES 300e3;");                       // resolution bandwidth
                gpibWrite(ud,    ":BAND:VID 3e3;");                         // video bandwidth
                gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
                gpibWrite(ud,    ":DISP:WIND:TRAC:Y:PDIV 5;");              // scale
                gpibWrite(ud,    ":INIT:CONT 1;");                          // continuous sweep
                gpibWrite(ud,    ":DET POS;");                              // detector mode (positive peak)

            }
            
            gpibWrite(ud,    ":TRAC:MODE WRIT;");                           // clear write
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));                    // center frequency
            gpibWrite(ud,    ":TRAC:MODE MAXH;");                           // max hold
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                            // trigger sweep
            break;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) {

                rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
                gpibWrite(ud,    "AAVG OFF;");                               // averaging off
                gpibWrite(ud, qq("SP %f;", span));                  // span
                gpibWrite(ud, qq("CF %f;", center));                // center frequency
                gpibWrite(ud,    "RB 300e3;");                       // resolution bandwidth
                gpibWrite(ud,    "VB 3e3;");                         // video bandwidth
                gpibWrite(ud, qq("RL %f;", ref_level)); // reference level
                gpibWrite(ud,    "DD 1;");              // scale 0: 10, 1: 5, 2: 2, 3: 1
                gpibWrite(ud,    "CONTS;");                          // continuous sweep
                gpibWrite(ud,    "DET POS;");                              // detector mode (positive peak)

            }
            
            gpibWrite(ud,    "AW;");                           // clear write
            gpibWrite(ud, qq("CF %f;", center));                    // center frequency
            gpibWrite(ud,    "AMAX ON;");                           // max hold
            gpibWrite(ud,    "TS;");                            // trigger sweep
            break;

        case SPA_8595E :
            
            // reset prior to setting the center frequency
            if (reset) {

                rsp = atoi(gpibQuery(ud, "IP;DONE?", 5L));
                gpibWrite(ud,    "VAVG OFF;");                              // averaging off
                gpibWrite(ud, qq("SP %f;", span));                          // span
                gpibWrite(ud, qq("CF %f;", center));                        // center frequency
                gpibWrite(ud,    "RB 300e3;");                              // resolution bandwidth
                gpibWrite(ud,    "VB 3e3;");                                // video bandwidth
                gpibWrite(ud, qq("RL %f;", ref_level));                     // reference level
                gpibWrite(ud,    "SC 5;");                                  // scale
                gpibWrite(ud,    "CONTS;");                                 // continuous sweep
                gpibWrite(ud,    "DET POS;");                               // detector mode (positive peak)

            }
            
            gpibWrite(ud,    "CLRW TRA;");                                  // clear write
            gpibWrite(ud, qq("CF %f;", center));                            // center frequency
            gpibWrite(ud,    "MXMH TRA;");                                  // max hold
            gpibQuery(ud,    "TS;DONE?;");                                  // trigger sweep
            break;
    
        default :

            return -120;

    }

    trace = spa_get_trace(spa);
    for (i = 0; i < iSwpPnts ; i++) {
        sum += pow(10,(trace[i]/10));
        n++;
    }
    tot = 0.005 * sum;

    obwl = spa_calc_obwl(trace,tot);
    obwu = spa_calc_obwu(trace,tot);
    return (1 - (obwu + obwl)/n) * span;

}

double spa_calc_obwl(double *trace, const double tot) {
    double sum_last = 0;
    double sum = 0;
    int m = 0;

    for (m = 0; m < 200; m++) {
        sum += pow(10,(trace[m]/10));
        if (sum >= tot) {
            sum_last = sum - pow(10,(trace[m]/10));
            return m + interp(tot,sum_last,sum);
        }
    }

    return 0;

}

double spa_calc_obwu(double *trace, const double tot) {
    double sum_last = 0;
    double sum = 0;
    int k = 0, m = spa_sweep_points()-1, n = m/2;

    for (k = 0; k < n; k++) {
        sum += pow(10,(trace[m-k]/10));
        if (sum >= tot) {
            sum_last = sum - pow(10,(trace[m-k]/10));
            return k + interp(tot,sum_last,sum);
        }
    }

    return 0;

}

double interp(const double y, const double y1, const double y2) {

    return (y - y1) / (y2 - y1);

}

MANLIB_API char *spaMeasACP(const int ud,
                            const double center,
                            const double ref_level,
                            const int reset) {

    int n, rsp;
    double main, upper20, lower20, upper40, lower40;
    static char emissions[44] = "-120.00, -120.00, -120.00, -120.00, -120.00";

    // reset prior to setting the center frequency
    if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));

    gpibWrite(ud, qq(":FREQ:CENT %f;", center));                    // center frequency

    if (reset) {
        gpibWrite(ud, ":CONF:ACP;");                                // activate acp menu
        gpibWrite(ud, ":ACP:AVER 0;");                              // averaging off
        gpibWrite(ud, ":ACP:BAND:ACH 18E6;");                       // adjacent bandwidth
        gpibWrite(ud, ":ACP:BAND:INT 18E6;");                       // main bandwidth
        gpibWrite(ud, ":ACP:CSP 20E6;");                            // spacing
        gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", ref_level)); // reference level
        gpibWrite(ud, ":DISP:WIND:TRAC:Y:PDIV 10;");                // scale
        gpibWrite(ud, ":FREQ:SPAN 60E6;");                          // span
        gpibWrite(ud, ":BAND:RES 3E5;VID 3E5;");                    // resolution bandwidth
        gpibWrite(ud, ":INIT:CONT ON;");                            // continuous sweep
        gpibWrite(ud, ":DET SAMP;");                                // detector mode (sample)
    }

    // Make measurement for adjacent channel spacing of 20 MHz
    gpibWrite(ud, ":ACP:CSP 20E6;");                      // spacing
    gpibWrite(ud, ":BAND:RES 3E5;VID 3E5;");              // resolution bandwidth
    main = atof(gpibQuery(ud, ":READ:ACP:MAIN?", 55L));   // measure the main channel power
    upper20 = atof(gpibQuery(ud, ":READ:ACP:UPP?", 55L)); // measure the 20 MHz upper adjacent channel power
    lower20 = atof(gpibQuery(ud, ":READ:ACP:LOW?", 55L)); // measure the 20 MHz lower adjacent channel power

    // Make measurement for adjacent channel spacing of 40 MHz
    gpibWrite(ud, ":ACP:CSP 40E6;");                      // spacing
    gpibWrite(ud, ":BAND:RES 3E5;VID 3E5;");              // resolution bandwidth
    upper40 = atof(gpibQuery(ud, ":READ:ACP:UPP?", 55L)); // measure the 40 MHz upper adjacent channel power
    lower40 = atof(gpibQuery(ud, ":READ:ACP:LOW?", 55L)); // measure the 40 MHz lower adjacent channel power

    n = sprintf(emissions, "%07.2f, %07.2f, %07.2f, %07.2f, %07.2f", main, upper20, lower20, upper40, lower40);
    emissions[n] = '\0';

    return emissions;
}

MANLIB_API int spaMeasSpectralMask11b(const int ud,
                                      const double center,
                                      const double span,
                                      const int reset,
                                      const int verbose,
                                      const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = NEW_SWP_PTS /*spa_sweep_points()*/, i = 0; /* BAA */

//	static int plot_num = 63;

    // labels for gnuplot output
 
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";
//	char	   gp_output_file[50];
    char        gp_label[40];

//	sprintf(gp_output_file, "c:\\temp\\spectral_mask_11b_pcdac%d.gif", plot_num);
//	plot_num = plot_num - 1;
//	if(plot_num < 1) plot_num = 1;

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));           // center frequency
        
            if (reset) {
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 100e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
				gpibWrite(ud,    ":POW:ATT 0");
//       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", SPA_REF_LEVEL)); // reference level
                gpibWrite(ud, qq(":SWEep:POIN %d;", iSwpPnts));         // # of sweep points
        
            }
            gpibWrite(ud, qq(":SWEep:POIN %d;", iSwpPnts));         // # of sweep points
#if (0)            
            gpibWrite(ud,    ":AVER:COUN 20;STATE 1");              // average 10 times
            gpibWrite(ud,    ":DET SAMP;");                         // detector mode (sample)
            //gpibWrite(ud,    ":DET POS;");                         // detector mode (peak)
#else  /* test time reduction */          
            gpibWrite(ud, ":SENS:DET:FUNC:RMS");   // detector mode (RMS)
            gpibWrite(ud, ":AVER:COUN 5;STATE 1"); // average 10 times
#endif            
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                   // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11b mask and plot if desired
            trace          = spa_get_trace_11b(spa, iSwpPnts);
            mask           = spa_802_11b_mask(f0, df, ref_level);
            fail           = spa_compare_mask_11b(trace, mask, f0, df, verbose, output, iSwpPnts);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    ":AVER 0");                           // averaging off
            gpibWrite(ud,    ":SWEep:POIN 401");                   // # of sweep points

            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 100e3;VB 100e3;");     // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "SWPCNT 10;AAVG ON");              // average 10 times
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 10; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11b mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_802_11b_mask(f0, df, ref_level);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF;");                           // averaging off

            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                 // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 100KHZ;VB 100KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep
        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 10;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11b mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_802_11b_mask(f0, df, ref_level);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                         // averaging off

            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    }
 
}

MANLIB_API int spaMeasSpectralMask(const int ud,
                                   const double center,
                                   const double span,
                                   const int reset,
                                   const int verbose,
                                   const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = spa_sweep_points(), i = 0;

    // labels for gnuplot output
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";
    char        gp_label[40];
    int         verbose_local;
    bool        HT40_mask = FALSE;

    if (verbose & 0x80000000) {
        HT40_mask = TRUE; // overloading "verbose" bit 31 to convey HT40 mask FLAG
        verbose_local = verbose & 0x7FFFFFF;
    } else {
        verbose_local = verbose;
    }

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));           // center frequency
            
            if (reset) {       
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 30e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
				gpibWrite(ud,    ":POW:ATT 0");
//       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", SPA_REF_LEVEL)); // reference level
            }

            //gpibWrite(ud,    ":DET SAMP;");       // detector mode (sample)
            gpibWrite(ud, ":SENS:DET:FUNC:RMS");    // detector mode (RMS)
            gpibWrite(ud, ":AVER:COUN 5;STATE 1"); // average 5 times (down from 15) /* test time reduction */
            //gpibWrite(ud,    ":DET POS;");        // detector mode (peak)
            gpibWrite(ud, ":INIT:IMM;*WAI;");       // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, maskLegacy);
            } else {
                mask = spa_802_11a_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose_local, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    ":AVER 0");                           // averaging off


            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency

            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 100e3;VB 30e3;");     // resolution bandwidth
                gpibWrite(ud,    "CONTS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            gpibWrite(ud,    "SWPCNT 7;AAVG ON");              // average 7 times
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 7; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, maskLegacy);
            } else {
                mask = spa_802_11a_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose_local, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF");                           // averaging off

            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                 // center frequency
            if (reset) {       
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 100KHZ;VB 30KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 7;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, maskLegacy);
            } else {
                mask = spa_802_11a_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose_local, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                         // averaging off
            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    }
 
}

MANLIB_API int spaMeasSpectralMaskHt20(const int ud,
                                   const double center,
                                   const double span,
                                   const int reset,
                                   const int verbose,
                                   const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = spa_sweep_points(), i = 0;

    // labels for gnuplot output
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";
    char        gp_label[40];

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));           // center frequency

            if (reset) {
        
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 30e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
				gpibWrite(ud,    ":POW:ATT 0");
//       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", SPA_REF_LEVEL)); // reference level
        
            }
                
            gpibWrite(ud,    ":DET SAMP;");                         // detector mode (sample)
            gpibWrite(ud,    ":AVER:COUN 15;STATE 1");              // average 7 times
            //gpibWrite(ud,    ":DET POS;");                         // detector mode (peak)
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                   // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt20);
            } else {
                mask = spa_802_11n_ht20_mask(f0, df, ref_level);
            }

            fail = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    ":AVER 0");                           // averaging off

            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency

            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 100e3;VB 30e3;");     // resolution bandwidth
                gpibWrite(ud,    "CONTS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            gpibWrite(ud,    "SWPCNT 7;AAVG ON");              // average 7 times
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 7; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt20);
            } else {
                mask = spa_802_11n_ht20_mask(f0, df, ref_level);
            }
            fail = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF");                           // averaging off
            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                 // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 100KHZ;VB 30KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep
        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 7;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt20);
            } else {
                mask = spa_802_11n_ht20_mask(f0, df, ref_level);
            }
            fail = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                         // averaging off
            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    }
 
}


MANLIB_API int spaMeasSpectralMaskHt40(const int ud,
                                   const double center,
                                   const double span,
                                   const int reset,
                                   const int verbose,
                                   const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = spa_sweep_points(), i = 0;

    // labels for gnuplot output
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";
    char        gp_label[40];

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));       // center frequency
            gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span

            if (reset) {
        
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 30e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
				gpibWrite(ud,    ":POW:ATT 0");
//       gpibWrite(ud, qq(":DISP:WIND:TRAC:Y:RLEV %f;", SPA_REF_LEVEL)); // reference level
        
            }
                
            gpibWrite(ud,    ":DET SAMP;");                         // detector mode (sample)
            gpibWrite(ud,    ":AVER:COUN 15;STATE 1");              // average 7 times
            //gpibWrite(ud,    ":DET POS;");                         // detector mode (peak)
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                   // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt40);
            } else {
                mask = spa_802_11n_ht40_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    ":AVER 0");                           // averaging off
            gpibWrite(ud, qq(":FREQ:SPAN %f;", span/2));         // span

            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency
            gpibWrite(ud, qq("SP %f;", span));         // span
            
            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 100e3;VB 30e3;");     // resolution bandwidth
                gpibWrite(ud,    "CONTS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            gpibWrite(ud,    "SWPCNT 7;AAVG ON");              // average 7 times
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 7; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt40);
            } else {
                mask = spa_802_11n_ht40_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF");                           // averaging off
            gpibWrite(ud, qq("SP %f;", span/2));         // span

            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));             // center frequency
            gpibWrite(ud, qq("SP %fHZ;", span));               // span

            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 100KHZ;VB 30KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep
        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 7;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            if(customMask == 1) {
                mask = spa_custom_mask(f0, df, ref_level, mask11nHt40);
            } else {
                mask = spa_802_11n_ht40_mask(f0, df, ref_level);
            }
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                       // averaging off
            gpibWrite(ud, qq("SP %fHZ;", span/2));               // span

            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    } 
}


MANLIB_API int spaMeasDSRCSpectralMask(const int ud,
                                   const double center,
                                   const double span,
                                   const int reset,
								   const int bandwidth,
                                   const int verbose,
                                   const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = spa_sweep_points(), i = 0;

    // labels for gnuplot output
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";
    char        gp_label[40];

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));           // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 30e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
        
            }
                
            gpibWrite(ud,    ":DET SAMP;");                         // detector mode (sample)
            gpibWrite(ud,    ":AVER:COUN 7;STATE 1");              // average 7 times
            //gpibWrite(ud,    ":DET POS;");                         // detector mode (peak)
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                   // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a dsrc mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_802_11a_dsrc_mask(f0, df, ref_level, bandwidth);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    ":AVER 0");                           // averaging off

            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 100e3;VB 30e3;");     // resolution bandwidth
                gpibWrite(ud,    "CONTS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            gpibWrite(ud,    "SWPCNT 7;AAVG ON");              // average 7 times
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 7; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_802_11a_dsrc_mask(f0, df, ref_level, bandwidth);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF");                           // averaging off

            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                 // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 100KHZ;VB 30KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep
        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 7;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_802_11a_dsrc_mask(f0, df, ref_level, bandwidth);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                         // averaging off

            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    }
 
}

MANLIB_API int spaMeasPHSSpectralMask(const int ud,
                                   const double center,
                                   const double span,
                                   const int reset,
                                   const int verbose,
                                   const int output) {
    double *trace;
    double *mask;
    double f0, df, ref_level;
    int rsp, fail = 0, iSwpPnts = spa_sweep_points(), i = 0;

    // labels for gnuplot output
    char        gp_label[40];
    char       *gp_script       = "c:\\temp\\spectral_mask.gp";
    char       *gp_data_file    = "c:\\temp\\spectral_mask.txt";
    char       *gp_output_file  = "c:\\temp\\spectral_mask.gif";

    

    sprintf(gp_label, "Spectral Mask (Channel : %4.2f)", center/1e9);

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));           // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));         // span
                gpibWrite(ud,    ":BAND:RES 10e3;VID 30e3;");     // resolution bandwidth
                gpibWrite(ud,    ":INIT:CONT 0;");                 // disable continuous sweep
                gpibWrite(ud,    ":CALC:MARK:PEAK:SEAR:MODE MAX"); // search for max
        
            }
                
            gpibWrite(ud,    ":DET SAMP;");                         // detector mode (sample)
            gpibWrite(ud,    ":AVER:COUN 7;STATE 1");              // average 7 times
            //gpibWrite(ud,    ":DET POS;");                         // detector mode (peak)
            gpibWrite(ud,    ":INIT:IMM;*WAI;");                   // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_phs_mask(f0, df, ref_level);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

			gpibWrite(ud,    ":AVER 0");                           // averaging off

            return fail;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));           // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));         // span
                gpibWrite(ud,    "RB 10e3;VB 30e3;");     // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                 // disable continuous sweep
                gpibWrite(ud,    "PS;"); // search for max
        
            }
                
            gpibWrite(ud,    "DET SMP;");                         // detector mode (sample)
            gpibWrite(ud,    "SWPCNT 7;AAVG ON");              // average 7 times
            //gpibWrite(ud,    "DET POS;");                         // detector mode (peak)
            for (i = 0; i < 7; i++) {
                gpibWrite(ud,    "TS;");                   // trigger sweep
            }

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "PS;ML?", 20L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);

            // get trace, compare to 802.11a mask and plot if desired
            trace          = spa_get_trace(spa);
            mask           = spa_phs_mask(f0, df, ref_level);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "AAVG OFF;");                           // averaging off

            return fail;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                 // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));               // span
                gpibWrite(ud,    "RBW 10KHZ;VB 30KHZ;");          // resolution bandwidth
                gpibWrite(ud,    "SNGLS;");                        // disable continuous sweep
        
            }
                
            gpibWrite(ud,    "DET POS;");                          // detector mode (peak)
            gpibWrite(ud,    "VAVG 7;");                           // average 7 times
            gpibQuery(ud,    "TS;DONE?;");                         // trigger sweep

            // get trace parameters
            ref_level      = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);

            trace          = spa_get_trace(spa);
            mask           = spa_phs_mask(f0, df, ref_level);
            fail           = spa_compare_mask(trace, mask, f0, df, verbose, output);
            if (output == 1) spa_plot_mask(gp_script, gp_data_file, gp_output_file, gp_label);

            gpibWrite(ud,    "VAVG OFF;");                         // averaging off

            return fail;

        default :

            // break out and return all failures
            return iSwpPnts;

    }
 
}

/*******************************************************************************
description:	measures the peak and restricted-band power... should behave
		similar to spectral mask function (moves to center freq, reduces
		span, etc.)
parameters:	ud		analyzer's unit descriptor
		center		center frequency (hz)
		span		frequency span (hz)
		edge_freq	frequency at the edge of the band
returns:	char array containing peak and restricted-band power (dbm)
*******************************************************************************/
MANLIB_API char *spaMeasRestrictedBand (const int ud,
                                        const double center,
                                        const double span,
                                        const double edge_freq,
                                        const int reset) {

    static char edge_parms[17] = "-120.00, -120.00";
//     static double edge_parms[2] = {-120, -120};
    double *trace;
    double f0, df, peak_power, edge_power;
    int rsp, n, iSwpPnts = spa_sweep_points();

    switch(spa_model) {

        case SPA_E4404B :
        case SPA_FSL :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq(":FREQ:CENT %f;", center));                  // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq(":FREQ:SPAN %f;", span));                // span
                gpibWrite(ud,    ":BAND:RES 100e3;VID 30e3;");            // resolution bandwidth
                gpibWrite(ud,    ":DET POS;");                            // detector mode (peak)
        
            }
                
            // get trace parameters
            rsp            = atoi(gpibQuery(ud, ":INIT:IMM;*OPC?", 5L));
            f0             = atof(gpibQuery(ud, ":FREQ:STAR?", 20L));
            df             = atof(gpibQuery(ud, ":FREQ:SPAN?", 20L)) / (iSwpPnts-1);
            peak_power     = atof(gpibQuery(ud, ":CALC:MARK:MAX;Y?", 20L));

            break;

        case SPA_R3162 :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "*RST;*OPC?", 5L));
        
            gpibWrite(ud, qq("CF %f;", center));                  // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %f;", span));                // span
                gpibWrite(ud,    "RB 100e3;VB 30e3;");            // resolution bandwidth
                gpibWrite(ud,    "DET POS;");                            // detector mode (peak)
        
            }
                
            // get trace parameters
            rsp            = atoi(gpibQuery(ud, "TS?", 5L));
            f0             = atof(gpibQuery(ud, "FA?", 20L));
            df             = atof(gpibQuery(ud, "SP?", 20L)) / (iSwpPnts-1);
            peak_power     = atof(gpibQuery(ud, "PS;ML?", 20L));

            break;

        case SPA_8595E :

            // reset prior to setting the center frequency
            if (reset) rsp = atoi(gpibQuery(ud, "IP;DONE?;", 5L));
        
            gpibWrite(ud, qq("CF %fHZ;", center));                       // center frequency
        
            if (reset) {
        
                gpibWrite(ud, qq("SP %fHZ;", span));                     // span
                gpibWrite(ud,    "RBW 100KHZ;VB 30KHZ;");                // resolution bandwidth
                gpibWrite(ud,    "DET POS;");                            // detector mode (peak)
        
            }
                
            // get trace parameters
            rsp            = atoi(gpibQuery(ud, "TS;DONE?;", 5L));
            f0             = atof(gpibQuery(ud, "FA?;", 20L));
            df             = atof(gpibQuery(ud, "SP?;", 20L)) / (iSwpPnts-1);
            peak_power     = atof(gpibQuery(ud, "MKPK HI;MKA?;", 20L));

            break;

        default :

            // break out and return all failures
            return edge_parms;

    }
 
    // get trace, lookup restricted-band power
    trace          = spa_get_trace(spa);
    edge_power     = spa_lookup_point(trace, f0, df, edge_freq);

    // explicitly assign array values for readability
//     edge_parms[0] = peak_power;
//     edge_parms[1] = edge_power;
    n = sprintf(edge_parms, "%07.2f, %07.2f", peak_power, edge_power);
    edge_parms[n] = '\0';

    return edge_parms;

}

/*******************************************************************************
description:	generates a gnuplot(gp) script using the specified data file,
		output file and label and runs the script using gp, then
		attempts to execute the file (in windows gifs will be displayed
		automatically using windows explorer or another associated
		viewer)
parameters:	trace 		iSwpPnts point analyzer trace (dbm)
		f0		start frequency (hz)
		df		frequency increment (hz)
		f		frequency (hz) used to look up point
returns:	power (dbm) at frequency closest to lookup frequency
*******************************************************************************/
double spa_lookup_point(double *trace, const double f0, const double df, const double f) {

    int pt = (int)((f - f0) / df), n = spa_sweep_points();
    
    // point less than the start frequency... return first point
    if (pt < 0) { return trace[0]; }

    // point greater than the stop frequency... return last point
    else if (pt > n) { return trace[n]; }

    // point within start and stop frequecies... return corresponding point
    else { return trace[pt]; }

}

/*******************************************************************************
description:	generates a gnuplot(gp) script using the specified data file,
		output file and label and runs the script using gp, then
		attempts to execute the file (in windows gifs will be displayed
		automatically using windows explorer or another associated
		viewer)
parameters:	gp_script	name of script containing gp commands
		gp_data_file	name of data file to plot
		gp_output_file	name of output file (.gif)
		gp_label	label appearing at top of plot
returns:	1
*******************************************************************************/
int spa_plot_mask(char *gp_script, char *gp_data_file, char *gp_output_file, char *gp_label) {

    FILE *fp;

    // open a new file for gnuplot
    fp = fopen(gp_script, "w");
    fprintf(fp,    "set terminal gif\n");
    fprintf(fp, qq("set output '%s'\n", gp_output_file));
    fprintf(fp, qq("set label '%s'\n", gp_label));
    fprintf(fp, qq("plot '%s' using 1:2 w l, '%s' using 1:3 w l\n", gp_data_file, gp_data_file));
    fprintf(fp,    "set output\n");
    fprintf(fp,    "set terminal windows\n");
    fclose(fp);

    // make a system call to gnuplot
    system(qq("wgnupl32 %s", gp_script));
//	system(gp_output_file);

    return 1;

}

/*******************************************************************************
description:	compares the specified trace to an 11a mask
parameters:	trace 		iSwpPnts point analyzer trace (dbm)
		mask 		iSwpPnts point 11a mask (dbm)
		f 		start frequency (hz)
		df 		frequency increment (hz)
		verbose		prints failures to screen if 1
		output		prints trace & mask to temp file in c:\temp if 1
returns:	number of failure points
*******************************************************************************/
int spa_compare_mask(double *trace, double *mask, const double f0, const double df, const int verbose, const int output) {

    int i, fail = 0;
    double f = f0;
    FILE *fp = NULL;
    int logData = output, n = spa_sweep_points();

#ifdef WIN32
    // Create the directory - Windows only
    if (logData == 1) _mkdir( "\\testtmp" );
#endif
    if (logData == 1) {
        // open file and print header
        if( (fp = fopen("C:\\temp\\spectral_mask.txt","w")) == NULL ) {
            if ( verbose == 1 ) printf( "FAILURE: to open spectral_mask.txt - no log will be created\n");
            logData = 0;
        }
        else {
            fprintf(fp,"# Spectral Mask Output\n# Atheros Communications Inc.\n\n");
        }
    }

    // print data
    for (i = 0; i <= n; i++) {

        // print all points to file
        if (logData == 1) fprintf(fp, "%10.5f\t%7.4f\t%7.4f\n", f / 1e9, trace[i], mask[i]);

        // print failures to screen
        if ( trace[i] > mask[i] ) {
            if ( verbose == 1 )  printf( "FAILURE : %7.4fdBm > %7.4fdBm at %7.5fGHz\n", trace[i], mask[i], f / 1e9);
            fail++;
        }

        // increment frequency
        f += df;

    }

    // close file
    if (logData == 1) fclose(fp);

    // return number of failures
    return fail;
}

/*******************************************************************************
description:	compares the specified trace to an 11b mask (specific to 11b mask meas)
parameters:	trace 		iSwpPnts point analyzer trace (dbm)
		mask 		iSwpPnts point 11a mask (dbm)
		f 		start frequency (hz)
		df 		frequency increment (hz)
		verbose		prints failures to screen if 1
		output		prints trace & mask to temp file in c:\temp if 1
returns:	number of failure points
*******************************************************************************/
int spa_compare_mask_11b(double *trace, 
                         double *mask, 
                         const double f0, 
                         const double df, 
                         const int verbose, 
                         const int output,
                         int iNumSwpPts) {

    int i, fail = 0;
    double f = f0;
    FILE *fp = NULL;
    int logData = output, n = iNumSwpPts;

#ifdef WIN32
    // Create the directory - Windows only
    if (logData == 1) _mkdir( "\\testtmp" );
#endif
    if (logData == 1) {
        // open file and print header
        if( (fp = fopen("C:\\temp\\spectral_mask.txt","w")) == NULL ) {
            if ( verbose == 1 ) printf( "FAILURE: to open spectral_mask.txt - no log will be created\n");
            logData = 0;
        }
        else {
            fprintf(fp,"# Spectral Mask Output\n# Atheros Communications Inc.\n\n");
        }
    }

    // print data
    for (i = 0; i <= n; i++) {

        // print all points to file
        if (logData == 1) fprintf(fp, "%10.5f\t%7.4f\t%7.4f\n", f / 1e9, trace[i], mask[i]);

        // print failures to screen
        if ( trace[i] > mask[i] ) {
            if ( verbose == 1 )  printf( "FAILURE : %7.4fdBm > %7.4fdBm at %7.5fGHz\n", trace[i], mask[i], f / 1e9);
            fail++;
        }

        // increment frequency
        f += df;

    }

    // close file
    if (logData == 1) fclose(fp);

    // return number of failures
    return fail;
}

double interpolate(const double y2, const double y1, const double x2, const double x1, const double x) {

    return y1 + (y2-y1) / (x2-x1) * (x-x1);

}

/*******************************************************************************
description:	generates an 11a compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz)
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_802_11a_mask(const double f0, const double df, const double ref_level) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <=  9e6) mask[i] = ref_level;
        else if (offset <= 11e6) mask[i] = ref_level - interpolate(  0, 20,  9e6, 11e6, offset);
        else if (offset <= 20e6) mask[i] = ref_level - interpolate( 20, 28, 11e6, 20e6, offset);
        else if (offset <= 30e6) mask[i] = ref_level - interpolate( 28, 40, 20e6, 30e6, offset);
        else    mask[i] = ref_level - 40;

        // increment frequency
        f += df;

    }
    return mask;

}

/*******************************************************************************
description:	generates an 11a compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz)
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_802_11n_ht20_mask(const double f0, const double df, const double ref_level) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <=  9e6) mask[i] = ref_level;
        else if (offset <= 11e6) mask[i] = ref_level - interpolate(  0, 20,  9e6, 11e6, offset);
        else if (offset <= 20e6) mask[i] = ref_level - interpolate( 20, 28, 11e6, 20e6, offset);
        else if (offset <= 30e6) mask[i] = ref_level - interpolate( 28, 45, 20e6, 30e6, offset);
        else    mask[i] = ref_level - 45;

        // increment frequency
        f += df;

    }
    return mask;

}

/*******************************************************************************
description:	generates an PHS compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz)
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_phs_mask(const double f0, const double df, const double ref_level) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <=  504e3) mask[i] = ref_level;
        else if (offset <= 804e3) mask[i] = ref_level - 31;
        else    mask[i] = ref_level - 36;

        // increment frequency
        f += df;

    }
    return mask;

}

/*******************************************************************************
description:	generates an 11a dsrc (class c) compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz)
		ref_level	level at flat-top of mask (dbm)
		bw      bandwidth in MHz (5, 10 or 20)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_802_11a_dsrc_mask(const double f0, const double df, 
									 const double ref_level, int bw) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;
	double scale = 1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

	switch (bw) {
	case 5:
		scale = 0.5;
		break;

	case 10:
		scale = 1;

		break;

	case 20:
		scale = 2;
		break;

	default:
		printf("Internal software error: illegal DSRC bandwidth\n");
		return mask;
	}

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <= 4.5e6 * scale) mask[i] = ref_level;
        else if (offset <= 5e6   * scale) mask[i] = ref_level - interpolate(  0, 26,  4.5e6*scale, 5e6*scale, offset);
        else if (offset <= 5.5e6 * scale) mask[i] = ref_level - interpolate( 26, 32, 5e6*scale, 5.5e6*scale, offset);
        else if (offset <= 10e6  * scale) mask[i] = ref_level - interpolate( 32, 40, 5.5e6*scale, 10e6*scale, offset);
        else if (offset <= 15e6  * scale) mask[i] = ref_level - interpolate( 40, 50, 10e6*scale, 15e6*scale, offset);
        else    mask[i] = ref_level - 50;

        // increment frequency
        f += df;

    }
    return mask;

}

/*******************************************************************************
description:	queries the analyzer for a trace
parameters:	ud		analyzer's unit descriptor
returns:	double array of trace points (dbm)
*******************************************************************************/
extern double *spa_get_trace(const int ud) {
    int            iNum             = 106496;
    char           cResult[106496]  = {0};
    int            iSwpPnts         = spa_sweep_points(),dd=0;
    long           lCount           = 0L;
    double         rl=0,bot=0;
    static char   *cToken;
    static int     scales[4]={10,5,2,1};
    static double  dResult[SPA_MAX_SWEEP_POINTS];

#ifndef _IQV
    switch(spa_model) {

         case SPA_E4404B :
         case SPA_FSL :
 
            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "FORM ASC\n");
         
            // get trace header and data
            gpibWrite(ud, "TRAC? TRACE1\n");
            ibrd(ud, (char *)cResult, iNum);
         
            cToken = strtok(cResult, ",");
            dResult[lCount] = atof(cToken);
         
            // print out array
            while (cToken != NULL) {
         
                lCount++;
                cToken = strtok(NULL, ",");
                if (lCount != iSwpPnts) dResult[lCount] = atof(cToken);

            }
            return dResult;
 
         case SPA_R3162 :
 
            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "DL0\n");
         
            rl=atoi(gpibQuery(ud,"RL?",20L));
            dd=atoi(gpibQuery(ud,"DD?",20L));
            bot=rl-scales[dd]*10;

            // get trace header and data
            gpibWrite(ud, "TAA?\n");
         
            for (lCount = 0; lCount < iSwpPnts; lCount++) {
                ibrd(ud, (char *)cResult, iNum);
                cToken = strtok(cResult, "\x0d\x0a");
                dResult[lCount] = bot+(atoi(cToken)-1792)/128;
         
            }
            return dResult;

        case SPA_8595E :

            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "TDF P;");
         
            // get trace header and data
            gpibWrite(ud, "TA;");
            ibrd(ud, (char *)cResult, iNum);
         
            cToken = strtok(cResult, "\x0d\x0a");
            dResult[lCount] = atof(cToken);
         
            // print out array
            while (cToken != NULL) {
         
                lCount++;
                cToken = strtok(NULL, "\x0d\x0a");
                if (lCount != iSwpPnts) dResult[lCount] = atof(cToken);
         
            }
            return dResult;

        default :

            return dResult;

    }
#else
	return dResult;
#endif       // _IQV                
}

/*******************************************************************************
description:	queries the analyzer for a trace (specific to 11b mask meas)
parameters:	ud		analyzer's unit descriptor
returns:	double array of trace points (dbm)
*******************************************************************************/
double* spa_get_trace_11b(const int ud, int iNumSwpPts) {
    int            iNum             = 106496;
    char           cResult[106496]  = {0};
    int            iSwpPnts         = iNumSwpPts,dd=0;
    long           lCount           = 0L;
    double         rl=0,bot=0;
    static char   *cToken;
    static int     scales[4]={10,5,2,1};
    static double  dResult[SPA_MAX_SWEEP_POINTS];

#ifndef _IQV
    switch(spa_model) {

         case SPA_E4404B :
         case SPA_FSL :
 
            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "FORM ASC\n");
         
            // get trace header and data
            gpibWrite(ud, "TRAC? TRACE1\n");
            ibrd(ud, (char *)cResult, iNum);
         
            cToken = strtok(cResult, ",");
            dResult[lCount] = atof(cToken);
         
            // print out array
            while (cToken != NULL) {
         
                lCount++;
                cToken = strtok(NULL, ",");
                if (lCount != iSwpPnts) dResult[lCount] = atof(cToken);
            }
            return dResult;
 
         case SPA_R3162 :
 
            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "DL0\n");
         
            rl=atoi(gpibQuery(ud,"RL?",20L));
            dd=atoi(gpibQuery(ud,"DD?",20L));
            bot=rl-scales[dd]*10;

            // get trace header and data
            gpibWrite(ud, "TAA?\n");
         
            for (lCount = 0; lCount < iSwpPnts; lCount++) {
                ibrd(ud, (char *)cResult, iNum);
                cToken = strtok(cResult, "\x0d\x0a");
                dResult[lCount] = bot+(atoi(cToken)-1792)/128;
         
            }
            return dResult;

        case SPA_8595E :

            // set analyzer trace data format to 32-bit real
            gpibWrite(ud, "TDF P;");
         
            // get trace header and data
            gpibWrite(ud, "TA;");
            ibrd(ud, (char *)cResult, iNum);
         
            cToken = strtok(cResult, "\x0d\x0a");
            dResult[lCount] = atof(cToken);
         
            // print out array
            while (cToken != NULL) {
         
                lCount++;
                cToken = strtok(NULL, "\x0d\x0a");
                if (lCount != iSwpPnts) dResult[lCount] = atof(cToken);
         
            }
            return dResult;

        default :

            return dResult;

    }
#else
	return dResult;
#endif       // _IQV                
}

/*******************************************************************************
description:	generates an 11b compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz)
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_802_11b_mask(const double f0, const double df, const double ref_level) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = NEW_SWP_PTS/*spa_sweep_points()*/-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <= 11e6) mask[i] = ref_level;
        else if (offset <= 22e6) mask[i] = ref_level - 30;
        else    mask[i] = ref_level - 50;

        // increment frequency
        f += df;

    }
    return mask;

}

/*******************************************************************************
description:	generates an 11n HT40 compliant mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz) - expected to give 200 MHz span (twice 
                of spa_802_11n_ht40_mask
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
extern double *spa_802_11n_ht40_mask(const double f0, const double df, const double ref_level) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <=  19e6) mask[i] = ref_level;
        else if (offset <= 21e6) mask[i] = ref_level - interpolate(  0, 20,  19e6, 21e6, offset);
        else if (offset <= 40e6) mask[i] = ref_level - interpolate( 20, 28, 21e6, 40e6, offset);
        else if (offset <= 60e6) mask[i] = ref_level - interpolate( 28, 45, 40e6, 60e6, offset);
        else    mask[i] = ref_level - 45;

        // increment frequency
        f += df;

    }
    return mask;

}


/*******************************************************************************
description:	Capture custom mask array (freq in Mhz, level in negative dbm)
parameters:		pointer to spectrum mask of Legacy rates
		        pointer to spectrum mask of 11n HT20 rates
		        pointer to spectrum mask of 11n HT40 rates
returns:	    void
*******************************************************************************/

MANLIB_API void setCustomMask(short maskArrayLegacy[4][2], short maskArray11nHt20[4][2], short maskArray11nHt40[4][2])
{
    int i;
    customMask = 1;
    for(i=0; i<4; i++) {
        maskLegacy[i][0] = maskArrayLegacy[i][0];
        maskLegacy[i][1] = maskArrayLegacy[i][1];

        mask11nHt20[i][0] = maskArray11nHt20[i][0];
        mask11nHt20[i][1] = maskArray11nHt20[i][1];

        mask11nHt40[i][0] = maskArray11nHt40[i][0];
        mask11nHt40[i][1] = maskArray11nHt40[i][1];

    }

    return;
}

/*******************************************************************************
description:	generates an customer specific mask
parameters:	f0		start frequency (hz)
		df 		frequency increment (hz) - expected to give 200 MHz span
		ref_level	level at flat-top of mask (dbm)
returns:	double array of mask points (dbm)
*******************************************************************************/
double *spa_custom_mask(double f0, double df, double ref_level, A_INT16 customMask[4][2]) {

    static double mask[SPA_MAX_SWEEP_POINTS];
    double f, fc, offset;
    int i, n = spa_sweep_points()-1;

    // initialize
    fc = f0 + (df * n)/2;
    f = f0;

    // loop through all points
    for (i = 0; i <= n; i++) {

        // determine abs offset
        offset = fabs(fc- f);

        // determine which limit to use
        if      (offset <=  (double)customMask[0][0]*1e6) mask[i] = ref_level;
 
        else if (offset <= (double)customMask[1][0]*1e6) mask[i] = ref_level 
            - interpolate((double)customMask[0][1], (double)customMask[1][1], (double)customMask[0][0]*1e6, (double)customMask[1][0]*1e6, offset);

        else if (offset <= (double)customMask[2][0]*1e6) mask[i] = ref_level 
            - interpolate( (double)customMask[1][1], (double)customMask[2][1], (double)customMask[1][0]*1e6, (double)customMask[2][0]*1e6, offset);

        else if (offset <= (double)customMask[3][0]*1e6) mask[i] = ref_level 
            - interpolate( (double)customMask[2][1], (double)customMask[3][1], (double)customMask[2][0]*1e6, (double)customMask[3][0]*1e6, offset);

        else    mask[i] = ref_level - (double)customMask[3][1];

        // increment frequency
        f += df;

    }
    return mask;

}