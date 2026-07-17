#ifndef __INI_H__
#define __INI_H__

#include "gxtype.h"
#include "common/gxlog.h"

#define ASSERT(x)

int32_t Ini_Oem_Init();
int32_t Ini_Oem_Close();
char *Ini_GetValue(const char *section, const char *parameter);
int32_t Ini_SetValue(const char *section, const char *parameter, const char *value);
int32_t Ini_Sync(void);
int* Ini_Get_File_Handle();
int32_t Ini_ReadOld(char *buf, int32_t *size);
int32_t Ini_WriteOld2New(char *buf, int32_t size);
int32_t Ini_oem_use_minifs(void);
const char *Ini_oem_name(void);
const char *Ini_backoem_name(void);
#endif

