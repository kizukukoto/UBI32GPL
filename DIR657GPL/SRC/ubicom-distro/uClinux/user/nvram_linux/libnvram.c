#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "typedefs.h"
#include "nvram.h"

//#define NVRAM_DEBUG
#ifdef NVRAM_DEBUG
#undef DEBUG_MSG(fmt, arg...)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#undef DEBUG_MSG
#define DEBUG_MSG(fmt, arg...)
#endif

#define PATH_DEV_NVRAM "/dev/nvram"



/* Globals */
static int nvram_fd = -1;
static char *nvram_buf = NULL;

static int NVRAM_SPACE;
int nvram_get_nvramspace()
{
	if (nvram_fd < 0)
		if (nvram_init(NULL)){
			DEBUG_MSG("%s: nvram_init fail\n",__func__);
			return -1;
		}	
	return NVRAM_SPACE;
}
	
int
nvram_init(void *unused)
{
	if ((nvram_fd = open(PATH_DEV_NVRAM, O_RDWR)) < 0){
		DEBUG_MSG("%s: open fail\n",__func__);
		goto err;
	}	
	DEBUG_MSG("%s: nvram_fd=%d\n",__func__,nvram_fd);
	ioctl(nvram_fd, NVRAM_IOCGSIZE, &NVRAM_SPACE);

	/* Map kernel string buffer into user space */
	if ((nvram_buf = mmap(NULL, NVRAM_SPACE, PROT_READ, MAP_PRIVATE, nvram_fd, 0)) == MAP_FAILED) {
		perror("mmap: ");
		close(nvram_fd);
		nvram_fd = -1;
		goto err;
	}
	return 0;

 err:
	perror(PATH_DEV_NVRAM);
	return errno;
}

char *
nvram_get(const char *name)
{
	char tmp[NVRAM_MAX_VALUE_LEN], *value;
	char *off = tmp;
	size_t count = sizeof(tmp);	

	if (nvram_fd < 0)
		if (nvram_init(NULL)){
			DEBUG_MSG("%s: nvram_init fail\n",__func__);
			return NULL;
	}		
	DEBUG_MSG("%s: nvram_fd=%d\n",__func__,nvram_fd);
	
	/* Get offset into mmap() space */
	strcpy(off, name);
    	//printf("%s: name=%s,count=%d, tmp: %p, off:%p\n",__func__,name,count, tmp, off);
	count = read(nvram_fd, off, count);
	if (count == sizeof(off)) {
		value = *(unsigned long *) off;
	//	printf("%s:%d ###  off=%s, count=%x, nvram_buf:%08X, off:%p, off:%s\n",
//		__func__, __LINE__, off,count, nvram_buf, off, *(unsigned long *)off);	
	} else {
		value = NULL;
	} 
	/*
	if (count == strlen(off)){		
		value = off;
		printf("%s: value=%s\n",__func__,value);
	}	
	else{
		value = NULL;
		DEBUG_MSG("%s: value=NULL\n",__func__);
	}	
	*/
	if (count < 0)
		perror(PATH_DEV_NVRAM);	

	return value;
}

int
nvram_getall(char *buf, int count)
{
	int ret;

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL))){
			DEBUG_MSG("%s: nvram_init fail\n",__func__);
			return ret;
		}	
	DEBUG_MSG("%s: nvram_fd=%d\n",__func__,nvram_fd);
	if (count == 0){
		DEBUG_MSG("%s: count fail\n",__func__);
		return 0;
	}	
	
	/* Get all variables */
	*buf = '\0';
	DEBUG_MSG("%s: count=%x\n",__func__,count);
	ret = read(nvram_fd, buf, count);

	if (ret < 0)
		perror(PATH_DEV_NVRAM);

#ifdef NVRAM_DEBUG
	{
		char *name;
		for (name = buf; *name; name += strlen(name) + 1)
			puts(name);
	}	
#endif	
	DEBUG_MSG("%s: ret=%x,count=%x\n",__func__,ret,count);
	return (ret == count) ? 0 : ret;
}

static int
_nvram_set(const char *name, const char *value)
{
	size_t count = strlen(name) + 1;
	char tmp[NVRAM_MAX_PARAM_LEN+NVRAM_MAX_VALUE_LEN], *buf = tmp;
	int ret=0;

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL))){
			DEBUG_MSG("%s: nvram_init fail\n",__func__);
			return ret;
		}	
	DEBUG_MSG("%s: nvram_fd=%d\n",__func__,nvram_fd);
	/* Unset if value is NULL */
	if (value)
		count += strlen(value) + 1;

	if (count > sizeof(tmp)) {
		if (!(buf = malloc(count)))
			return -ENOMEM;			
	}

	if (value)
		sprintf(buf, "%s=%s", name, value);
	else
		strcpy(buf, name);

	DEBUG_MSG("%s: write=%s\n",__func__,buf);
	ret = write(nvram_fd, buf, count);
	if (ret < 0)
		perror(PATH_DEV_NVRAM);	
	if (buf != tmp)
		free(buf);

	return (ret == count) ? 0 : ret;
}

int
nvram_set(const char *name, const char *value)
{
	return _nvram_set(name, value);
}

int
nvram_unset(const char *name)
{
	return _nvram_set(name, NULL);
}

int
nvram_commit(void)
{
	int ret;

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL))){
			DEBUG_MSG("%s: nvram_commit fail\n",__func__);
			return ret;
		}	

	ret = ioctl(nvram_fd, NVRAM_MAGIC, NULL);

	if (ret < 0)
		perror(PATH_DEV_NVRAM);

	return ret;
}
