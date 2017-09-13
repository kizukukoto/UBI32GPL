#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/wrapper.h>
#include <asm/memory.h>
#include <asm/addrspace.h>
#else
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <asm/cacheflush.h> 	/* flush_cache_all() ?*/
#endif

#include "nvram.h"
#define      ROUNDUP(x, y)           ((((x)+((y)-1))/(y))*(y))
#define MTD_WRITE(mtd, args...) (*(mtd->write))(mtd, args)
#define MTD_READ(mtd, args...) (*(mtd->read))(mtd, args)
#define MTD_ERASE(mtd, args...) (*(mtd->erase))(mtd, args)
//#define NVRAM_DEBUG
#ifdef NVRAM_DEBUG
#define NVDBG			printk
static void dump_hex(char *d, int len)
{
	int i = 0;

	for (;i < len; i++) {
		if (0x20 <= d[i] && d[i] <= 0x7E)
			printk("%c", d[i]);
		else
			printk("%2X", d[i]);
	}
}
static void dump_erase_info(struct erase_info *e)
{
	printk("erase info:\n"
		"\tname: %s\n"
		"\taddr: %08X\n"
		"\tlen: %08X\n",
		e->mtd->name, e->addr, e->len);
}
#if 0
static void dump_flash_regs(void)
{
	int reg, base, tail;
	
	base = SL2312_GLOBAL_BASE_ADDR;
	tail = base + 0x30;
	printk("DUMP REGS===============\n");
	for (reg = base; reg <= tail; reg += 4) {
		printk("%2x %8x\n", reg, readl(reg));
		
	}
	printk("END REGS================\n");
}
#endif
#else		/* NVRAM_DEBUG */
#define NVDBG(fmt, a...)	do { } while(0)
#define dump_hex(d, len)	do { } while(0)
#define dump_erase_info(e)	do { } while(0)
#endif		/* NVRAM_DEBUG */

static inline void set_page_reserved(struct page *page)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	 mem_map_reserve(page);
#else
	SetPageReserved(page);
#endif
}
static inline void clear_page_reserved(struct page *page)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	mem_map_unreserve(page);
#else
	ClearPageReserved(page);
#endif
}

/* In BSS to minimize text size and page aligned so it can be mmap()-ed */
//static char nvram_buf[NVRAM_SPACE] __attribute__((aligned(PAGE_SIZE)));
int NVRAM_SPACE = -1;
#define NVRAM_SPACE_MAX		0x20000	/* 128k limited by __get_free_page() */
static char *nvram_buf;

extern char * _nvram_get(const char *name);
extern int _nvram_set(const char *name, const char *value);
extern int _nvram_unset(const char *name);
extern int _nvram_getall(char *buf, int count);
extern int _nvram_commit(struct nvram_header *header);
extern int _nvram_init(void);
extern void _nvram_exit(void);
/* Globals */
static spinlock_t nvram_lock = SPIN_LOCK_UNLOCKED;
static struct semaphore nvram_sem;
static unsigned long nvram_offset = 0;
static int nvram_major = 225;
static struct mtd_info *nvram_mtd = NULL;

EXPORT_SYMBOL(nvram_get);
EXPORT_SYMBOL(nvram_getall);
EXPORT_SYMBOL(nvram_set);
EXPORT_SYMBOL(nvram_unset);
EXPORT_SYMBOL(nvram_commit);

/**********************************
 * For Storlink SL3512 SPEC
 **********************************/

#ifndef CONFIG_SL2312_SHARE_PIN
#define SL2312_SHARE_PIN_ENABLE()
#define SL2312_SHARE_PIN_DISABLE()
#else
#define SL2312_SHARE_PIN_ENABLE()	sl2312flash_enable_parallel_flash()
#define SL2312_SHARE_PIN_DISABLE()	sl2312flash_disable_parallel_flash() 

#include <asm/arch/sl2312.h>
#include <asm/arch-sl2312/hardware.h>

/* the base address of FLASH control register */
#define FLASH_CONTROL_BASE_ADDR     (IO_ADDRESS(SL2312_FLASH_CTRL_BASE))
#define SL2312_GLOBAL_BASE_ADDR     (IO_ADDRESS(SL2312_GLOBAL_BASE))

