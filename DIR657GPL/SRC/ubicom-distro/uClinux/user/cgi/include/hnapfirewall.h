#ifndef __HNAPFIREWALL_H__
#define __HNAPFIREWALL_H__

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

/*for port mapping */
typedef struct vs_rule_s{
        char *rule;
} vs_rule_t;

/* for mac filter */
typedef struct mac_filter_list_s{
        char *mac_address;
} mac_filter_list_t;

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);
extern void do_get_element_value(const char *, char *, char *);

#endif
