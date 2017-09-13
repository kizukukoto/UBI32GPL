#ifndef __LLMNR_H__
#define __LLMNR_H__

#define LLMNR_PORT  "5355"    // the port users will be connecting to

#define MAX_BUFLEN  1400

#define LLMNR_MCAST_V4GROUP "224.0.0.252"
#define LLMNR_MCAST_V6GROUP "FF02::1:3"



struct llmnr_hdr_s {

   unsigned short id;           /* 16 bit unique query ID */

   unsigned short flags;        /* various bit fields, see below */

   unsigned short qdcount;      /* entries in the question field */

   unsigned short ancount;      /* resource records in the answer field */

   unsigned short nscount;      /* name server resource records */

   unsigned short arcount;      /* resource records in the additional records */

} __attribute__ ((packed));



#define LLMNR_TTL       30



#if 0

#define LLMNR_QUERY     0

#define LLMNR_RESPONSE  1

#define LLMNR_OPCODE    0

#define LLMNR_CONFLICT  0

#define LLMNR_TRUNCAT   0

#define LLMNR_TENTATIVE 0

#define LLMNR_RESERVE   0

#define LLMNR_RESPCODE  0

#endif



#endif /* __LLMNR_H__ */

