#ifndef __MDIO_H__
#define __MDIO_H__

#define mdelay(n) udelay((n)*1000)

#define phy_reg_read mdiobb_read
#define phy_reg_write mdiobb_write

int mdiobb_write(int phy_base, int phy, int reg, u16 val);
u32 mdiobb_read(int phy_base, int phy, int reg);
int mdio_reset(void);
int mdio_init(void);

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILED
#define FAILED -1
#endif

#endif
