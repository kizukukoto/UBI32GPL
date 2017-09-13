#ifndef __HEADER__
#define __HEADER__

/*
 * DIR-730 Image Header:
 *
 * offset  0             12                 24                        48
 *         ------------------------------------------------------------
 *         | Kernel Size | File System Size |       Hardware ID       |
 *         |  12 bytes   |     12 bytes     |         24 bytes        |
 *         ------------------------------------------------------------
 *
 */

#define HOFF	24
#define KOFF	0
#define FOFF	12

typedef struct firmware_header {

	char mb[48];
}firmware_header;

#endif
