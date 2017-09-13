/* pci_bus.h - contains declarations of the pci bus functions */

#ifndef __DK_PCI_BUS_H_
#define __DK_PCI_BUS_H_

#include "dk.h"

/* UBICOM_PORT */
#if defined(__LINUX_UBICOM32_ARCH__)
static inline u_int32_t __bswap32(u_int32_t _x)
{
    return ((u_int32_t)(
          (((const u_int8_t *)(&_x))[0]    ) |
          (((const u_int8_t *)(&_x))[1]<< 8) |
          (((const u_int8_t *)(&_x))[2]<<16) |
          (((const u_int8_t *)(&_x))[3]<<24))
    );
}
#define __bswap16(_x) ( (u_int16_t)( (((const u_int8_t *)(&_x))[0] ) | ( ( (const u_int8_t *)( &_x ) )[1]<< 8) ) )

extern unsigned int ubi32_pci_read_u32(const volatile void *addr);
extern  void ubi32_pci_write_u32(unsigned int val, const volatile void *addr);
#define _ATH_REG_WRITE(_addr, _val) do {             \
        ubi32_pci_write_u32(_val, ((void *)(_addr))); \
} while(0)
#define _ATH_REG_READ(_addr) \
        ubi32_pci_read_u32((void *)(_addr))
#define OS_FLUSH_CACHE_SINGLE(addr)       { \
    asm volatile ("flush (%0)\n" "pipe_flush 0\n" "sync (%0)\n" "flush (%0)\n" : : "a"(addr) : "memory"); }
#else /* __LINUX_UBICOM32_ARCH__ */
#define _ATH_REG_WRITE(_addr, _val)  writel(_val,_addr)
#define _ATH_REG_READ(_addr)         readl(_addr)
#define OS_FLUSH_CACHE_SINGLE(addr)
#endif /* __LINUX_UBICOM32_ARCH__ */

#if defined(OWL_PB42) || defined(PYTHON_EMU)
INT32 bus_module_init
(
	VOID
);

VOID bus_module_exit
(
	VOID
);

INT32 bus_dev_init
(
     void *bus_dev
);


INT32 bus_dev_exit
(
     void  *bus_dev
);

INT32 bus_cfg_read
(
     void *bus_dev,
     INT32 offset,
     INT32 size,
     INT32 *ret_val
);

INT32 bus_cfg_write
(
    void *bus_dev,
    INT32 offset,
	INT32 size,
	INT32 ret_val
);

#endif
		
#endif //__PCI_BUS_H_
