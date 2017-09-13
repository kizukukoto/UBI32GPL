
/*
 * 
 * ex: @in ="no,3des-sha1-96,3des-sha1-96,3des-sha1-96,3des-sha1-96"
 * ex: @out = "no,3des-md5-modp1024,3des-md5-modp1024,3des-md5-modp1024,3des-md5-modp1024"
 * 
 */
static int __convert_phase1(const char *in, char *out)
{
	char *st[24], *h, *p;
	int idx, i, rev = -1;
	h = strdup(in);
	
	idx = split(h, ",", st, 1);
		
	if (idx != 5)
		goto out;
	sprintf(out, "%s,", st[0]);
	for (i = 1; i < idx; i++) { 	/* skip "yes" */
		p = strrchr(st[i], '-');
		*p = '\0';		/* cut "-96" of "3des-sha1-96" */
		strcat(out, st[i]);
		strcat(out, "-modp1024,");
	}
	out[strlen(out) - 1] = '\0';
	rev = 0;
out:
	free(h);
	return rev;

}

/* 
 * Convert phase2 format of v1.00 to v.1.20.
 * ex: @s = "yes,3des-md5-96,3des-sha1-96,3des-sha1-96,3des-sha1-96"
 * ex: @buf = "yes,3des-md5,aes-md5,aes128-sha1,aes192-sha1"
 */
static int __convert_phase2(const char *in, char *out)
{
	char *st[24], *h, *p;
	int idx, i, rev = -1;
	h = strdup(in);
	
	idx = split(h, ",", st, 1);

	if (idx != 5)
		goto out;
	sprintf(out, "%s,", st[0]);	/* out = "yes," */
	for (i = 1; i < idx; i++) { 	/* skip "yes" */
		p = strrchr(st[i], '-');
		*p = '\0';		/* cut "-96" of "3des-sha1-96" */
		strcat(out, st[i]);
		strcat(out, ",");
	}
	out[strlen(out) - 1] = '\0';
	rev = 0;
out:
	free(h);
	return rev;
}

/*
 *
 * V1.10/1.12/1.20
 * "vpn_conn1=add;remote_all;tunnel,172.21.33.203-%
 * any,219.84.0.0/16-255.255.255.255/32;no,3des-md5-modp1024,3des-md5-modp1024,3des-md5-modp1024,3des-md5-modp1024;yes,3des-md5,3des-md5,3des-md5,3des-md5;PSK:123456"
 *
 * "route;site2site;tunnel,172.21.33.203-172.21.34.6,192.168.0.0/24-192.168.3.0/24;no,3des-md5-modp1024,aes-md5-modp1024,3des-sha-modp1024,aes128-sha-modp1024;yes,3des-md5,aes-md5,aes128-sha1,aes192-sha1;PSK:6543210"
 *
 * 
 * "vpn_extra1=pfsgroup=modp1024,ikelifetime=28800s,keylife=3600s,#default,#leftid=,#default,#rightid=,dpdaction=clear,dpdtimeout=120s"
 *
 * V1.00:
 * vpn_conn1="start;remote_all;tunnel,172.21.33.203-172.21.34.6,192.168.0.1/24-192.168.3.0/24;no,3des-sha1-96,3des-sha1-96,3des-sha1-96,3des-sha1-96;yes,3des-sha1-96,3des-sha1-96,3des-sha1-96,3des-sha1-96;123456;"
 *
 * 
 * vpn_conn2="add;remote_user;tunnel,172.21.33.203-%any,192.168.0.1/24-0.0.0.0/0;no,3des-sha1-96,3des-sha1-96,3des-sha1-96,3des-sha1-96;yes,3des-sha1-96,3des-sha1-96,3des-sha1-96,3des-sha1-96;12345;"
 * 
 */
void convert_ipsec(const char *ov, const char *newov)
{
	int i;
	char conn[] = "vpn_connXX";
	char *nv, buf[2048], *p, psk[128], p1[512], p2[512];

	if (strcmp(ov, "1.00") != 0)
	       	return;
	
	for (i = 0; i < 25; i++) {
		sprintf(conn, "vpn_conn%d", i);
		if (strlen((nv = nvram_safe_get(conn)))== 0)
			continue;
		if (strlen(nv) >= sizeof(buf))
			continue;			/* ignore buffer error */
		strcpy(buf, nv);
		buf[strlen(buf) - 1] = '\0';		// delete tail of ';'
		if ((p = strrchr(buf, ';')) == NULL)	// p = ";mypsk_key;"
			continue;			/* format error */
		strcpy(psk, p + 1);			/* save psk. */
		
		/* convert phase2 */
		cprintf("****** CONVERT:%s, psk:%s,%s\n", conn, psk, p);
		*p = '\0';
		cprintf("******: p:%s\n", buf);
		if ((p = strrchr(buf, ';')) == NULL)
			continue; 			/* format error! */
		if (__convert_phase2(p + 1, p2) == -1)
			continue;
		cprintf("******: p2:%s\n", buf);

		/* convert phase1 */
		*p = '\0';
		if ((p = strrchr(buf, ';')) == NULL)
			continue; 			/* format error! */
		__convert_phase1(p + 1, p1);
		cprintf("******: p1:%s\n", p1);
		
		sprintf(p + 1, "%s;%s;PSK:%s", p1, p2, psk);
		nvram_set(conn, buf);
		sprintf(conn, "vpn_extra%d", i);
		nvram_set(conn,  "pfsgroup=modp1024,ikelifetime=28800s,"
			"keylife=3600s,#default,#leftid=,#default,#rightid=,"
			"dpdaction=clear,dpdtimeout=120s");
	}
	return;
}
