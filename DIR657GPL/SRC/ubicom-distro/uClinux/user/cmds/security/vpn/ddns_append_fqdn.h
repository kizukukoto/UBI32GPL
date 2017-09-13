#ifndef __DDNS_FQDN
#define __DDNS_FQDN
#define NS_PATH "/tmp/ns.res"
#endif

extern void ddns_append_fqdn(const char *fqdn,char *path);