/* define read/write register utility */
#define FLASH_READ_REG(offset)                  (__raw_readl(offset+FLASH_CONTROL_BASE_ADDR))
#define FLASH_WRITE_REG(offset,val)     (__raw_writel(val,offset+FLASH_CONTROL_BASE_ADDR))

static void sl2312flash_enable_parallel_flash(void)
{
	unsigned int    reg_val;
	
	NVDBG("Paralled_flash:%X, %X\n", SL2312_GLOBAL_BASE_ADDR, SL2312_GLOBAL_BASE);
	reg_val = readl(SL2312_GLOBAL_BASE_ADDR + 0x30);
	NVDBG("read reg:%8X\n", reg_val);
	reg_val = reg_val & 0xfffffffd;
	writel(reg_val,SL2312_GLOBAL_BASE_ADDR + 0x30);
	return;
}

static void sl2312flash_disable_parallel_flash(void)
{

	unsigned int    reg_val;

	reg_val = readl(SL2312_GLOBAL_BASE_ADDR + 0x30);
	reg_val = reg_val | 0x00000002;
	writel(reg_val,SL2312_GLOBAL_BASE_ADDR + 0x30);
	return;
}
#endif
/*
#ifdef MTD_READ
#define SLMTD_READ	MTD_READ
#undef MTD_READ
#define MTD_READ	SLMTD_READ
#define MTD_READ(mtd, args...)				\
	{						\
		sl2312flash_enable_parallel_flash();	\
		(*(mtd_read))(mtd, args);		\
		sl2312flash_disable_parallel_flash();	\
	}
#endif
*/
/**********************************
 * END For Storlink SL3512 SPEC
 **********************************/

int
_nvram_read(char *buf)
{
	struct nvram_header *header = (struct nvram_header *) buf;
	size_t len = 0;

	SL2312_SHARE_PIN_ENABLE();
	if (!nvram_mtd ||
		    MTD_READ(nvram_mtd, nvram_mtd->size - NVRAM_SPACE,
		    NVRAM_SPACE, &len, buf) || len != NVRAM_SPACE ||
		    header->magic != NVRAM_MAGIC)
	{
		/* Maybe we can recover some data from early initialization */
		memcpy(buf, nvram_buf, NVRAM_SPACE);
	}
	SL2312_SHARE_PIN_DISABLE();

	return 0;
}


static void
erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

struct nvram_tuple *
_nvram_realloc(struct nvram_tuple *t, const char *name, const char *value)
{
	if ((nvram_offset + strlen(value) + 1) > NVRAM_SPACE)
		return NULL;

	if (!t) {
		if (!(t = kmalloc(sizeof(struct nvram_tuple) + strlen(name) + 1, GFP_ATOMIC)))
			return NULL;

		/* Copy name */
		t->name = (char *) &t[1];
		strcpy(t->name, name);

		t->value = NULL;
	}

	/* Copy value */
	if (!t->value || strcmp(t->value, value)) {
		t->value = &nvram_buf[nvram_offset];
		strcpy(t->value, value);
		nvram_offset += strlen(value) + 1;
	}

	return t;
}
void
_nvram_free(struct nvram_tuple *t)
{
	if (!t)
		nvram_offset = 0;
	else
		kfree(t);
}
int
nvram_set(const char *name, const char *value)
{
	unsigned long flags;
	int ret=0;
	struct nvram_header *header;
	
	NVDBG("%s: name=%s,value=%s\n",__func__,name,value);
	spin_lock_irqsave(&nvram_lock, flags);
	if ((ret = _nvram_set(name, value))) {
		NVDBG("%s: _nvram_set fail name=%s,value=%s\n",__func__,name,value);
		/* Consolidate space and try again */
		if ((header = kmalloc(NVRAM_SPACE, GFP_ATOMIC))) {
			NVDBG("%s: _nvram_set kmalloc header\n");
			if (_nvram_commit(header) == 0){
				printk("%s: _nvram_commit header _nvram_set name=%s,value=%s\n",__func__,name,value);
				ret = _nvram_set(name, value);
			}	
			kfree(header);
		}
		else
			printk("%s: _nvram_set kmalloc header fail\n", __FUNCTION__);
	}
	else
		NVDBG("%s: _nvram_set ok\n",__func__);
	spin_unlock_irqrestore(&nvram_lock, flags);
	NVDBG("%s: ret=%d\n",__func__,ret);
	return ret;
}

