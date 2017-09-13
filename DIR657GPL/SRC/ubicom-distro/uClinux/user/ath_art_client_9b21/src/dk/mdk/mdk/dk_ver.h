/* dk_ver.h - macros and definitions for memory management */

/*
 *  Copyright © 2000-2001 Atheros Communications, Inc.,  All Rights Reserved.
 */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/mdk/dk_ver.h#270 $, $Header: //depot/sw/src/dk/mdk/mdk/dk_ver.h#270 $"

#ifndef __INCdk_verh
#define __INCdk_verh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ART_VERSION_MAJOR 0
#define ART_VERSION_MINOR 9
#ifdef __ATH_DJGPPDOS__
#define ART_BUILD_NUM     18
#else
#define ART_BUILD_NUM     21
#endif

#define MAUIDK_VER1 ("\n   --- Atheros: MDK (multi-device version) ---\n")

#define MAUIDK_VER_SUB ("              - Revision 2.0.3 ")
#ifdef CUSTOMER_REL
#define MAUIDK_VER2  ("              - Revision 0.9 BUILD #21 ART_11n")
#ifdef ANWI
#define MAUIDK_VER3 ("\n            - Customer Version (ANWI BUILD)-\n")
#else
#define MAUIDK_VER3 ("\n            - Customer Version -\n")
#endif //ANWI
#else
#define MAUIDK_VER2 ("              - Revision 0.9 BUILD #21 ART_11n")
#define MAUIDK_VER3 ("\n      --- Atheros INTERNAL USE ONLY ---\n")
#endif
#ifdef __ATH_DJGPPDOS__
#define DEVLIB_VER1 ("Devlib Revision 0.9 BUILD #18 ART_11n\n")
#else
#define DEVLIB_VER1 ("Devlib Revision 0.9 BUILD #21 ART_11n\n")
#endif

#ifdef ART
#define MDK_CLIENT_VER1 ("\n   --- Atheros: ART Client (multi-device version) ---\n")
#else
#define MDK_CLIENT_VER1 ("\n   --- Atheros: MDK Client (multi-device version) ---\n")
#endif
#ifdef __ATH_DJGPPDOS__
#define MDK_CLIENT_VER2 ("         - Revision 0.9 BUILD #18 ART_11n -\n")
#else
#define MDK_CLIENT_VER2 ("         - Revision 0.9 BUILD #21 ART_11n -\n")
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdk_verh */
