/*
 * TFTP library 
 * copyright (c) 2004 Vanden Berghen Frank  
 *
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
 
#ifndef __TFTP_H
#define	__TFTP_H 1

/*
 * Trivial File Transfer Protocol (IEN-133)
 */
#define	SEGSIZE	512 		/* data segment size */
#define PKTSIZE SEGSIZE+4   /* full packet size */

/*
 * Packet types.
 */
#define	RRQ	      1				/* read request */
#define	WRQ	2				/* write request */
#define	DATA     3			      /* data packet */
#define	ACK 	4				/* acknowledgement */
#define	ERROR   5 			      /* error code */

/*
 * Error codes.
 */
#define	EUNDEF             0		/* not defined */
#define	ENOTFOUND       1		/* file not found */
#define	EACCESS            2		/* access violation */
#define	ENOSPACE	      3		/* disk full or allocation exceeded */
#define	EBADOP		      4		/* illegal TFTP operation */
#define	EBADID		      5		/* unknown transfer ID */
#define	EEXISTS		      6		/* file already exists */
#define	ENOUSER		7		/* no such user */

struct	tftphdr 
{
	short	th_opcode;			      /* packet type */
	union {
		unsigned short	tu_block;   	/* block # */
		short	tu_code;		             /* error code */
		char	tu_stuff[1];		             /* request packet stuff */
	} th_u;
	char	th_data[1];			             /* data or error string */
} __attribute__((packed));

#define	th_block    th_u.tu_block
#define	th_code     th_u.tu_code
#define	th_stuff     th_u.tu_stuff
#define	th_msg      th_data

void nak(int peer,struct sockaddr_in *p_to_addr,int error,char *p_error_msg);

int tftp_receive(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
                char (*TFTPwrite)(char *,long ,char,void *),
                void *argu);
int tftp_receive_ext(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,                
                     char (*TFTPwrite)(char *,long ,char,void *),
                     void *argu,int vPKTSIZE);
int tftp_send(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
              char (*TFTPread)(char *,long *,char,void *),
              void *argu);
int tftp_send_ext(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
                  char (*TFTPread)(char *,long *,char,void *),
                  void *argu,int vPKTSIZE);

/* return 0 if no error. */
char TFTPswrite(char *p_data,long n,char first,void *f);
/* return 0 if no error. */
char TFTPsread(char *p_data,long *n,char first,void *f);

void tftp_free(void * ptr);

#endif /* tftp.h */
