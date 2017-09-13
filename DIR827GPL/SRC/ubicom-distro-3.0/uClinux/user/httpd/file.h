/* file.h */
#ifndef _FILE_H
#define _FILE_H

size_t safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t safe_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
char *safe_fgets(char *s, int size, FILE *stream);

#endif
