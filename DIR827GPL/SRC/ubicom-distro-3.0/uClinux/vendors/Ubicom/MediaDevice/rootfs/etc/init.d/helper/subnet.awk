# IP and netmask helper routines

# if bits is 255, 254, 252, ... returns 8, 7, 6, ...
     function subnetFieldBitCount(bits, count) {
         if (bits == 0)
             return "0"

         count = 8
     
         for (; bits != 0; bits = rshift(bits, 1))
             if (and(bits, 1))
                return count
             else
                count = count - 1
          
         return count
     }

# if mask 255.255.255.0 returns 24
# if mask 255.255.128.0 returns 17
     function subnetBitCount(mask, subnet_len) {
	 mask_fields_count = split(mask, mask_fields, ".")

         for (i = 1; i <= mask_fields_count; i++) {
	     field_len = subnetFieldBitCount(mask_fields[i])
	     if(field_len)
		 subnet_len = subnet_len + field_len
	     else 
		 return subnet_len
         }

	 return subnet_len
     }

# if ip 192.168.0.1 and mask 255.255.255.0 returns 192.168.0.0
     function subnetGet(ip, mask, subnet) {
	 ip_fields_count = split(ip, ip_fields, ".")
	 mask_fields_count = split(mask, mask_fields, ".")

         for (i = 1; i <= ip_fields_count; i++) {
	     subnet = subnet and(ip_fields[i], mask_fields[i])
	     if (i < ip_fields_count)
		 subnet = subnet "."
         }

	 return subnet
     }

# start and end ip must be in the same network with device ip
# start ip must be smaller than end ip
# device ip cannot have a value between start and end ips
     function ipRangeCheck(ip, mask, sta_ip, end_ip) {
	 subnet = subnetGet(ip, mask)
	 subnet_sta = subnetGet(sta_ip, mask)
	 subnet_end = subnetGet(end_ip, mask)
	 if (subnet_sta != subnet || subnet_end != subnet)
	     return 1
	 ip_fields_count = split(ip, ip_fields, ".")
	 sta_fields_count = split(sta_ip, sta_fields, ".")
	 end_fields_count = split(end_ip, end_fields, ".")
         for (i = 1; i <= sta_fields_count; i++) {
	     if (sta_fields[i] > end_fields[i])
		 return 2
	     if ((ip_fields[i] > sta_fields[i]) || ((i == sta_fields_count) && (ip_fields[i] == sta_fields[i])))
		 return 3
	 }
	 return 0
     }

     BEGIN {
	 if (args == "s")
	     printf "%s", subnetGet(ip, mask)
	 else if (args == "b")
	     printf "%s", subnetBitCount(mask)	     
	 else if (args == "sb")
	     printf "%s/%s", subnetGet(ip, mask), subnetBitCount(mask)	     
	 else if (args == "rc")
	     printf "%s", ipRangeCheck(ip, mask, sta_ip, end_ip)
     }
