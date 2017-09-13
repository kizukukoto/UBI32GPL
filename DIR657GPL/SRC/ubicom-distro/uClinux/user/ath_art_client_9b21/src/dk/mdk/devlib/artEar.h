/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* artEar.h - Exported functions and defines for the EAR support */

#ifndef	__INCartEarh
#define	__INCartEarh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define REG_RMW(devNum, off, set, clr)    \
        (REGW(devNum, off, (REGR(devNum, off) & ~(clr)) | (set)))
#define IS_SPUR_CHAN(x)     ( (((x) % 32) != 0) && ((((x) % 32) < 10) || (((x) % 32) > 22)) )
#define IS_CM_SPUR(x)       (((x) >> 14) & 1)
#define IS_CM_SINGLE(x)     (((x) >> 15) & 1)

/* EAR definitions and structures */
#define EAR_VERSION_SUPPORT        0xEA01

#define MAX_NUM_REGISTER_HEADERS   256
#undef MAX_EAR_LOCATIONS
#define MAX_EAR_LOCATIONS          1024
//#define A_DIV_UP(x, y) (((x) + (y) - 1) / (y))
#define EAR_MAX_RH_REGS            256
//#define ATHEROS_EEPROM_END          0x400   /* Size of the EEPROM */
#define ATHEROS_EEPROM_OFFSET       0xC0    /* Atheros EEPROM defined values start at 0xC0 */

typedef struct disablerModifier {
    A_BOOL   valid;
    A_UINT16 disableField;
    A_UINT16 pllValue;
} DISABLER_MODIFIER;

typedef struct registerHeader {
    A_UINT16 versionMask;
    A_UINT16 channel;
    A_UINT16 stage;
    A_UINT16 type;
    A_UINT16 modes;
    DISABLER_MODIFIER disabler;
    A_UINT16 *regs;
} REGISTER_HEADER;

typedef struct earHeader {
    A_UINT16 versionId;
    A_UINT16 numRHs;
    int      earSize;
    REGISTER_HEADER *pRH;
} EAR_HEADER;

typedef struct headerAlloc {
    A_UINT16 numRHs;
    A_UINT16 locsPerRH[MAX_NUM_REGISTER_HEADERS];
} EAR_ALLOC;

typedef enum {
    EAR_TYPE0,
    EAR_TYPE1,
    EAR_TYPE2,
    EAR_TYPE3
} earTypesEnum;

enum earWirelessMode {
    EAR_11B   = 0x001,
    EAR_11G   = 0x002,
    EAR_2T    = 0x004,
    EAR_11A   = 0x010,
    EAR_XR    = 0x020,
    EAR_5T    = 0x040,
};

enum earStages {
    EAR_STAGE_ANALOG_WRITE,
    EAR_STAGE_PRE_PHY_ENABLE,
    EAR_STAGE_POST_RESET,
    EAR_STAGE_POST_PER_CAL
};

enum earMasksAndShifts {
    VERSION_MASK_M   = 0x7FFF,
    RH_MODES_M       = 0x01FF,
    RH_TYPE_S        = 9,
    RH_TYPE_M        = 0x0600,
    RH_STAGE_S       = 11,
    RH_STAGE_M       = 0x1800,
    CM_SINGLE_CHAN_M = 0x7FFF,
    T0_TAG_M         = 0x0003,
    T1_NUM_M         = 0x0003,
    T2_BANK_S        = 13,
    T2_BANK_M        = 0xE000,
    T2_COL_S         = 10,
    T2_COL_M         = 0x0C00,
    T2_START_M       = 0x01FF,
    T2_NUMB_S        = 12,
    T2_NUMB_M        = 0xF000,
    T2_DATA_M        = 0x0FFF,
    T3_NUMB_M        = 0x001F,
    T3_START_S       = 5,
    T3_START_M       = 0x03E0,
    T3_OPCODE_S      = 10,
    T3_OPCODE_M      = 0x1C00,
};

enum earMiscDefines {
    RH_RESERVED_MODES    = 0x0120,
    RH_ALL_MODES         = 0x01FF,
    RH_RESERVED_PLL_BITS = 0xFF00,
    T0_TAG_32BIT         = 0,
    T0_TAG_16BIT_LOW     = 1,
    T0_TAG_16BIT_HIGH    = 2,
    T0_TAG_32BIT_LAST    = 3,
    T3_MAX_OPCODE        = 6,
    T3_RESERVED_BITS     = 0x6000,
    MAX_ANALOG_START     = 319,
};

