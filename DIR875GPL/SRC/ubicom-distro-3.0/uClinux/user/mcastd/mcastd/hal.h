#ifndef __HAL_H__
#define __HAL_H__
int atu_search(unsigned char *mac, int vlan);
int atu_add(unsigned char *mac, int vlan, char portmap);
int atu_purge(unsigned char *mac, int vlan);
void atu_flushall();
//int acl_add_mac_entry(int n);
int acl_add_mac_entry(int argc, char *argv[]);
void reg_init();
void acl_dump(int debug);
void acl_disable_entry(int n);
void acl_enable_entry(int n, const char *s);
int atu_get_next();
int __ether_atoe(const char *a, unsigned char *e);
#endif
