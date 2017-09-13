#include <unistd.h>

int main(int argc, char **argv)
{
	char msg[] = "type some text and hit Enter, you should see a `b' for an `a' and so forth...\n";
	write(0, msg, sizeof msg - 1);
	char c;
	while(read(1, &c, 1)) {
		if (c != '\n') {
			c++;
		}
		write(0, &c, 1);
	}

	/* NOTREACHED */
	return 0;
}

