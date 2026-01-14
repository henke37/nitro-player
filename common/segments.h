#ifndef SEGMENTS_H
#define SEGMENTS_H

#include <nds/ndstypes.h>

#ifdef __INTELLISENSE__
#undef ITCM_CODE
#undef DTCM_BSS
#undef DTCM_DATA

#undef TWL_CODE
#undef TWL_BSS
#undef TWL_DATA

#define ITCM_CODE __declspec(code_seg(".itcm.text"))
#define DTCM_BSS __declspec(allocate(".sbss"))
#define DTCM_DATA __declspec(allocate(".dtcm"))

#define TWL_CODE __declspec(code_seg(".twl.text"))
#define TWL_BSS __declspec(allocate(".twl_bss"))
#define TWL_DATA __declspec(allocate(".twl.data"))

#pragma section(".itcm.text",read,execute)
#pragma section(".dtcm",read,write)
#pragma section(".sbss",read,write)

#pragma section(".twl.text",read,execute)
#pragma section(".dtcm",read,write)
#pragma section(".twl_bss",read,write)

#endif

#endif // SEGMENTS_H