char *
real_nvram_get(const char *name)
{
	unsigned long flags;
	char *value;
	NVDBG("%s: name=%s\n",__func__,name);
	spin_lock_irqsave(&nvram_lock, flags);
	value = _nvram_get(name);
	spin_unlock_irqrestore(&nvram_lock, flags);
	NVDBG("%s: value=%s ok\n",__func__,value);
	return value;
}

char *
nvram_get(const char *name)
{
	return real_nvram_get(name);
	/*
	if (nvram_major >= 0)
		return real_nvram_get(name);
	else
		return early_nvram_get(name);
	*/
}

int
nvram_unset(const char *name)
{
	unsigned long flags;
	int ret=0;
    NVDBG("%s: name=%s\n",__func__,name);
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_unset(name);
	spin_unlock_irqrestore(&nvram_lock, flags);
	NVDBG("%s: ret=%d\n",__func__,ret);
	return ret;
}

int
nvram_commit(void)
{
	char *buf;
	size_t erasesize, len, magic_len;
	unsigned int i;
	int ret;
	struct nvram_header *header;
	unsigned long flags;
	u_int32_t offset;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	struct erase_info erase;
	u_int32_t magic_offset = 0; /* Offset for writing MAGIC # */

	if (!nvram_mtd) {
		printk("nvram_commit: NVRAM not found\n");
		return -ENODEV;
	}

	if (in_interrupt()) {
		printk("nvram_commit: not committing in interrupt\n");
		return -EINVAL;
	}
	
	/* Backup sector blocks to be erased */
	erasesize = ROUNDUP(NVRAM_SPACE, nvram_mtd->erasesize);
	if (!(buf = kmalloc(erasesize, GFP_KERNEL))) {
		printk("nvram_commit: out of memory\n");
		return -ENOMEM;
	}
	printk("size crc %X erasesize:%X, nvram->size: %X\n",
		nvram_mtd->erasesize, erasesize, nvram_mtd->size);
	down(&nvram_sem);

	if ((i = erasesize - NVRAM_SPACE) > 0) {
		offset = nvram_mtd->size - erasesize;
		len = 0;
		SL2312_SHARE_PIN_ENABLE();
		ret = MTD_READ(nvram_mtd, offset, i, &len, buf);
		SL2312_SHARE_PIN_DISABLE();
		if (ret || len != i) {
			printk("nvram_commit: read error ret = %d, len = %d/%d\n", ret, len, i);
			ret = -EIO;
			goto done;
		}
		header = (struct nvram_header *)(buf + i);
		magic_offset = i + ((void *)&header->magic - (void *)header);
	} else {
		offset = nvram_mtd->size - NVRAM_SPACE;
		magic_offset = ((void *)&header->magic - (void *)header);
		header = (struct nvram_header *)buf;
	}

	/* clear the existing magic # to mark the NVRAM as unusable 
		 we can pull MAGIC bits low without erase	*/
	header->magic = NVRAM_CLEAR_MAGIC; /* All zeros magic */
	NVDBG("offset:%X, magic_offset:%X\nMTD WRITE( %X, %X)\n",
	       offset, magic_offset,
	       offset + magic_offset, sizeof(header->magic));
	ret = MTD_WRITE(nvram_mtd, offset + magic_offset, sizeof(header->magic), 
									&magic_len, (char *)&header->magic);
	NVDBG("MTD WRITE ret: %d, len:%x\n", ret, len);
	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: clear MAGIC error\n");
		ret = -EIO;
		goto done;
	}
	header->magic = NVRAM_MAGIC; /* reset MAGIC before we regenerate the NVRAM,
																otherwise we'll have an incorrect CRC */
	/* Regenerate NVRAM */
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_commit(header);
	spin_unlock_irqrestore(&nvram_lock, flags);
	if (ret)
		goto done;
	NVDBG("MTD_ERASE ing\n");
	/* Erase sector blocks */
	init_waitqueue_head(&wait_q);
	for (; offset < nvram_mtd->size - NVRAM_SPACE + header->len; offset += nvram_mtd->erasesize) {
		erase.mtd = nvram_mtd;
		erase.addr = offset;
		erase.len = nvram_mtd->erasesize;
		erase.callback = erase_callback;
		erase.priv = (u_long) &wait_q;
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);
		dump_erase_info(&erase);
		/* Unlock sector blocks */
		if (nvram_mtd->unlock)
			nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);

		if ((ret = MTD_ERASE(nvram_mtd, &erase))) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk("nvram_commit: erase error\n");
			goto done;
		}
		/* Wait for erase to finish */
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}
	/* Write partition up to end of data area */
	header->magic = NVRAM_INVALID_MAGIC; /* All ones magic */
	offset = nvram_mtd->size - erasesize;
	i = erasesize - NVRAM_SPACE + header->len;
	NVDBG("MTD_WRITE(%X, %X)\n", offset, i);
	SL2312_SHARE_PIN_ENABLE();
	ret = MTD_WRITE(nvram_mtd, offset, i, &len, buf);
	SL2312_SHARE_PIN_DISABLE();
	if (ret || len != i) {
		printk("nvram_commit: write error\n");
		ret = -EIO;
		goto done;
	}
	/* Now mark the NVRAM in flash as "valid" by setting the correct
		 MAGIC # */
	header->magic = NVRAM_MAGIC;

	NVDBG("MTD_WRITE(%X, %X)\n", offset + magic_offset, sizeof(header->magic));	
	SL2312_SHARE_PIN_ENABLE();
	ret = MTD_WRITE(nvram_mtd, offset + magic_offset, sizeof(header->magic), 
									&magic_len, (char *)&header->magic);
	SL2312_SHARE_PIN_DISABLE();

	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: write MAGIC error\n");
		ret = -EIO;
		goto done;
	}
	offset = nvram_mtd->size - erasesize;
	SL2312_SHARE_PIN_ENABLE();
	ret = MTD_READ(nvram_mtd, offset, 4, &len, buf);
	SL2312_SHARE_PIN_DISABLE();

 done:
	up(&nvram_sem);
	kfree(buf);
	return ret;
}

