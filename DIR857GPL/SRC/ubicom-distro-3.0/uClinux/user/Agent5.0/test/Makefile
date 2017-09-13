
# debug use only
CC = ubicom32-linux-uclibc-gcc -march=ubicom32v4



all: udpclient sendtocli

udpclient: udpclient.c
	$(CC) -o udpclient udpclient.c
sendtocli: sendtocli.c
	$(CC) -o sendtocli sendtocli.c

clean:
	@rm -f udpclient sendtocli
