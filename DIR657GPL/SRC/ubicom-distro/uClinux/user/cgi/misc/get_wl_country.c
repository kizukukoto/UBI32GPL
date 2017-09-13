#include <stdio.h>
#include <strings.h>
#include <string.h>


/* The function will change the country to 0XXX code.
 * Maybe you have better method to rewrite the code.
*/
int get_wl_country_main(int argc, char *argv[])
{
	int ret = -1;
	FILE *fp;
	char country[32], *p;

	bzero(country, sizeof(country));

	fp = popen("/usr/sbin/wl country", "r");

	if(fp == NULL)
		goto out;

	if(fgets(country, sizeof(country), fp) == NULL)
		goto out;
	else{
		if((p = strstr(country, "US/")) != NULL){
			printf("0X3A\n");
		}else if((p = strstr(country, "CA/")) != NULL){
			printf("0X14\n");
		}else if(((p = strstr(country, "AU/")) != NULL) ||
			((p = strstr(country, "HK/")) != NULL)){
			printf("0X23\n");
		}else if((p = strstr(country, "SG/")) != NULL ||
			((p = strstr(country, "IN/")) != NULL)){
			printf("0X5B\n");
		}else if((p = strstr(country, "CN/")) != NULL){
			printf("0X52\n");
		}else if(((p = strstr(country, "JP/")) != NULL) ||
			((p = strstr(country, "NL/")) != NULL) ||
			((p = strstr(country, "SA/")) != NULL) ||
			((p = strstr(country, "RU/")) != NULL)){
			printf("0X37\n");
		}else if((p = strstr(country, "CL/")) != NULL){
			printf("0X51\n");
		}else{
			printf("0X00\n");
		}
	}
		ret = 0;
out:
	return ret;
}
