#ifdef __INTELLISENSE__
#undef ITCM_CODE
#undef DTCM_BSS
#undef DTCM_DATA
#define ITCM_CODE __declspec(code_seg(".itcm.text"))
#define DTCM_BSS __declspec(allocate(".sbss"))
#define DTCM_DATA __declspec(allocate(".dtcm"))

#pragma section(".itcm.text",read,execute)
#pragma section(".dtcm",read,write)
#pragma section(".sbss",read,write)

#pragma code_seg(".itcm.text")
#pragma data_seg(".dtcm")
#pragma bss_seg(".sbss")
#endif