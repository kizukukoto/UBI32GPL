#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void ddns_append_fqdn(const char *fqdn, const char *path)
{
        struct in_addr in;
        FILE *fd;

        if (strcmp(fqdn, "%any") == 0)
                return;
        if (inet_pton(AF_INET, fqdn, &in) < 0)
                return; /* not fqdn format... */
        if ((fd = fopen(path, "a")) == NULL)
                goto sys_err;
        fprintf(fd, "%s\n", fqdn);
        fclose(fd);
        return;
sys_err:
        perror("ddns_append_fqdn");
        return;
}
