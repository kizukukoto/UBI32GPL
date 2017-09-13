#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PROC_VPN	"/proc/sys/net/vpn/vpn_pair"

struct vpn_pair{
	int en;
	int dir;
	int src, src_mask, left;
	int dst, dst_mask, right;
	int peer;
};

static void dump(struct vpn_pair *v)
{
	char src[INET_ADDRSTRLEN], src_mask[INET_ADDRSTRLEN], left[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN], dst_mask[INET_ADDRSTRLEN], right[INET_ADDRSTRLEN];
	char peer[INET_ADDRSTRLEN];
	v->src = ntohl(v->src);
	v->src_mask = ntohl(v->src_mask);
	v->left = ntohl(v->left);

	if (inet_ntop(AF_INET, &v->src, src, sizeof(src)) == NULL) goto out;
	if (inet_ntop(AF_INET, &v->src_mask, src_mask, sizeof(src_mask)) == NULL) goto out;
	if (inet_ntop(AF_INET, &v->left, left, sizeof(left)) == NULL) goto out;

	printf("%d %d %s/%s %s --->", v->en, v->dir, src, src_mask, left);

	v->dst = ntohl(v->dst);
	v->dst_mask = ntohl(v->dst_mask);
	v->right = ntohl(v->right);
	v->peer = ntohl(v->peer);
	if (inet_ntop(AF_INET, &v->dst, dst, sizeof(dst)) == NULL) goto out;
	if (inet_ntop(AF_INET, &v->dst_mask, dst_mask, sizeof(dst_mask)) == NULL) goto out;
	if (inet_ntop(AF_INET, &v->right, right, sizeof(right)) == NULL) goto out;
	if (inet_ntop(AF_INET, &v->peer, peer, sizeof(peer)) == NULL) goto out;
	printf("%s/%s %s %s\n", dst, dst_mask, right, peer);

out:
	return;
}

int hw_vpn_main(int argc, char *argv[])
{
	FILE *fd;
	int rev = -1, offset, i = 0;
	char buf[4096], *proc = PROC_VPN;
	struct vpn_pair v1, *vp1 = &v1;
	struct vpn_pair v2, *vp2 = &v2;

	if (argc >= 2)
		proc = argv[1];
	
	if ((fd = fopen(proc, "r")) == NULL)
		goto out;

	while (1) {
		printf("=====TUNNEL: %d\n", i);
		offset = fscanf(fd, "%d %d %d %d %d %d %d %d %d*",
			&vp1->en, &vp1->dir,
			&vp1->src, &vp1->src_mask, &vp1->left,
			&vp1->dst, &vp1->dst_mask, &vp1->right, &vp1->peer);
		//printf("OFFSET %d\n", offset);
		if (offset != 9)
			break;
		dump(vp1);
		offset = fscanf(fd, "%d %d %d %d %d %d %d %d %d*",
			&vp1->en, &vp1->dir,
			&vp1->src, &vp1->src_mask, &vp1->left,
			&vp1->dst, &vp1->dst_mask, &vp1->right, &vp1->peer);
		if (offset != 9)
			break;
		
		dump(vp1);
		i++;
	}
	rev = 0;
out:
	return rev;
}
