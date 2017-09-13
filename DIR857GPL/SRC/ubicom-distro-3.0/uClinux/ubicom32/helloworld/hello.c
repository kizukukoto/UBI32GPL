#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int i = 0;
	while(1) {
		printf("hello world! %d", i++);
		printf("\n");
		printf("Testing Ubicom UcLibc implementation.");
		printf("\n");
		printf("Testing conducted by Nat Gurumoorthy.");
		printf("\n");
		printf("\n");
		
		fflush(stdout);
		sleep(5);
	}

	/* NOTREACHED */
	return 0;
}

