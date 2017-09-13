#ifndef __HNAPNETWORK_H__
#define __HNAPNETWORK_H__

/* for pure object */
#define MAX_RES_LEN         10240
#define MAX_RES_LEN         10240

typedef struct pure_atrs_s
{
        /* init in httpd.c */
        int conn_fd;
        long content_length;
        char *if_modified_since;
        char *content_type;
        char *protocol;
        char *action_string;

        /* pure.c (set_pure_settings) */
        int reboot;
        int action;
        int set_nvram_error;

        /* init in pure.c (add to response)*/
        char response[MAX_RES_LEN];
        int response_size;
        int response_len;
        int status;
        long bytes;

        /* init in pure.c (ConnectedDevice) */
        char wlanTxPkts[10];
        char wlanRxPkts[10];
        char wlanTxBytes[10];
        char wlanRxBytes[10];
        char lanTxPkts[10];
        char lanRxPkts[10];
        char lanTxBytes[10];
        char lanRxBytes[10];

        /* init in pure.c for get soap body (if soap header and soap body in one package)*/
        FILE *stream;

}pure_atrs_t;

pure_atrs_t pure_atrs;


typedef enum PktType
{
        TXPACKETS =1,
        RXPACKETS,
        LOSSPACKETS
}ePktType;

typedef enum ByteType
{
        TXBYTES =1,
        RXBYTES,
}eByteType;

extern int rc_restart();	/* in hnap_system.c */
extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);

#endif
