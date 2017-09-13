 /* 
 * file.c -- Read / Write file steam function *
 * Rewrite by anderson_chen@cameo.com.tw Dec 2005
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <assert.h>
#include <assert.h>
#include <httpd.h>

/* Read NMEMB elements of SIZE bytes into PTR from STREAM.  Returns the
 * number of elements read, and a short count if an eof or non-interrupt
 * error is encountered.  */

size_t safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = 0;

/*   
*   Date: 2009-4-29
*   Name: Yufa Huang
*   Reason: Fix that HTTPS can not upgrade firmware.
*   Notice :
*/
#if defined(HAVE_HTTPS)
    if (openssl_request) {
        return BIO_read((BIO *) stream, ptr, nmemb * size);
    }
#endif

    do {
        clearerr(stream);
        ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
    } while (ret < nmemb && ferror(stream) && errno == EINTR);

    return ret;
}


/* Write NMEMB elements of SIZE bytes from PTR to STREAM.  Returns the
 * number of elements written, and a short count if an eof or non-interrupt
 * error is encountered.  */
size_t safe_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = 0;

    do {
        clearerr(stream);
        ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
    } while (ret < nmemb && ferror(stream) && errno == EINTR);

    return ret;
}


/* Read a line or SIZE - 1 bytes into S, whichever is less, from STREAM.
 * Returns S, or NULL if an eof or non-interrupt error is encountered.  */
char *safe_fgets(char *s, int size, FILE *stream)
{
    char *ret;

    do {
        clearerr(stream);
        ret = fgets(s, size, stream);
    } while (ret == NULL && ferror(stream) && errno == EINTR);

    return ret;
}
