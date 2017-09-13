# this file contains general utility functions
# execute this file from your shell scripts, usage example
#   helper_utils=/etc/init.d/helper/utils.awk
#   awk -v args=ec -v number=122 -f $helper_utils

# if number is even return 1, else return 0
     function is_even(number) {
	 result = number % 2
	 if (result == 0)
	     return 1
	 return 0
     }

     BEGIN {
	 if (args == "ec")
	     printf "%s", is_even(number)
     }
