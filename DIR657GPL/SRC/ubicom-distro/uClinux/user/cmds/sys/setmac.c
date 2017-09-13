#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include "libcameo.h"

#define MTD_SIZE	131072
#define SUBHEADER_LEN	20
#define VCT_VLAN	8

typedef struct {
	char header[4];
	unsigned int entry_num;
} mheader;

typedef struct {
	char header[4];
	unsigned int size;
	unsigned int type;
	char majorver[4];
	char minorver[4];
	char *payload;
} sheader;

typedef struct {
	mheader mhdr;
	sheader headers[256];
	long fsize;
} vctrl_block;

static int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[128];
	int i;

	if ((fp = fopen("/proc/mtd", "rb"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
				snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
				fclose(fp);
				return open(dev, flags);
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

static int setmac_usage()
{
	printf("Usage: setmac [devnumber] [mac addr]\n");
	printf("Example: setmac 1 001C251E9510\n");

	return -1;
}

static int setmac_validchar(char ch)
{
	int idx = 0;
	char valid_char[] = "01234567890ABCDEFabcdef";

	for (; idx < strlen(valid_char); idx++)
		if (ch == valid_char[idx])
			return 0;

	return -1;
}

static int setmac_validmac(const char *mac)
{
	int idx = 0;

	if (strlen(mac) != 12)
		return -1;

	for (; idx < strlen(mac); idx++) {
		if (setmac_validchar(mac[idx]) == -1)
			return -1;
	}

	return 0;
}

static vctrl_block *setmac_getheader()
{
	int  i = 0;
	FILE *handle = NULL;
	vctrl_block *hdr = (vctrl_block *)malloc(sizeof(vctrl_block));

	if ((handle = fopen(MTD_PARTITION, "rb")) == NULL) {
		printf("open %s error.\n", MTD_PARTITION);
		return -1;
	}

	hdr->fsize = 64 * 1024;

	if (fread((char *)&(hdr->mhdr), sizeof(char), sizeof(mheader), handle) < sizeof(mheader)) {
		return NULL;
	}

	for (; i< hdr->mhdr.entry_num; i++) {
		sheader tmp;
		char *tbuf;

		if (fread((char *)&tmp, sizeof(char), SUBHEADER_LEN, handle) < SUBHEADER_LEN) {
			return NULL;
		}

		if (tmp.size < SUBHEADER_LEN) {
			return NULL;
		}

		memcpy((void *)&(hdr->headers[tmp.type]), (void *)&tmp, SUBHEADER_LEN);
		hdr->headers[tmp.type].payload = (char *)malloc(tmp.size - SUBHEADER_LEN);
		tbuf = hdr->headers[tmp.type].payload;

		if (fread(tbuf, sizeof(char), tmp.size - SUBHEADER_LEN, handle) < (tmp.size - SUBHEADER_LEN)) {
			return NULL;
		}
	}

	if(hdr->headers[VCT_VLAN].header[0] == NULL) {
		printf("doesn't find vlaninfo in VCTRL\n");
		return NULL;
	}

	if(hdr->headers[VCT_VLAN].size <= (SUBHEADER_LEN + 1)) {
		printf("doesn't find mac address in vlaninfo\n");
		return NULL;
	}

	fclose(handle);
	return hdr;
}

static void setmac_erasemtd()
{
	int mtd_fd = open(MTD_PARTITION, O_RDWR);
	struct erase_info_user erase;

	erase.start = 0;
	erase.length = MTD_SIZE;
	ioctl(mtd_fd, MEMERASE, &erase);
	close(mtd_fd);
}

static int setmac_update(vctrl_block *hdr)
{
	FILE *mtd_fp = fopen(MTD_PARTITION, "wb");
	int i = 0;
	long fsize;
	char buf[4096];
	char payload[1024];

	fsize = hdr->fsize;
	if (fwrite((void *)&(hdr->mhdr), sizeof(char), sizeof(mheader), mtd_fp) < sizeof(mheader)) {
                printf("Write error\n");
		fclose(mtd_fp);
		return -1;
        }

        fsize -= sizeof(mheader);
	for (; i < 256; i++) {
		if (hdr->headers[i].header[0]) {
			if (fwrite((char *)&(hdr->headers[i]), sizeof(char), SUBHEADER_LEN, mtd_fp) < SUBHEADER_LEN) {
                                printf("Write error\n");
				fclose(mtd_fp);
				return -1;
                        }

			bzero(payload, sizeof(payload));
			if (hdr->headers[i].size - SUBHEADER_LEN > 0)
				memcpy(payload, hdr->headers[i].payload, hdr->headers[i].size - SUBHEADER_LEN);

			if (fwrite(payload, sizeof(char), hdr->headers[i].size -  SUBHEADER_LEN, mtd_fp) < (hdr->headers[i].size - SUBHEADER_LEN)) {
                                printf("Write error\n");
				fclose(mtd_fp);
				return -1;
			}
			fsize -= hdr->headers[i].size;
                }
        }

	bzero(buf, 4096);
	while(fsize > 0) {
		int len;

		len = ( fsize > 4096 ? 4096 : fsize);
		fwrite(buf, sizeof(char), len, mtd_fp);
		fsize -= len;
	}

	fclose(mtd_fp);
	return 0;
}

static int setmac_write(int devnumber, const char *mac)
{
	char p_string[8];
	char *find_ptr = NULL;
	vctrl_block *hdr = NULL;

	if (devnumber > 3 || devnumber < 1)
		return -1;

	hdr = setmac_getheader();
	bzero(p_string, sizeof(p_string));
	sprintf(p_string, "MAC%d", devnumber);

	if ((find_ptr = strstr(hdr->headers[VCT_VLAN].payload, p_string) + 7) == NULL) {
		printf("cannot find %s\n", p_string);
		return -1;
	}

	memcpy(find_ptr, mac, strlen(mac));
	setmac_erasemtd();
	setmac_update(hdr);

	return 1;
}

int setmac_main(int argc, char *argv[])
{
	char buf[128];

	if (argc != 3 || setmac_validmac(argv[2]) == -1)
		return setmac_usage();

	setmac_write(atoi(argv[1]), argv[2]);

	return 1;
}

static int mac_split(const char *mac, char *buf, char token)
{
	sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		mac[0], mac[1], token, mac[2], mac[3], token,
		mac[4], mac[5], token, mac[6], mac[7], token,
		mac[8], mac[9], token, mac[10], mac[11]);
}

int getmac_main(int argc, char *argv[])
{
	char buf[1024];
	char split_mac[1024];

	if (argc < 2)
		return -1;
	if (argc == 3 && strlen(argv[2]) > 1)
		return -1;

	bzero(buf, sizeof(buf));
	getmac(atoi(argv[1]), buf);

	if (argc == 3) {
		mac_split(buf, split_mac, argv[2][0]);
		strcpy(buf, split_mac);
	}

	printf("%s\n", buf);
}