int
nvram_getall(char *buf, int count)
{
	unsigned long flags;
	int ret;
	
	NVDBG("%s: buf=%s,count=%x\n",__func__,buf,count);
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_getall(buf, count);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}


/* User mode interface below */

static ssize_t
dev_nvram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char tmp[NVRAM_MAX_VALUE_LEN], *name = tmp, *value;
	ssize_t ret;
	unsigned long off;
		
	NVDBG("%s:\n",__func__);	
	if (count > sizeof(tmp)) {
		NVDBG("%s: size over tmp\n",__func__);
		if (!(name = kmalloc(count, GFP_KERNEL))){
			printk("%s: kmalloc fail\n",__func__);
			return -ENOMEM;
		}	
	}

	if (copy_from_user(name, buf, count)) {
		printk("%s: copy_from_user fail\n", __FUNCTION__);
		ret = -EFAULT;
		goto done;
	}

	if (*name == '\0') {
		/* Get all variables */
		NVDBG("%s: name=NULL,count=%x\n",__func__,count);
		ret = nvram_getall(name, count);
		if (ret == 0) {
			if (copy_to_user(buf, name, count)) {
				printk("%s: nvram_getall copy_from_user fail\n",__func__);
				ret = -EFAULT;
				goto done;
			}
			NVDBG("%s: name=%s,buf=%s,off=%d\n",__func__,name,buf,off);
			ret = count;
		}
	} else {
		NVDBG("%s: name=%s,count=%x\n",__func__,name,count);
		if (!(value = nvram_get(name))) {
			NVDBG("%s: nvram_get fail\n",__func__);
			ret = 0;
			goto done;
		}
		off=strlen(value);
                if (copy_to_user(buf, &value, 4)) {
			printk("%s: copy_to_user fail\n",__func__);
			ret = -EFAULT;
			goto done;
		}
		/* Provide the offset into mmap() space */
		//if (copy_to_user(buf, value, off)) {
		//	printk("%s: copy_to_user fail\n",__func__);
		//	ret = -EFAULT;
		//	goto done;
		//}
		NVDBG("%s: name=%s,buf=%s,off=%d\n",__func__,name,buf,off);
		//buf[off]= '\0';
		ret = sizeof(unsigned long);
		//ret = off;
	}

	//flush_cache_all();	
 	NVDBG("%s: name=%s,buf=%s,ret=%d\n",__func__,name,buf,ret);
