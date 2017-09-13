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

# if ip 192.168.0.51 and mask 255.255.255.0 returns 0.0.0.51
     function subnetGetNot(ip, mask, subnet) {
	 ip_fields_count = split(ip, ip_fields, ".")
	 mask_fields_count = split(mask, mask_fields, ".")

         for (i = 1; i <= ip_fields_count; i++) {
	     subnet = subnet and(ip_fields[i], compl(mask_fields[i]))
	     if (i < ip_fields_count)
		 subnet = subnet "."
         }

	 return subnet
     }

# if subnet is 192.168.0.0 and offset is 0.0.0.10, then total is 192.168.0.10
     function subnetPlusOffset(subnet, offset, total) {
	 subnet_fields_count = split(subnet, subnet_fields, ".")
	 offset_fields_count = split(offset, offset_fields, ".")

         for (i = 1; i <= subnet_fields_count; i++) {
	     field_total = subnet_fields[i] + offset_fields[i]
	     total =  total field_total
	     if (i < subnet_fields_count)
		 total = total "."
         }

	 return total     
     }

# value of tuned_ip will be changed according to ip and mask
# if ip=182.68.0.139, mask=255.255.255.0, tuned_ip=192.168.0.140, then 
# 182.68.0.140 will be returned
     function tuneIP(ip, mask, prev_mask, tuned_ip, is_start) {
	 subnet = subnetGet(ip, mask)
	 bit_count_new=subnetBitCount(mask)
	 bit_count_old=subnetBitCount(prev_mask)
#	 printf "%s:%s-%s:%s\n", mask, bit_count_new, prev_mask, bit_count_old
	 if (is_start == "y") {
	     if (bit_count_old < bit_count_new) # if it is narrowing, use 1 for lower boundary
		 return subnetPlusOffset(subnet, "0.0.0.1")
	 } else {
	     if (bit_count_old < bit_count_new) { # if it is narrowing, use max value  for upper boundary
		 expo_val = 32 - bit_count_new
		 upper_val = 2 ^ expo_val
		 upper_val = upper_val - 2
		 if (upper_val > 254)
		     upper_val = 254
		 return subnetPlusOffset(subnet, "0.0.0." upper_val)
	     }
	 }
	 tuned_offset = subnetGetNot(tuned_ip, mask)
	 return subnetPlusOffset(subnet, tuned_offset)
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
	 else if (args == "ti")
	     printf "%s", tuneIP(ip, mask, prev_mask, tuned_ip, is_start)
     }
