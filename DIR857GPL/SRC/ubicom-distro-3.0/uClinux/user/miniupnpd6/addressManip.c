#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <ctype.h>
#include "addressManip.h"


/* Computing the IPv6 mask according to the mask lentgh into the in6_addr structure given */
int
getv6AddressMask(struct in6_addr * addr, int masklen)
{
    int bytelen=0, bitlen=0;

    uint8_t bitmask;
    /* Uncomment "//"comment to print the mask */
    //char mask[46]="";

    /* Generate the "all 1s" part */
    bytelen = masklen / 8;
    memset(&(addr->s6_addr), 0xff, bytelen);
    if(bytelen == sizeof(struct in6_addr))
    {
        /* Print the formated mask */
        //inet_ntop(AF_INET6, addr, &mask, INET6_ADDRSTRLEN);
        //printf("IPv6 Mask: %s\n", mask);
        return 0;
    }

    /* Generate the "crossing byte" */
    bitlen = masklen % 8;
    bitmask = 0xff << (8 - bitlen); // Shifting the 0s
    addr->s6_addr[bytelen]=bitmask;

    /* Generate the "all 0s" part */
    memset(&(addr->s6_addr[bytelen+1]), 0x00, sizeof(struct in6_addr) - bytelen);

    /* Print the formated mask */
    //inet_ntop(AF_INET6, addr, &mask, INET6_ADDRSTRLEN);
    //printf("IPv6 Mask: %s\n", mask);

    return 0;
}

/* Adding 0's when needed to have an 32 xdigit IPv6 address */
void
paddingAddr(char *str, int *index, int lim)
{
    int i=0, id=*index;
    for(i=1; i<lim; i++)
    {
        str[id]='0';
        id++;
    }
    *index=id;

}

/* Formating the IPv6 address accordingly to the address stored in the /proc/net/if_inet6 file */
void
del_char(char *str)
{
#ifdef DEBUG
    //printf("\tdel_char: input string = %s\n", str);
#endif
    int id_read=0, id_write=0, init=0, str_len=0;
    char tmp[40]="";
    str_len = strlen(str);
    if(!isxdigit(str[id_read]))
        init=1;

    while (str[id_read]!='\0')
    {
        if(isxdigit(str[id_read]))
        {
            tmp[id_write]=str[id_read];
            id_write++;
        }
        else
        {
            if(!isxdigit(str[id_read+1]))
            {
                if(str[id_read+2]!='\0')
                {
                    char *p;
                    int sep=0, mis=0;
                    p=str;
                    while(*p!='\0')
                    {
                        if(*p==':')
                             sep++;
                        p++;
                    }
                    if(!init)
                        mis = 7 -sep +1;
                    else
                        mis = 7 -sep +2;
                    paddingAddr(tmp, &id_write, mis*4+1);
                }
                else
                {
                    if(str_len <= 2)
                    {
                        paddingAddr(tmp, &id_write, 33);
                        break;
                    }
                    else
                    {
                        char *p;
                        int sep=0, mis=0;
                        p=str;
                        while(*p!='\0')
                        {
                            if(*p==':')
                                 sep++;
                            p++;
                        }
                        if(!init)
                            mis = 7 -sep +1;
                        else
                            mis = 7 -sep +2;
                        paddingAddr(tmp, &id_write, mis*4+1);
                    }
                }
            }
            else if(!isxdigit(str[id_read+2]))
                paddingAddr(tmp, &id_write, 4);
            else if(!isxdigit(str[id_read+3]))
                paddingAddr(tmp, &id_write, 3);
            else if(!isxdigit(str[id_read+4]))
                paddingAddr(tmp, &id_write, 2);
        }
        id_read++;
    }
    tmp[id_write]='\0';
    strcpy(str, tmp);
#ifdef DEBUG
    //printf("\tdel_char: output string = %s\n", str);
#endif
}

/* Re-Formating the IPv6 address accordingly to the useful presentation method */
void
add_char(char *str)
{
#ifdef DEBUG
    //printf("\tadd_char: input string = %s\n", str);
#endif
    int id_read=0, id_write=0, num=0, sep=0, count=0;
    char tmp[40]="";

    while (str[id_read]!='\0')
    {
        num=id_read%4;
        if(id_read && !num)
        {
            tmp[id_write]=':';
            id_write++;
        }
        tmp[id_write]=str[id_read];
        id_write++;
        id_read++;
    }
    tmp[id_write]='\0';
    strcpy(str, tmp);
    id_read=0, id_write=0;
    memset(tmp, 0, sizeof(tmp));
    if(str[id_read]=='0')
    {
        sep=1;
        tmp[id_write]=':';
        id_write++;
    }
    while (str[id_read]!='\0')
    {
        if(!sep)
        {
            if(isxdigit(str[id_read]))
                sep=0;
            else
                sep=1;
            count=0;
            tmp[id_write]=str[id_read];
            id_write++;
        }
        else
        {
            if(str[id_read]=='0')
            {
                count++;
                sep=1;
            }
            else
            {
                if(!isxdigit(str[id_read]))
                {
                    sep=1;
                    if(count==28)
                    {
                        tmp[id_write]=':';
                        id_write++;
                        count=0;
                    }
                }
                else
                {
                    sep=0;
                    if(count>=4)
                    {
                        tmp[id_write]=':';
                        id_write++;
                        count=0;
                    }
                    tmp[id_write]=str[id_read];
                    id_write++;
                }
            }
        }
        id_read++;
    }
    tmp[id_write]='\0';
    strcpy(str, tmp);
#ifdef DEBUG
    //printf("\tadd_char: output string = %s\n", str);
#endif
}

/* Retrieve the index of an interface from its address */
int
getifindex(const char * addr, int * ifindex)
{
    FILE *f;//int s;
	char devname[IFNAMSIZ];
	char addr6p[40]="", buf[40]="";
	int has_index=0, plen, scope, dad_status, if_idx;
	strcpy(buf, addr);
	del_char(buf);

	f=fopen("/proc/net/if_inet6", "r");

	if(f == NULL)
	{
		printf("\tfopen(\"/proc/net/if_inet6\", \"r\")");
		return -1;
	}
	else
	{
	    while(fscanf(f, "%32s %02x %02x %02x %02x %20s\n", addr6p, &if_idx, &plen, &scope, &dad_status, devname) !=EOF)
	    {
	            //printf("\naddr6p: %s\n", addr6p);
		    //printf("  addr: %s\n", buf);
	            if(strcmp(addr6p,buf)==0)
	            {
	                has_index=1;
	                *ifindex=if_idx;
#ifdef DEBUG
	                printf("\tAddress index: %d\n", *ifindex);
#endif
	                break;
	            }
	    }
	    if(has_index==0)
        {
            printf("\tNo match found for this address! No index retreived\n");
            return -1;
        }
        fclose(f);
	}
	return 0;
}