done:
	if (name != tmp)
		kfree(name);
	NVDBG("%s: ok\n",__func__);
	return ret;
}

static ssize_t
dev_nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
        char tmp[100], *name = tmp, *value;
        ssize_t ret;

        if (count > sizeof(tmp)) {
                if (!(name = kmalloc(count, GFP_KERNEL)))
                        return -ENOMEM;
        }

        if (copy_from_user(name, buf, count)) {
                ret = -EFAULT;
                goto done;
        }

        value = name;
        name = strsep(&value, "=");
        if (value)
                ret = nvram_set(name, value) ? : count;
        else
                ret = nvram_unset(name) ? : count;

 done:
        if (name != tmp)
                kfree(name);

        return ret;
}
#if 0
static ssize_t
dev_nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	char tmp[NVRAM_MAX_PARAM_LEN+NVRAM_MAX_VALUE_LEN], *name = tmp, *value;
	ssize_t ret;
	
	NVDBG("%s:\n",__func__);	
	if (count > sizeof(tmp)) {
			printk("%s: size over fail\n",__func__);
			return -ENOMEM;			
	}

	if (copy_from_user(name, buf, count)) {
		printk("%s: copy_from_user fail\n",__func__);
		ret = -EFAULT;
		goto done;
	}

	value = name;
	NVDBG("%s: value=%s\n",__func__,value);
	name = strsep(&value, "=");
	NVDBG("%s: name=%s\n",__func__,name);
	if (value)
		ret = nvram_set(name, value) ? : count;
	else
		ret = nvram_unset(name) ? : count;

 done:
	if (name != tmp)
		kfree(name);
	NVDBG("%s:ok\n",__func__);
	return ret;
}	
#endif
static int
dev_nvram_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd == NVRAM_MAGIC){
		printk("%s: nvram_commit\n",__func__);
		return nvram_commit();
	}
	
	if (_IOC_TYPE(cmd) != NVRAM_IOC_MAGIC){
		printk("%s: nvram cmd fail\n",__func__);
		return -EINVAL;
	}

	if (_IOC_DIR(cmd) & _IOC_READ){
		NVDBG("%s: nvram cmd READ\n",__func__);		
		if (!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))){
			printk("%s: nvram cmd READ access fail\n",__func__);	
			return -EINVAL;
		}
	}
	switch(cmd) {
		case NVRAM_IOCGSIZE:
		NVDBG("%s: nvram cmd SIZE=0x%x\n",__func__,NVRAM_SPACE);
		return __put_user (NVRAM_SPACE, (int __user *) arg);
	}
	printk("%s: nvram fail\n",__func__);
	return -EINVAL;
}

static int
dev_nvram_mmap(struct file *file, struct vm_area_struct *vma)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	if (io_remap_pfn_range(vma, vma->vm_start, __pa(nvram_buf) >> PAGE_SHIFT,
				vma->vm_end-vma->vm_start, vma->vm_page_prot)){
		printk("%s: io_remap_pfn_range fail\n",__func__);
		return -EAGAIN;
	}
#else
	unsigned long offset = virt_to_phys(nvram_buf);
	if (remap_page_range(vma->vm_start,offset,
			vma->vm_end-vma->vm_start, vma->vm_page_prot)){
		printk("%s: remap_pfn_range fail\n",__func__);
		return -EAGAIN;
	}	  
#endif
	NVDBG("%s:nvram_buf=%x ok\n",__func__,nvram_buf);
	return 0;
}

static int
dev_nvram_open(struct inode *inode, struct file * file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#endif
	return 0;
}

