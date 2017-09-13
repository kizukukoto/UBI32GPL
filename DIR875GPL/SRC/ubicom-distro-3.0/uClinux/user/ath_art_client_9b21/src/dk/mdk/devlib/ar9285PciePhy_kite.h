/* Auto generated PCI-E PHY config for Merlin with CLKREQ always asserted in L1 mode */
/* In L1 mode leave CLKREQ asserted, power consumption will be little high.          */
static const A_UINT32 ar9285PciePhy_clkreq_always_on_L1_kite[][2] = {
    {0x00004040,  0x9248fd00 },
    {0x00004040,  0x24924924 },
    {0x00004040,  0xa8000019 },
    {0x00004040,  0x13160820 },
    {0x00004040,  0xe5980560 },
    {0x00004040,  0xc01dcffd },
    {0x00004040,  0x1aaabe41 },
    {0x00004040,  0xbe105554 },
    {0x00004040,  0x00043007 },
    {0x00004044,  0x00000000 },
};

/* Auto generated PCI-E PHY config for Merlin with CLKREQ de-asserted in L1 mode.             */
/* In L1 mode, deassert CLKREQ, power consumption will be lower than leaving CLKREQ asserted. */
static const A_UINT32 ar9285PciePhy_clkreq_off_L1_kite[][2] = {
    {0x00004040,  0x9248fd00 },
    {0x00004040,  0x24924924 },
    {0x00004040,  0xa8000019 },
    {0x00004040,  0x13160820 },
    {0x00004040,  0xe5980560 },
    {0x00004040,  0xc01dcffc },
    {0x00004040,  0x1aaabe41 },
    {0x00004040,  0xbe105554 },
    {0x00004040,  0x00043007 },
    {0x00004044,  0x00000000 },
};