enum earType3OpCodes {
    T3_OC_REPLACE,
    T3_OC_ADD,
    T3_OC_SUB,
    T3_OC_MUL,
    T3_OC_XOR,
    T3_OC_OR,
    T3_OC_AND
};

typedef enum {
    EAR_DEBUG_OFF,
    EAR_DEBUG_BASIC,
    EAR_DEBUG_VERBOSE,
    EAR_DEBUG_EXTREME
} EAR_DEBUG_LEVELS;

typedef enum {
    EAR_LC_RF_WRITE,           /* Modifies or disables analog writes */
    EAR_LC_PHY_ENABLE,         /* Modifies any register(s) before PHY enable */
    EAR_LC_POST_RESET,         /* Modifies any register(s) at the end of Reset */
    EAR_LC_POST_PER_CAL,       /* Modifies any register(s) at the end of periodic cal */
    EAR_LC_PLL,                /* Changes the PLL value in chipReset */
    EAR_LC_RESET_OFFSET,       /* Enable/disable offset in reset */
    EAR_LC_RESET_NF,           /* Enable/disable NF in reset */
    EAR_LC_RESET_IQ,           /* Enable/disable IQ in reset */
    EAR_LC_PER_FIXED_GAIN,     /* Enable/disable fixed gain */
    EAR_LC_PER_GAIN_CIRC,      /* Enable/disable gain circulation */
    EAR_LC_PER_NF              /* Enable/disable periodic cal NF */
} EAR_LOC_CHECK;

/* Macros */
#define IS_SPUR_CHAN(x)     ( (((x) % 32) != 0) && ((((x) % 32) < 10) || (((x) % 32) > 22)) )

#define IS_VER_MASK(x)      (((x) >> 15) & 1)
#define IS_END_HEADER(x)    ((x) == 0)

#define IS_CM_SET(x)        (((x) >> 13) & 1)
#define IS_CM_SPUR(x)       (((x) >> 14) & 1)
#define IS_CM_SINGLE(x)     (((x) >> 15) & 1)
#define IS_DISABLER_SET(x)  (((x) >> 14) & 1)
#define IS_PLL_SET(x)       (((x) >> 15) & 1)
#define IS_TYPE2_EXTENDED(x) (((x) >> 9) & 1)
#define IS_TYPE2_LAST(x)    (((x) >> 12) & 1)
#define IS_TYPE3_LAST(x)    (((x) >> 15) & 1)
#define IS_5GHZ_CHAN(x)     ((((x) & CM_SINGLE_CHAN_M) > 4980) && (((x) & CM_SINGLE_CHAN_M) < 6100))
#define IS_2GHZ_CHAN(x)     ((((x) & CM_SINGLE_CHAN_M) > 2300) && (((x) & CM_SINGLE_CHAN_M) < 2700))
#define IS_VALID_ADDR(x)    ((((x) & 0x3) == 0) && \
                             (((x) <= 0x100) || ((x) >= 0x400 && (x) <= 0xc00) || \
                             ((x) >= 0x1000 && (x) <= 0x1500) || ((x) >= 0x4000 && (x) <= 0x4080) || \
                             ((x) >= 0x6000 && (x) <= 0x6020) || ((x) >= 0x8000 && (x) <= 0x9000) || \
                             ((x) >= 0x9800 && (x) <= 0x9D00) || ((x) >= 0xA000 && (x) <= 0xA400)))
#define IS_EAR_VMATCH(x, y) (((x) > EAR_VERSION_SUPPORT) && \
                             ((1 << ((x) - EAR_VERSION_SUPPORT)) & ((y) & VERSION_MASK_M)))

A_BOOL
ar5212IsEarEngaged(A_UINT32 devNum, EAR_HEADER   *pEarHead, A_UINT32 freq);

A_BOOL
ar5212EarModify(A_UINT32 devNum, EAR_HEADER   *pEarHead, EAR_LOC_CHECK loc, A_UINT32 freq, A_UINT32 *modifier);

A_BOOL
readEar
(
 A_UINT32 devNum,
 A_UINT32 earStartLocation
);

void
ar5212EarFree(EAR_HEADER *pEarHead) ;


#ifdef __cplusplus
}
#endif

#endif // #define __INCartEarh
