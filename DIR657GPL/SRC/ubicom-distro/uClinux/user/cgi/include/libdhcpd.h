// dhcpd_list_table
#define M_MAX_DHCP_LIST 254

struct html_leases_s
{
     unsigned char hostname[64];
     unsigned char mac_addr[20];
     unsigned char ip_addr[16];
     unsigned char expires[32];
};

struct dhcpd_leased_table_s{
        char *hostname;
        char *ip_addr;
        char *mac_addr;
        char *expired_at;
};

extern int get_dhcpd_leased_table(struct dhcpd_leased_table_s *client_list);
// end dhcpd_list_table
