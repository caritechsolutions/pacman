#ifndef __SIMPLE_INI_H__
#define __SIMPLE_INI_H__

#define GXINI_DBG

#ifdef GXINI_DBG


#define INI_DBG(...)        do {\
	printf("[INIPT]\t");\
	printf( __VA_ARGS__ );\
} while (0) 


#else

#define INI_DBG(...)    

#endif

#define ASSERT(x) 

int Ini_Oem_Init(char *dlp,char *backup_dlp);
int Ini_Oem_Close(void);
char *Ini_GetValue(const char *section, const char *parameter);
int Ini_SetValue(const char *section, const char *parameter, const char *value);
int Ini_Sync(void);
int Ini_ReadOld(char *buf, int *size);
int Ini_WriteOld2New(char *buf, int size);
#endif