static int
dev_nvram_release(struct inode *inode, struct file * file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

static struct file_operations dev_nvram_fops = {
	owner:		THIS_MODULE,
	open:		dev_nvram_open,
	release:	dev_nvram_release,
	read:		dev_nvram_read,
	write:		dev_nvram_write,
	ioctl:		dev_nvram_ioctl,
	mmap:		dev_nvram_mmap,
};

struct cdev nvram_cdev;
static char mtdname[64] = "nvram";

static unsigned long NVRAM_PTR;
static void
dev_nvram_exit(void)
{
	int order = 0;
	struct page *page, *end;
	
	if (nvram_mtd)
		put_mtd_device(nvram_mtd);

	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		clear_page_reserved(page);
	
	free_pages(NVRAM_PTR, order);
	_nvram_exit();
	cdev_del(&nvram_cdev);
	unregister_chrdev_region(MKDEV (nvram_major, 0), 1);
}
static int
dev_nvram_init(void)
{
	int order = 0, ret = 0;
	struct page *page, *end;
	unsigned int i;
	dev_t dev = MKDEV(nvram_major, 0);
//#ifndef CONFIG_MTD
//#warn	"Please make sure MTD is configured in kernel"
//#endif
	/* Find associated MTD device */
	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
		if (!nvram_mtd) {
			//printk("%s: KERNEL_VERSION < 2.6.20\n",__func__);
#else
		if (!IS_ERR(nvram_mtd)) {
#endif
			//printk("%s: KERNEL_VERSION > 2.6.20\n",__func__);
			NVDBG("%s:%d, (%s) == (%s)?, mtd->size: 0x%08llX, "
				"nvram_space: 0x%08X\n",
				__FUNCTION__, __LINE__, nvram_mtd->name,
				mtdname, nvram_mtd->size, NVRAM_SPACE);
			if (!strcmp(nvram_mtd->name, mtdname))
				break;
			put_mtd_device(nvram_mtd);
		}
	}

	if (i >= MAX_MTD_DEVICES) {
		nvram_mtd = NULL;
		printk("can not find nvram partition\n");
		return -1;
	}
	/* get mmap buffer size */
	if (NVRAM_SPACE <= 0) {		
		NVRAM_SPACE = nvram_mtd->size;
		printk("init NVRAM_SPACE from mtdblock size=0x%08x\n",NVRAM_SPACE);
	}
	if (NVRAM_SPACE > NVRAM_SPACE_MAX) {
		printk("error: NVRAM_SPACE 0x%08X > 0x%08X\n",
				NVRAM_SPACE, NVRAM_SPACE_MAX);
		return -1;
	}
		
	/* Allocate and reserve memory to mmap() */
	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	NVDBG("init nvram memory map size: 0x%08X order of pages: 0x%X\n",
			NVRAM_SPACE, order);
	
	if ((NVRAM_PTR = __get_free_pages(GFP_ATOMIC, order)) == NULL) {
		printk("no free pages\n");
		goto err;
	}
	nvram_buf = NVRAM_PTR;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		set_page_reserved(page);
	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Initialize commit semaphore */
	init_MUTEX(&nvram_sem);

	/* Register char device */
	if (nvram_major)
		ret = register_chrdev_region(dev, 1, "nvram");
	else {
		ret = alloc_chrdev_region(&dev, 0, 1, "nvram");
		nvram_major = MAJOR(dev);
	}
	if (ret < 0){
		printk("%s: register_chrdev_region fail\n",__func__);
		return -1;
	}

	/* Initialize hash table */
	_nvram_init();
	cdev_init(&nvram_cdev, &dev_nvram_fops);
	nvram_cdev.owner = THIS_MODULE;

	if ((ret = cdev_add (&nvram_cdev, dev, 1))) {
		printk("err %d, dev:%d\n", ret, dev);
		goto err;
	}
	printk("/dev/nvram major number %d glues to "
		"mtd: \"%s\" size: 0x%08X\n\tnvram_space: 0x%08X mapped via mmap(2)\n",
		MAJOR(dev), mtdname, nvram_mtd->size, NVRAM_SPACE);
	return 0;
 err:
	dev_nvram_exit();
	return ret;
}

module_init(dev_nvram_init);
module_exit(dev_nvram_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD nvram driver.");
module_param_string(mtdname, mtdname, 0444, sizeof(mtdname));
MODULE_PARM_DESC(mtdname, "mtd partition name in /proc/mtd. (default name: nvram)");
module_param(nvram_major, int, 0644);
MODULE_PARM_DESC(nvram_major, "major number in /dev/nvram (default: 225)");
module_param(NVRAM_SPACE, int, 0644);
MODULE_PARM_DESC(NVRAM_SPACE, "the mmap(2) space size of nvram driver mapped to user");
