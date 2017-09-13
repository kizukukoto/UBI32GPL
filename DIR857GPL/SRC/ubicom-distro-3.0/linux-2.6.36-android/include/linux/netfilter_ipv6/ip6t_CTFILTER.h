#ifndef _IP6T_CTFILTER_H
#define _IP6T_CTFILTER_H

enum ip6t_ctfilter_proto_type {
	IP6T_UDP,
	IP6T_TCP,
	IP6T_ICMPV6
};

enum ip6t_ctfilter_type {
	IP6T_ENDPOINT_INDEPENDENT,
	IP6T_ADDRESS_DEPENDENT,
	IP6T_ADDRESS_PORT_DEPENDENT
};

struct ip6t_ctfilter_info {
	u_int32_t	proto_type;	/* ctfilter proto type */
	u_int32_t	type;		/* ctfilter type */
};

#endif /*_IP6T_REJECT_H*/
