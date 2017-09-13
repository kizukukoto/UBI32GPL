#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmds.h"


#define IPSEC_STATUS_CMD "ip xfrm policy list dir fwd|"			\
	"awk \'/^src/{ printf \"%s,%s,\",$2,$4;next};"			\
	"/dst/{ printf \"%s,%s,\",$3,$5;next};"				\
	"/proto/{ print $6}\'"
//#define IPSEC_CIPHER_CMD "ipsec auto status"
//#define IPSEC_CIPHER_CMD "ipsec whack --status"
#define IPSEC_CIPHER_CMD "whack --status"

static struct ipsec_entry {
	char remote_addr_h[20];
	char local_network_h[20];
	char remote_addr[16];
	char local_addr[16];
};

static struct ipsec_entry utility_ipsec_getentry(const char *result)
{
	struct ipsec_entry entry;

	sscanf(result, "%[^,],%[^,],%[^,],%[^,]",
			entry.remote_addr_h, entry.local_network_h,
			entry.remote_addr, entry.local_addr);

	return entry;
}

static void utility_ipsec_getconnnum(const char *result, char *cipher_conn)
{
	int num = -1;
	char __garbage_str[128];
	char __garbage_str2[128];

	bzero(__garbage_str, sizeof(__garbage_str));
	bzero(__garbage_str2, sizeof(__garbage_str2));
	
	//fprintf(stderr, "%s%d:%s\n", __FUNCTION__, __LINE__, result);
	sscanf(result, "%s %s %s", __garbage_str, __garbage_str2, cipher_conn);
}

static void utility_ipsec_specialchar(const char *cipher_conn, char *__cipher_conn)
{
	int idx1 = 0;
	int idx2 = 0;

	while (cipher_conn[idx1] != '\0') {
		if (cipher_conn[idx1] == '\"' ||
			cipher_conn[idx1] == '[' ||
			cipher_conn[idx1] == ']') {
			__cipher_conn[idx2++] = '\\';
			__cipher_conn[idx2++] = cipher_conn[idx1++];
		} else {
			__cipher_conn[idx2++] = cipher_conn[idx1++];
		}
	}
}

static void utility_ipsec_getinfo(
	const char *__cipher_conn,
	const char *keyword,
	int column,
	char *buf)
{
	FILE *fp;
	char cmds[128];

	sprintf(cmds, "%s|grep \"%s:\"|grep %s|awk '{print $%d}'",
			IPSEC_CIPHER_CMD,
			__cipher_conn,
			keyword,
			column);

	if ((fp = popen(cmds, "r")) == NULL)
		return;

	while(fscanf(fp, "%[^\n]\n", buf) != EOF) {
		//printf("%s cipher: %s\n", __cipher_conn, esp_cipher);
	}

	pclose(fp);

	if (buf != NULL && *buf != '\0' &&
		(buf[strlen(buf)-1] == ';' || buf[strlen(buf)-1] == ','))
		buf[strlen(buf) - 1] = '\0';
}

static void utility_ipsec_getcipher(
	const char *__cipher_conn,
	const char *keyword,
	char *buf)
{
	utility_ipsec_getinfo(__cipher_conn, keyword, 7, buf);
}

static void utility_ipsec_getdhgroup(
	const char *__cipher_conn,
	const char *keyword,
	char *buf)
{
	char *p, local_buf[128];

	/* @local_buf: pfsgroup=MODP1024 */
	utility_ipsec_getinfo(__cipher_conn, keyword, 8, local_buf);
	if ((p = strstr(local_buf, "=")) != NULL)
		strcpy(buf, p + 1);
	if (strcmp(buf, "<N/A>") == 0)
		strcpy(buf, "No DH");
}

static void utility_ipsec_cipher(
	const char *result,
	char *esp_cipher,
	char *ike_cipher,
	char *dh_group)
{
	struct ipsec_entry entry = utility_ipsec_getentry(result);
	FILE *fp = NULL;
	char cmds[128];
	char cipher_result[256];
	char cipher_conn[256];
	char __cipher_conn[256];
	char cipher_cmds[256];

	bzero(cipher_result, sizeof(cipher_result));
	bzero(cipher_conn, sizeof(cipher_conn));
	bzero(cipher_cmds, sizeof(cipher_cmds));
	bzero(__cipher_conn, sizeof(__cipher_conn));

	sprintf(cmds, "%s|grep \"newest IPSEC\"|grep %s", IPSEC_CIPHER_CMD, entry.remote_addr);
	//fprintf(stderr, "%s%d:%s\n", __FUNCTION__, __LINE__, cmds);
	if ((fp = popen(cmds, "r")) == NULL)
		return;
	
	while(fscanf(fp, "%[^\n]\n", cipher_result) != EOF) {
		//fprintf(stderr, "1: %s\n", cipher_result);
	}

	pclose(fp);

	/* @cipher_result: ## original data in `ipsec whack --status|grep [remote_addr]` ##
	 * @cipher_conn: "conn_ipsec"[1]
	 * @__cipher_conn: \"conn_ipsec\"\[1\]
	 * because of @__cipher_conn used in awk script, '\' charactor be necessary.
	 * */

	utility_ipsec_getconnnum(cipher_result, cipher_conn);
	utility_ipsec_specialchar(cipher_conn, __cipher_conn);

	utility_ipsec_getcipher(__cipher_conn, "DIR730_ESP", esp_cipher);
	utility_ipsec_getcipher(__cipher_conn, "DIR730_IKE", ike_cipher);
	utility_ipsec_getdhgroup(__cipher_conn, "DIR730_ESP", dh_group);
}

int utility_ipsec_main(int argc, char *argv[])
{
	FILE *fp = popen(IPSEC_STATUS_CMD, "r");
	char result[128];

	if (fp == NULL) goto out;

	bzero(result, sizeof(result));

	/* @result like: 172.21.33.207/32,192.168.0.0/24,172.21.33.207,172.21.33.71,tunnel
	 * 172.21.33.207: remote addr
	 * 172.21.33.71: dir-730
	 * */
	while(fscanf(fp, "%s", result) != EOF) {
		#define temp  "unknow"
		char esp_cipher[128] =temp, ike_cipher[128] = temp, dh_group[128] = temp;

		bzero(esp_cipher, sizeof(esp_cipher));
		bzero(ike_cipher, sizeof(ike_cipher));
		utility_ipsec_cipher(result, esp_cipher, ike_cipher, dh_group);

		/* @esp_cipher: 3DES_0-HMAC_SHA1
		 * @ike_cipher: 3DES_CBC_192-MD5-MODP1024
		 * */
		printf("%s,%s%s%s,%s%s,%s#",
			result,
			(*esp_cipher == '\0')?"":"ESP: ",
			esp_cipher,
			(*ike_cipher == '\0')?"":"<br>",
			(*ike_cipher == '\0')?"":"IKE: ",
			ike_cipher,
			dh_group);
		bzero(result, sizeof(result));
	}

	pclose(fp);
out:
	return 0;
}
