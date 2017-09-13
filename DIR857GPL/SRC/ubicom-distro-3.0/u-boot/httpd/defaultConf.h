#ifndef __DEFAULT_CONF_H__
#define __DEFAULT_CONF_H__
#define IP_ADDR(w,x,y,z)    ((unsigned long)(((unsigned long)w << 24) | \
                                             ((unsigned long)x << 16) | \
                                             ((unsigned long)y << 8) | \
                                             (unsigned long)z))
                                             
#define     DEFAULT_IP_ADDRESS          IP_ADDR(192,168,0,1)
#define     DEFAULT_ADMIN		"admin"
#define     DEFAULT_USER		"user"
#define     DEFAULT_PASSWORD            ""
#define     DEFAULT_IP_ADDRESS          IP_ADDR(192,168,0,1)
#define     DEFAULT_IP_MASK             IP_ADDR(255,255,255,0)
#define     DEFAULT_DHCP_START_IP       IP_ADDR(192,168,0,100)

#endif //__DEFAULT_CONF_H__
