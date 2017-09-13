#include <stdio.h>
#include <unistd.h>
void foo(){
	char *x = malloc(1024 * 1024);
	fprintf(stderr, "%s:%d:\n", __FUNCTION__, __LINE__);
	pause();
	fprintf(stderr, "%s:%d:\n", __FUNCTION__, __LINE__);
	free(x);
}
int main()
{
	foo();
}
