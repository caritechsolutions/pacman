#ifndef APP_MAINMENU_SYS_DLL_H_20250116
#define APP_MAINMENU_SYS_DLL_H_20250116

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <locale.h>
#include <setjmp.h>
#include <signal.h>
#include <math.h>
//#include <tree.h>
//#include <parser.h>
//#include <curl/curl.h>
//#include <cJSON.h>
//#include <gx_apps.h>
#include <gxos/gxcore_os_core.h>
#include "gui_key.h"
#include "gui_core.h"
#include "gui_timer.h"

#include "netapps/app_netvideo_play.h"

//gcc functions
//4.1 Routines for integer arithmetic
//4.1.1 Arithmetic functions
long __ashldi3 (long a, int b);
long __ashrdi3 (long a, int b);
int __divsi3 (int a, int b);
long __divdi3 (long a, long b);
long __lshrdi3 (long a, int b);
int __modsi3 (int a, int b);
long __moddi3 (long a, long b);
long __muldi3 (long a, long b);
long __negdi2 (long a);
unsigned int __udivsi3 (unsigned int a, unsigned int b);
unsigned long __udivdi3 (unsigned long a, unsigned long b);
unsigned long __udivmoddi4 (unsigned long a, unsigned long b, unsigned long *c);
unsigned int __umodsi3 (unsigned int a, unsigned int b);
unsigned long __umoddi3 (unsigned long a, unsigned long b);
//4.1.2 Comparison functions
int __cmpdi2 (long a, long b);
int __ucmpdi2 (unsigned long a, unsigned long b);
//4.1.3 Trapping arithmetic functions
int __absvsi2 (int a);
long __absvdi2 (long a);
int __addvsi3 (int a, int b);
long __addvdi3 (long a, long b);
int __mulvsi3 (int a, int b);
long __mulvdi3 (long a, long b);
int __negvsi2 (int a);
long __negvdi2 (long a);
int __subvsi3 (int a, int b);
long __subvdi3 (long a, long b);
//4.1.4 Bit operations
int __clzsi2 (unsigned int a);
int __clzdi2 (unsigned long a);
int __ctzsi2 (unsigned int a);
int __ctzdi2 (unsigned long a);
int __ffsdi2 (unsigned long a);
int __paritysi2 (unsigned int a);
int __paritydi2 (unsigned long a);
int __popcountsi2 (unsigned int a);
int __popcountdi2 (unsigned long a);
int32_t __bswapsi2 (int32_t a);
int64_t __bswapdi2 (int64_t a);
//4.2 Routines for floating point emulation
//4.2.1 Arithmetic functions
float __addsf3 (float a, float b);
float __subsf3 (float a, float b);
double __adddf3 (double a, double b);
double __subdf3 (double a, double b);
float __mulsf3 (float a, float b);
double __muldf3 (double a, double b);
float __divsf3 (float a, float b);
double __divdf3 (double a, double b);
float __negsf2 (float a);
double __negdf2 (double a);
//4.2.2 Conversion functions
double __extendsfdf2 (float a);
float __truncdfsf2 (double a);
int __fixsfsi (float a);
int __fixdfsi (double a);
long __fixsfdi (float a);
long __fixdfdi (double a);
unsigned int __fixunssfsi (float a);
unsigned int __fixunsdfsi (double a);
unsigned long __fixunssfdi (float a);
unsigned long __fixunsdfdi (double a);
float __floatsisf (int i);
double __floatsidf (int i);
float __floatdisf (long i);
double __floatdidf (long i);
float __floatunsisf (unsigned int i);
double __floatunsidf (unsigned int i);
float __floatundisf (unsigned long i);
double __floatundidf (unsigned long i);
//4.2.3 Comparison functions
int __cmpsf2 (float a, float b);
int __cmpdf2 (double a, double b);
int __unordsf2 (float a, float b);
int __unorddf2 (double a, double b);
int __eqsf2 (float a, float b);
int __eqdf2 (double a, double b);
int __nesf2 (float a, float b);
int __nedf2 (double a, double b);
int __gesf2 (float a, float b);
int __gedf2 (double a, double b);
int __ltsf2 (float a, float b);
int __ltdf2 (double a, double b);
int __lesf2 (float a, float b);
int __ledf2 (double a, double b);
int __gtsf2 (float a, float b);
int __gtdf2 (double a, double b);
//4.2.4 Other floating-point functions
float __powisf2 (float a, int b);
double __powidf2 (double a, int b);

//appcore
//gx_apps.h
//OS
void gxapps_mutex_lock(void);
void gxapps_mutex_unlock(void);
//stb config functions
char *gxapps_get_chip_sn_str(void);
char *gxapps_get_chip_name_str(void);
char *gxapps_get_customer_name_str(void);
char *gxapps_get_customer_model_str(void);
char *gxapps_get_os_str(void);
char *gxapps_get_platform_version_str(void);
char *gxapps_get_app_version_str(void);
char *gxapps_get_ssl_version_str(void);
char *gxapps_get_curl_version_str(void);
char *gxapps_get_appcore_version_str(void);
char *gxapps_get_country_str(void);
char *gxapps_get_region_str(void);
char *gxapps_get_city_str(void);
//cloud functions
int gxapps_check_authorization(char *name);
char *gxapps_get_config(char *name);
unsigned int gxapps_get_server_time(void);
unsigned int gxapps_get_check_value(char *str);
void gxapps_get_server_url(char *server_url, char *url);
void gxapps_get_server_post_url(char *server_url);
char *gxapps_get_server_user_agent(void);



//gcc functions
//4.1 Routines for integer arithmetic
//4.1.1 Arithmetic functions
GX_LDR_TABLE_ENTRY(__ashldi3_entry, "__ashldi3", &__ashldi3);
GX_LDR_TABLE_ENTRY(__ashrdi3_entry, "__ashrdi3", &__ashrdi3);
GX_LDR_TABLE_ENTRY(__divsi3_entry, "__divsi3", &__divsi3);
GX_LDR_TABLE_ENTRY(__divdi3_entry, "__divdi3", &__divdi3);
GX_LDR_TABLE_ENTRY(__lshrdi3_entry, "__lshrdi3", &__lshrdi3);
GX_LDR_TABLE_ENTRY(__modsi3_entry, "__modsi3", &__modsi3);
GX_LDR_TABLE_ENTRY(__moddi3_entry, "__moddi3", &__moddi3);
GX_LDR_TABLE_ENTRY(__muldi3_entry, "__muldi3", &__muldi3);
GX_LDR_TABLE_ENTRY(__negdi2_entry, "__negdi2", &__negdi2);
GX_LDR_TABLE_ENTRY(__udivsi3_entry, "__udivsi3", &__udivsi3);
GX_LDR_TABLE_ENTRY(__udivdi3_entry, "__udivdi3", &__udivdi3);
GX_LDR_TABLE_ENTRY(__udivmoddi4_entry, "__udivmoddi4", &__udivmoddi4);
GX_LDR_TABLE_ENTRY(__umodsi3_entry, "__umodsi3", &__umodsi3);
GX_LDR_TABLE_ENTRY(__umoddi3_entry, "__umoddi3", &__umoddi3);
//4.1.2 Comparison functions
GX_LDR_TABLE_ENTRY(__cmpdi2_entry, "__cmpdi2", &__cmpdi2);
GX_LDR_TABLE_ENTRY(__ucmpdi2_entry, "__ucmpdi2", &__ucmpdi2);
//4.1.3 Trapping arithmetic functions
GX_LDR_TABLE_ENTRY(__absvsi2_entry, "__absvsi2", &__absvsi2);
GX_LDR_TABLE_ENTRY(__absvdi2_entry, "__absvdi2", &__absvdi2);
GX_LDR_TABLE_ENTRY(__addvsi3_entry, "__addvsi3", &__addvsi3);
GX_LDR_TABLE_ENTRY(__addvdi3_entry, "__addvdi3", &__addvdi3);
GX_LDR_TABLE_ENTRY(__mulvsi3_entry, "__mulvsi3", &__mulvsi3);
GX_LDR_TABLE_ENTRY(__mulvdi3_entry, "__mulvdi3", &__mulvdi3);
GX_LDR_TABLE_ENTRY(__negvsi2_entry, "__negvsi2", &__negvsi2);
GX_LDR_TABLE_ENTRY(__negvdi2_entry, "__negvdi2", &__negvdi2);
GX_LDR_TABLE_ENTRY(__subvsi3_entry, "__subvsi3", &__subvsi3);
GX_LDR_TABLE_ENTRY(__subvdi3_entry, "__subvdi3", &__subvdi3);
//4.1.4 Bit operations
GX_LDR_TABLE_ENTRY(__clzsi2_entry, "__clzsi2", &__clzsi2);
GX_LDR_TABLE_ENTRY(__clzdi2_entry, "__clzdi2", &__clzdi2);
GX_LDR_TABLE_ENTRY(__ctzsi2_entry, "__ctzsi2", &__ctzsi2);
GX_LDR_TABLE_ENTRY(__ctzdi2_entry, "__ctzdi2", &__ctzdi2);
GX_LDR_TABLE_ENTRY(__ffsdi2_entry, "__ffsdi2", &__ffsdi2);
GX_LDR_TABLE_ENTRY(__paritysi2_entry, "__paritysi2", &__paritysi2);
GX_LDR_TABLE_ENTRY(__paritydi2_entry, "__paritydi2", &__paritydi2);
GX_LDR_TABLE_ENTRY(__popcountsi2_entry, "__popcountsi2", &__popcountsi2);
GX_LDR_TABLE_ENTRY(__popcountdi2_entry, "__popcountdi2", &__popcountdi2);
GX_LDR_TABLE_ENTRY(__bswapsi2_entry, "__bswapsi2", &__bswapsi2);
GX_LDR_TABLE_ENTRY(__bswapdi2_entry, "__bswapdi2", &__bswapdi2);
//4.2 Routines for floating point emulation
//4.2.1 Arithmetic functions
GX_LDR_TABLE_ENTRY(__addsf3_entry, "__addsf3", &__addsf3);
GX_LDR_TABLE_ENTRY(__subsf3_entry, "__subsf3", &__subsf3);
GX_LDR_TABLE_ENTRY(__adddf3_entry, "__adddf3", &__adddf3);
GX_LDR_TABLE_ENTRY(__subdf3_entry, "__subdf3", &__subdf3);
GX_LDR_TABLE_ENTRY(__mulsf3_entry, "__mulsf3", &__mulsf3);
GX_LDR_TABLE_ENTRY(__muldf3_entry, "__muldf3", &__muldf3);
GX_LDR_TABLE_ENTRY(__divsf3_entry, "__divsf3", &__divsf3);
GX_LDR_TABLE_ENTRY(__divdf3_entry, "__divdf3", &__divdf3);
GX_LDR_TABLE_ENTRY(__negsf2_entry, "__negsf2", &__negsf2);
GX_LDR_TABLE_ENTRY(__negdf2_entry, "__negdf2", &__negdf2);
//4.2.2 Conversion functions
GX_LDR_TABLE_ENTRY(__extendsfdf2_entry, "__extendsfdf2", &__extendsfdf2);
GX_LDR_TABLE_ENTRY(__truncdfsf2_entry, "__truncdfsf2", &__truncdfsf2);
GX_LDR_TABLE_ENTRY(__fixsfsi_entry, "__fixsfsi", &__fixsfsi);
GX_LDR_TABLE_ENTRY(__fixdfsi_entry, "__fixdfsi", &__fixdfsi);
GX_LDR_TABLE_ENTRY(__fixsfdi_entry, "__fixsfdi", &__fixsfdi);
GX_LDR_TABLE_ENTRY(__fixdfdi_entry, "__fixdfdi", &__fixdfdi);
GX_LDR_TABLE_ENTRY(__fixunssfsi_entry, "__fixunssfsi", &__fixunssfsi);
GX_LDR_TABLE_ENTRY(__fixunsdfsi_entry, "__fixunsdfsi", &__fixunsdfsi);
GX_LDR_TABLE_ENTRY(__fixunssfdi_entry, "__fixunssfdi", &__fixunssfdi);
GX_LDR_TABLE_ENTRY(__fixunsdfdi_entry, "__fixunsdfdi", &__fixunsdfdi);
GX_LDR_TABLE_ENTRY(__floatsisf_entry, "__floatsisf", &__floatsisf);
GX_LDR_TABLE_ENTRY(__floatsidf_entry, "__floatsidf", &__floatsidf);
GX_LDR_TABLE_ENTRY(__floatdisf_entry, "__floatdisf", &__floatdisf);
GX_LDR_TABLE_ENTRY(__floatdidf_entry, "__floatdidf", &__floatdidf);
GX_LDR_TABLE_ENTRY(__floatunsisf_entry, "__floatunsisf", &__floatunsisf);
GX_LDR_TABLE_ENTRY(__floatunsidf_entry, "__floatunsidf", &__floatunsidf);
GX_LDR_TABLE_ENTRY(__floatundisf_entry, "__floatundisf", &__floatundisf);
GX_LDR_TABLE_ENTRY(__floatundidf_entry, "__floatundidf", &__floatundidf);
//4.2.3 Comparison functions
GX_LDR_TABLE_ENTRY(__cmpsf2_entry, "__cmpsf2", &__cmpsf2);
GX_LDR_TABLE_ENTRY(__cmpdf2_entry, "__cmpdf2", &__cmpdf2);
GX_LDR_TABLE_ENTRY(__unordsf2_entry, "__unordsf2", &__unordsf2);
GX_LDR_TABLE_ENTRY(__unorddf2_entry, "__unorddf2", &__unorddf2);
GX_LDR_TABLE_ENTRY(__eqsf2_entry, "__eqsf2", &__eqsf2);
GX_LDR_TABLE_ENTRY(__eqdf2_entry, "__eqdf2", &__eqdf2);
GX_LDR_TABLE_ENTRY(__nesf2_entry, "__nesf2", &__nesf2);
GX_LDR_TABLE_ENTRY(__nedf2_entry, "__nedf2", &__nedf2);
GX_LDR_TABLE_ENTRY(__gesf2_entry, "__gesf2", &__gesf2);
GX_LDR_TABLE_ENTRY(__gedf2_entry, "__gedf2", &__gedf2);
GX_LDR_TABLE_ENTRY(__ltsf2_entry, "__ltsf2", &__ltsf2);
GX_LDR_TABLE_ENTRY(__ltdf2_entry, "__ltdf2", &__ltdf2);
GX_LDR_TABLE_ENTRY(__lesf2_entry, "__lesf2", &__lesf2);
GX_LDR_TABLE_ENTRY(__ledf2_entry, "__ledf2", &__ledf2);
GX_LDR_TABLE_ENTRY(__gtsf2_entry, "__gtsf2", &__gtsf2);
GX_LDR_TABLE_ENTRY(__gtdf2_entry, "__gtdf2", &__gtdf2);
//4.2.4 Other floating-point functions
GX_LDR_TABLE_ENTRY(__powisf2_entry, "__powisf2", &__powisf2);
GX_LDR_TABLE_ENTRY(__powidf2_entry, "__powidf2", &__powidf2);



//libc
//stdio.h
//ISO C89 7.9.4 Functions for operations on files
GX_LDR_TABLE_ENTRY(remove_entry, "remove", &remove);
GX_LDR_TABLE_ENTRY(rename_entry, "rename", &rename);
GX_LDR_TABLE_ENTRY(tmpfile_entry, "tmpfile", &tmpfile);
GX_LDR_TABLE_ENTRY(tmpnam_entry, "tmpnam", &tmpnam);
//ISO C89 7.9.5 File access functions
GX_LDR_TABLE_ENTRY(fopen_entry, "fopen", &fopen);
GX_LDR_TABLE_ENTRY(fileno_entry, "fileno", &fileno);
GX_LDR_TABLE_ENTRY(freopen_entry, "freopen", &freopen);
GX_LDR_TABLE_ENTRY(fflush_entry, "fflush", &fflush);
GX_LDR_TABLE_ENTRY(fclose_entry, "fclose", &fclose);
GX_LDR_TABLE_ENTRY(setbuf_entry, "setbuf", &setbuf);
GX_LDR_TABLE_ENTRY(setvbuf_entry, "setvbuf", &setvbuf);
//ISO C89 7.9.6 Formatted input/output functions
GX_LDR_TABLE_ENTRY(printf_entry, "printf", &printf);
GX_LDR_TABLE_ENTRY(fprintf_entry, "fprintf", &fprintf);
GX_LDR_TABLE_ENTRY(sprintf_entry, "sprintf", &sprintf);
GX_LDR_TABLE_ENTRY(scanf_entry, "scanf", &scanf);
GX_LDR_TABLE_ENTRY(fscanf_entry, "fscanf", &fscanf);
GX_LDR_TABLE_ENTRY(sscanf_entry, "sscanf", &sscanf);
GX_LDR_TABLE_ENTRY(vfprintf_entry, "vfprintf", &vfprintf);
GX_LDR_TABLE_ENTRY(vprintf_entry, "vprintf", &vprintf);
GX_LDR_TABLE_ENTRY(vsprintf_entry, "vsprintf", &vsprintf);
//ISO C89 7.9.7 Character input/output functions
GX_LDR_TABLE_ENTRY(fgetc_entry, "fgetc", &fgetc);
GX_LDR_TABLE_ENTRY(fgets_entry, "fgets", &fgets);
GX_LDR_TABLE_ENTRY(fputc_entry, "fputc", &fputc);
GX_LDR_TABLE_ENTRY(putc_entry, "putc", &putc);
GX_LDR_TABLE_ENTRY(putchar_entry, "putchar", &putchar);
GX_LDR_TABLE_ENTRY(fputs_entry, "fputs", &fputs);
GX_LDR_TABLE_ENTRY(gets_entry, "gets", &gets);
GX_LDR_TABLE_ENTRY(getc_entry, "getc", &getc);
GX_LDR_TABLE_ENTRY(getchar_entry, "getchar", &getchar);
GX_LDR_TABLE_ENTRY(puts_entry, "puts", &puts);
GX_LDR_TABLE_ENTRY(ungetc_entry, "ungetc", &ungetc);
//ISO C89 7.9.8 Direct input/output functions
GX_LDR_TABLE_ENTRY(fread_entry, "fread", &fread);
GX_LDR_TABLE_ENTRY(fwrite_entry, "fwrite", &fwrite);
//ISO C89 7.9.9 File positioning functions
GX_LDR_TABLE_ENTRY(fgetpos_entry, "fgetpos", &fgetpos);
GX_LDR_TABLE_ENTRY(fsetpos_entry, "fsetpos", &fsetpos);
GX_LDR_TABLE_ENTRY(fseek_entry, "fseek", &fseek);
GX_LDR_TABLE_ENTRY(fseeko_entry, "fseeko", &fseeko);
GX_LDR_TABLE_ENTRY(ftell_entry, "ftell", &ftell);
GX_LDR_TABLE_ENTRY(ftello_entry, "ftello", &ftello);
GX_LDR_TABLE_ENTRY(rewind_entry, "rewind", &rewind);
//ISO C89 7.9.10 Error-handling functions
GX_LDR_TABLE_ENTRY(clearerr_entry, "clearerr", &clearerr);
GX_LDR_TABLE_ENTRY(feof_entry, "feof", &feof);
GX_LDR_TABLE_ENTRY(ferror_entry, "ferror", &ferror);
GX_LDR_TABLE_ENTRY(perror_entry, "perror", &perror);
//Other non-ISO C functions
GX_LDR_TABLE_ENTRY(fnprintf_entry, "fnprintf", &fnprintf);
GX_LDR_TABLE_ENTRY(snprintf_entry, "snprintf", &snprintf);
GX_LDR_TABLE_ENTRY(vfnprintf_entry, "vfnprintf", &vfnprintf);
GX_LDR_TABLE_ENTRY(vsnprintf_entry, "vsnprintf", &vsnprintf);
GX_LDR_TABLE_ENTRY(vscanf_entry, "vscanf", &vscanf);
GX_LDR_TABLE_ENTRY(vsscanf_entry, "vsscanf", &vsscanf);
GX_LDR_TABLE_ENTRY(vfscanf_entry, "vfscanf", &vfscanf);

//unistd.h
GX_LDR_TABLE_ENTRY(unlink_entry, "unlink", &unlink);
GX_LDR_TABLE_ENTRY(rmdir_entry, "rmdir", &rmdir);
GX_LDR_TABLE_ENTRY(access_entry, "access", &access);
GX_LDR_TABLE_ENTRY(fsync_entry, "fsync", &fsync);

//stdlib.h
//string functions
GX_LDR_TABLE_ENTRY(atof_entry, "atof", &atof);
GX_LDR_TABLE_ENTRY(atoi_entry, "atoi", &atoi);
GX_LDR_TABLE_ENTRY(atol_entry, "atol", &atol);
GX_LDR_TABLE_ENTRY(atoll_entry, "atoll", &atoll);
GX_LDR_TABLE_ENTRY(strtod_entry, "strtod", &strtod);
GX_LDR_TABLE_ENTRY(strtof_entry, "strtof", &strtof);
GX_LDR_TABLE_ENTRY(strtol_entry, "strtol", &strtol);
GX_LDR_TABLE_ENTRY(strtold_entry, "strtold", &strtold);
GX_LDR_TABLE_ENTRY(strtoll_entry, "strtoll", &strtoll);
GX_LDR_TABLE_ENTRY(strtoul_entry, "strtoul", &strtoul);
GX_LDR_TABLE_ENTRY(strtoull_entry, "strtoull", &strtoull);
//memory functions
GX_LDR_TABLE_ENTRY(malloc_entry, "malloc", &malloc);
GX_LDR_TABLE_ENTRY(free_entry, "free", &free);
GX_LDR_TABLE_ENTRY(calloc_entry, "calloc", &calloc);
GX_LDR_TABLE_ENTRY(realloc_entry, "realloc", &realloc);
//Environment functions
GX_LDR_TABLE_ENTRY(_exit_entry, "_exit", &_exit);
GX_LDR_TABLE_ENTRY(exit_entry, "exit", &exit);
GX_LDR_TABLE_ENTRY(atexit_entry, "atexit", &atexit);
GX_LDR_TABLE_ENTRY(abort_entry, "abort", &abort);
GX_LDR_TABLE_ENTRY(getenv_entry, "getenv", &getenv);
GX_LDR_TABLE_ENTRY(system_entry, "system", &system);
//search and sort
GX_LDR_TABLE_ENTRY(bsearch_entry, "bsearch", &bsearch);
GX_LDR_TABLE_ENTRY(qsort_entry, "qsort", &qsort);
//math
GX_LDR_TABLE_ENTRY(abs_entry, "abs", &abs);
GX_LDR_TABLE_ENTRY(div_entry, "div", &div);
GX_LDR_TABLE_ENTRY(labs_entry, "labs", &labs);
GX_LDR_TABLE_ENTRY(ldiv_entry, "ldiv", &ldiv);
GX_LDR_TABLE_ENTRY(rand_entry, "rand", &rand);
GX_LDR_TABLE_ENTRY(srand_entry, "srand", &srand);
GX_LDR_TABLE_ENTRY(rand_r_entry, "rand_r", &rand_r);

//string.h
//Copying functions
GX_LDR_TABLE_ENTRY(memcpy_entry, "memcpy", &memcpy);
GX_LDR_TABLE_ENTRY(memmove_entry, "memmove", &memmove);
GX_LDR_TABLE_ENTRY(strcpy_entry, "strcpy", &strcpy);
GX_LDR_TABLE_ENTRY(strncpy_entry, "strncpy", &strncpy);
//Concatenation functions
GX_LDR_TABLE_ENTRY(strcat_entry, "strcat", &strcat);
GX_LDR_TABLE_ENTRY(strncat_entry, "strncat", &strncat);
//Comparison functions
GX_LDR_TABLE_ENTRY(memcmp_entry, "memcmp", &memcmp);
GX_LDR_TABLE_ENTRY(strcmp_entry, "strcmp", &strcmp);
GX_LDR_TABLE_ENTRY(strcoll_entry, "strcoll", &strcoll);
GX_LDR_TABLE_ENTRY(strncmp_entry, "strncmp", &strncmp);
GX_LDR_TABLE_ENTRY(strxfrm_entry, "strxfrm", &strxfrm);
//Search functions
GX_LDR_TABLE_ENTRY(memchr_entry, "memchr", &memchr);
GX_LDR_TABLE_ENTRY(strchr_entry, "strchr", &strchr);
GX_LDR_TABLE_ENTRY(strcspn_entry, "strcspn", &strcspn);
GX_LDR_TABLE_ENTRY(strpbrk_entry, "strpbrk", &strpbrk);
GX_LDR_TABLE_ENTRY(strrchr_entry, "strrchr", &strrchr);
GX_LDR_TABLE_ENTRY(strspn_entry, "strspn", &strspn);
GX_LDR_TABLE_ENTRY(strstr_entry, "strstr", &strstr);
GX_LDR_TABLE_ENTRY(strcasestr_entry, "strcasestr", &strcasestr);
GX_LDR_TABLE_ENTRY(strtok_entry, "strtok", &strtok);
GX_LDR_TABLE_ENTRY(strtok_r_entry, "strtok_r", &strtok_r);
//Miscellaneous functions
GX_LDR_TABLE_ENTRY(memset_entry, "memset", &memset);
GX_LDR_TABLE_ENTRY(strerror_entry, "strerror", &strerror);
GX_LDR_TABLE_ENTRY(strlen_entry, "strlen", &strlen);
GX_LDR_TABLE_ENTRY(strnlen_entry, "strnlen", &strnlen);
GX_LDR_TABLE_ENTRY(strdup_entry, "strdup", &strdup);
//Ecos BSD String functions
GX_LDR_TABLE_ENTRY(strcasecmp_entry, "strcasecmp", &strcasecmp);
GX_LDR_TABLE_ENTRY(strncasecmp_entry, "strncasecmp", &strncasecmp);
GX_LDR_TABLE_ENTRY(bcmp_entry, "bcmp", &bcmp);
GX_LDR_TABLE_ENTRY(bcopy_entry, "bcopy", &bcopy);
GX_LDR_TABLE_ENTRY(bzero_entry, "bzero", &bzero);
GX_LDR_TABLE_ENTRY(index_entry, "index", &index);
GX_LDR_TABLE_ENTRY(rindex_entry, "rindex", &rindex);
GX_LDR_TABLE_ENTRY(swab_entry, "swab", &swab);

//time.h
GX_LDR_TABLE_ENTRY(time_entry, "time", &time);
GX_LDR_TABLE_ENTRY(difftime_entry, "difftime", &difftime);
GX_LDR_TABLE_ENTRY(mktime_entry, "mktime", &mktime);
GX_LDR_TABLE_ENTRY(asctime_entry, "asctime", &asctime);
GX_LDR_TABLE_ENTRY(asctime_r_entry, "asctime_r", &asctime_r);
GX_LDR_TABLE_ENTRY(ctime_entry, "ctime", &ctime);
GX_LDR_TABLE_ENTRY(ctime_r_entry, "ctime_r", &ctime_r);
GX_LDR_TABLE_ENTRY(gmtime_entry, "gmtime", &gmtime);
GX_LDR_TABLE_ENTRY(gmtime_r_entry, "gmtime_r", &gmtime_r);
GX_LDR_TABLE_ENTRY(strftime_entry, "strftime", &strftime);
GX_LDR_TABLE_ENTRY(strptime_entry, "strptime", &strptime);
GX_LDR_TABLE_ENTRY(localtime_entry, "localtime", &localtime);
GX_LDR_TABLE_ENTRY(localtime_r_entry, "localtime_r", &localtime_r);
GX_LDR_TABLE_ENTRY(clock_entry, "clock", &clock);

//system/time.h
GX_LDR_TABLE_ENTRY(gettimeofday_entry, "gettimeofday", &gettimeofday);

//ctype.h
GX_LDR_TABLE_ENTRY(isalnum_entry, "isalnum", &isalnum);
GX_LDR_TABLE_ENTRY(isalpha_entry, "isalpha", &isalpha);
GX_LDR_TABLE_ENTRY(iscntrl_entry, "iscntrl", &iscntrl);
GX_LDR_TABLE_ENTRY(isdigit_entry, "isdigit", &isdigit);
GX_LDR_TABLE_ENTRY(isgraph_entry, "isgraph", &isgraph);
GX_LDR_TABLE_ENTRY(islower_entry, "islower", &islower);
GX_LDR_TABLE_ENTRY(isprint_entry, "isprint", &isprint);
GX_LDR_TABLE_ENTRY(ispunct_entry, "ispunct", &ispunct);
GX_LDR_TABLE_ENTRY(isspace_entry, "isspace", &isspace);
GX_LDR_TABLE_ENTRY(isupper_entry, "isupper", &isupper);
GX_LDR_TABLE_ENTRY(isxdigit_entry, "isxdigit", &isxdigit);
GX_LDR_TABLE_ENTRY(tolower_entry, "tolower", &tolower);
GX_LDR_TABLE_ENTRY(toupper_entry, "toupper", &toupper);

//locale.h
GX_LDR_TABLE_ENTRY(setlocale_entry, "setlocale", &setlocale);
GX_LDR_TABLE_ENTRY(localeconv_entry, "localeconv", &localeconv);

//setjmp.h
GX_LDR_TABLE_ENTRY(longjmp_entry, "longjmp", &longjmp);

//signal.h
GX_LDR_TABLE_ENTRY(signal_entry, "signal", &signal);
GX_LDR_TABLE_ENTRY(raise_entry, "raise", &raise);


//libm
//math.h
GX_LDR_TABLE_ENTRY(pow_entry, "pow", &pow);
GX_LDR_TABLE_ENTRY(log_entry, "log", &log);
GX_LDR_TABLE_ENTRY(log10_entry, "log10", &log10);
GX_LDR_TABLE_ENTRY(copysign_entry, "copysign", &copysign);
GX_LDR_TABLE_ENTRY(scalbn_entry, "scalbn", &scalbn);
GX_LDR_TABLE_ENTRY(sqrt_entry, "sqrt", &sqrt);
GX_LDR_TABLE_ENTRY(fabs_entry, "fabs", &fabs);
GX_LDR_TABLE_ENTRY(asin_entry, "asin", &asin);
GX_LDR_TABLE_ENTRY(sin_entry, "sin", &sin);
GX_LDR_TABLE_ENTRY(cos_entry, "cos", &cos);
GX_LDR_TABLE_ENTRY(acos_entry, "acos", &acos);
GX_LDR_TABLE_ENTRY(tan_entry, "tan", &tan);
GX_LDR_TABLE_ENTRY(cosh_entry, "cosh", &cosh);
GX_LDR_TABLE_ENTRY(sinh_entry, "sinh", &sinh);
GX_LDR_TABLE_ENTRY(tanh_entry, "tanh", &tanh);
GX_LDR_TABLE_ENTRY(atan_entry, "atan", &atan);
GX_LDR_TABLE_ENTRY(ceil_entry, "ceil", &ceil);
GX_LDR_TABLE_ENTRY(floor_entry, "floor", &floor);
GX_LDR_TABLE_ENTRY(fmod_entry, "fmod", &fmod);
GX_LDR_TABLE_ENTRY(frexp_entry, "frexp", &frexp);
GX_LDR_TABLE_ENTRY(ldexp_entry, "ldexp", &ldexp);
GX_LDR_TABLE_ENTRY(modf_entry, "modf", &modf);
GX_LDR_TABLE_ENTRY(atan2_entry, "atan2", &atan2);
GX_LDR_TABLE_ENTRY(exp_entry, "exp", &exp);
GX_LDR_TABLE_ENTRY(expm1_entry, "expm1", &expm1);

#if 0
//libxml
//tree.h
//Creating/freeing new structures
GX_LDR_TABLE_ENTRY(GXxmlNewDtd_entry, "GXxmlNewDtd", &GXxmlNewDtd);
GX_LDR_TABLE_ENTRY(GXxmlFreeDtd_entry, "GXxmlFreeDtd", &GXxmlFreeDtd);
GX_LDR_TABLE_ENTRY(GXxmlNewGlobalNs_entry, "GXxmlNewGlobalNs", &GXxmlNewGlobalNs);
GX_LDR_TABLE_ENTRY(GXxmlNewNs_entry, "GXxmlNewNs", &GXxmlNewNs);
GX_LDR_TABLE_ENTRY(GXxmlFreeNs_entry, "GXxmlFreeNs", &GXxmlFreeNs);
GX_LDR_TABLE_ENTRY(GXxmlNewDoc_entry, "GXxmlNewDoc", &GXxmlNewDoc);
GX_LDR_TABLE_ENTRY(GXxmlFreeDoc_entry, "GXxmlFreeDoc", &GXxmlFreeDoc);
GX_LDR_TABLE_ENTRY(GXxmlNewDocProp_entry, "GXxmlNewDocProp", &GXxmlNewDocProp);
GX_LDR_TABLE_ENTRY(GXxmlNewProp_entry, "GXxmlNewProp", &GXxmlNewProp);
GX_LDR_TABLE_ENTRY(GXxmlFreePropList_entry, "GXxmlFreePropList", &GXxmlFreePropList);
GX_LDR_TABLE_ENTRY(GXxmlFreeProp_entry, "GXxmlFreeProp", &GXxmlFreeProp);
GX_LDR_TABLE_ENTRY(GXxmlCopyProp_entry, "GXxmlCopyProp", &GXxmlCopyProp);
GX_LDR_TABLE_ENTRY(GXxmlCopyPropList_entry, "GXxmlCopyPropList", &GXxmlCopyPropList);
GX_LDR_TABLE_ENTRY(GXxmlCopyDtd_entry, "GXxmlCopyDtd", &GXxmlCopyDtd);
GX_LDR_TABLE_ENTRY(GXxmlCopyDoc_entry, "GXxmlCopyDoc", &GXxmlCopyDoc);
//Creating new nodes
GX_LDR_TABLE_ENTRY(GXxmlNewDocNode_entry, "GXxmlNewDocNode", &GXxmlNewDocNode);
GX_LDR_TABLE_ENTRY(GXxmlNewNode_entry, "GXxmlNewNode", &GXxmlNewNode);
GX_LDR_TABLE_ENTRY(GXxmlNewChild_entry, "GXxmlNewChild", &GXxmlNewChild);
GX_LDR_TABLE_ENTRY(GXxmlNewDocText_entry, "GXxmlNewDocText", &GXxmlNewDocText);
GX_LDR_TABLE_ENTRY(GXxmlNewText_entry, "GXxmlNewText", &GXxmlNewText);
GX_LDR_TABLE_ENTRY(GXxmlNewDocTextLen_entry, "GXxmlNewDocTextLen", &GXxmlNewDocTextLen);
GX_LDR_TABLE_ENTRY(GXxmlNewTextLen_entry, "GXxmlNewTextLen", &GXxmlNewTextLen);
GX_LDR_TABLE_ENTRY(GXxmlNewDocComment_entry, "GXxmlNewDocComment", &GXxmlNewDocComment);
GX_LDR_TABLE_ENTRY(GXxmlNewReference_entry, "GXxmlNewReference", &GXxmlNewReference);
GX_LDR_TABLE_ENTRY(GXxmlCopyNode_entry, "GXxmlCopyNode", &GXxmlCopyNode);
GX_LDR_TABLE_ENTRY(GXxmlCopyNodeList_entry, "GXxmlCopyNodeList", &GXxmlCopyNodeList);
//Navigating
GX_LDR_TABLE_ENTRY(GXxmlNextElementSibling_entry, "GXxmlNextElementSibling", &GXxmlNextElementSibling);
GX_LDR_TABLE_ENTRY(GXxmlGetLastChild_entry, "GXxmlGetLastChild", &GXxmlGetLastChild);
GX_LDR_TABLE_ENTRY(GXxmlNodeIsText_entry, "GXxmlNodeIsText", &GXxmlNodeIsText);
//Changing the structure
GX_LDR_TABLE_ENTRY(GXxmlAddChild_entry, "GXxmlAddChild", &GXxmlAddChild);
GX_LDR_TABLE_ENTRY(GXxmlUnlinkNode_entry, "GXxmlUnlinkNode", &GXxmlUnlinkNode);
GX_LDR_TABLE_ENTRY(GXxmlFirstElementChild_entry, "GXxmlFirstElementChild", &GXxmlFirstElementChild);
GX_LDR_TABLE_ENTRY(GXxmlChildElementCount_entry, "GXxmlChildElementCount", &GXxmlChildElementCount);
GX_LDR_TABLE_ENTRY(GXxmlDocGetRootElement_entry, "GXxmlDocGetRootElement", &GXxmlDocGetRootElement);
GX_LDR_TABLE_ENTRY(GXxmlTextMerge_entry, "GXxmlTextMerge", &GXxmlTextMerge);
GX_LDR_TABLE_ENTRY(GXxmlTextConcat_entry, "GXxmlTextConcat", &GXxmlTextConcat);
GX_LDR_TABLE_ENTRY(GXxmlFreeNodeList_entry, "GXxmlFreeNodeList", &GXxmlFreeNodeList);
GX_LDR_TABLE_ENTRY(GXxmlFreeNode_entry, "GXxmlFreeNode", &GXxmlFreeNode);
//Namespaces
GX_LDR_TABLE_ENTRY(GXxmlSearchNs_entry, "GXxmlSearchNs", &GXxmlSearchNs);
GX_LDR_TABLE_ENTRY(GXxmlSearchNsByHref_entry, "GXxmlSearchNsByHref", &GXxmlSearchNsByHref);
GX_LDR_TABLE_ENTRY(GXxmlSetNs_entry, "GXxmlSetNs", &GXxmlSetNs);
GX_LDR_TABLE_ENTRY(GXxmlCopyNamespace_entry, "GXxmlCopyNamespace", &GXxmlCopyNamespace);
GX_LDR_TABLE_ENTRY(GXxmlCopyNamespaceList_entry, "GXxmlCopyNamespaceList", &GXxmlCopyNamespaceList);
//Changing the content
GX_LDR_TABLE_ENTRY(GXxmlSetProp_entry, "GXxmlSetProp", &GXxmlSetProp);
GX_LDR_TABLE_ENTRY(GXxmlGetProp_entry, "GXxmlGetProp", &GXxmlGetProp);
GX_LDR_TABLE_ENTRY(GXxmlStringGetNodeList_entry, "GXxmlStringGetNodeList", &GXxmlStringGetNodeList);
GX_LDR_TABLE_ENTRY(GXxmlStringLenGetNodeList_entry, "GXxmlStringLenGetNodeList", &GXxmlStringLenGetNodeList);
GX_LDR_TABLE_ENTRY(GXxmlNodeListGetString_entry, "GXxmlNodeListGetString", &GXxmlNodeListGetString);
GX_LDR_TABLE_ENTRY(GXxmlNodeSetContent_entry, "GXxmlNodeSetContent", &GXxmlNodeSetContent);
GX_LDR_TABLE_ENTRY(GXxmlNodeSetContentLen_entry, "GXxmlNodeSetContentLen", &GXxmlNodeSetContentLen);
GX_LDR_TABLE_ENTRY(GXxmlNodeAddContent_entry, "GXxmlNodeAddContent", &GXxmlNodeAddContent);
GX_LDR_TABLE_ENTRY(GXxmlNodeAddContentLen_entry, "GXxmlNodeAddContentLen", &GXxmlNodeAddContentLen);
GX_LDR_TABLE_ENTRY(GXxmlNodeGetContent_entry, "GXxmlNodeGetContent", &GXxmlNodeGetContent);
GX_LDR_TABLE_ENTRY(GXxmlFreeChar_entry, "GXxmlFreeChar", &GXxmlFreeChar);
//Saving
GX_LDR_TABLE_ENTRY(GXxmlDocDumpMemory_entry, "GXxmlDocDumpMemory", &GXxmlDocDumpMemory);
GX_LDR_TABLE_ENTRY(GXxmlDocDump_entry, "GXxmlDocDump", &GXxmlDocDump);
GX_LDR_TABLE_ENTRY(GXxmlSaveFile_entry, "GXxmlSaveFile", &GXxmlSaveFile);
//Compression
GX_LDR_TABLE_ENTRY(GXxmlGetDocCompressMode_entry, "GXxmlGetDocCompressMode", &GXxmlGetDocCompressMode);
GX_LDR_TABLE_ENTRY(GXxmlSetDocCompressMode_entry, "GXxmlSetDocCompressMode", &GXxmlSetDocCompressMode);
GX_LDR_TABLE_ENTRY(GXxmlGetCompressMode_entry, "GXxmlGetCompressMode", &GXxmlGetCompressMode);
GX_LDR_TABLE_ENTRY(GXxmlSetCompressMode_entry, "GXxmlSetCompressMode", &GXxmlSetCompressMode);

//parser.h
GX_LDR_TABLE_ENTRY(GXxmlParseDocument_entry, "GXxmlParseDocument", &GXxmlParseDocument);
GX_LDR_TABLE_ENTRY(GXxmlParseDoc_entry, "GXxmlParseDoc", &GXxmlParseDoc);
GX_LDR_TABLE_ENTRY(GXxmlParseMemory_entry, "GXxmlParseMemory", &GXxmlParseMemory);
GX_LDR_TABLE_ENTRY(GXxmlParseFile_entry, "GXxmlParseFile", &GXxmlParseFile);
GX_LDR_TABLE_ENTRY(GXxmlParseBuffer_entry, "GXxmlParseBuffer", &GXxmlParseBuffer);
GX_LDR_TABLE_ENTRY(GXxmlSAXParseDoc_entry, "GXxmlSAXParseDoc", &GXxmlSAXParseDoc);
GX_LDR_TABLE_ENTRY(GXxmlSAXParseMemory_entry, "GXxmlSAXParseMemory", &GXxmlSAXParseMemory);
GX_LDR_TABLE_ENTRY(GXxmlSAXParseFile_entry, "GXxmlSAXParseFile", &GXxmlSAXParseFile);
GX_LDR_TABLE_ENTRY(GXxmlSAXParseBuffer_entry, "GXxmlSAXParseBuffer", &GXxmlSAXParseBuffer);
GX_LDR_TABLE_ENTRY(GXxmlStrdup_entry, "GXxmlStrdup", &GXxmlStrdup);
GX_LDR_TABLE_ENTRY(GXxmlStrndup_entry, "GXxmlStrndup", &GXxmlStrndup);
GX_LDR_TABLE_ENTRY(GXxmlStrchr_entry, "GXxmlStrchr", &GXxmlStrchr);
GX_LDR_TABLE_ENTRY(GXxmlStrcmp_entry, "GXxmlStrcmp", &GXxmlStrcmp);
GX_LDR_TABLE_ENTRY(GXxmlStrncmp_entry, "GXxmlStrncmp", &GXxmlStrncmp);
GX_LDR_TABLE_ENTRY(GXxmlStrlen_entry, "GXxmlStrlen", &GXxmlStrlen);
GX_LDR_TABLE_ENTRY(GXxmlStrcat_entry, "GXxmlStrcat", &GXxmlStrcat);
GX_LDR_TABLE_ENTRY(GXxmlStrncat_entry, "GXxmlStrncat", &GXxmlStrncat);
GX_LDR_TABLE_ENTRY(GXxmlInitParserCtxt_entry, "GXxmlInitParserCtxt", &GXxmlInitParserCtxt);
GX_LDR_TABLE_ENTRY(GXxmlClearParserCtxt_entry, "GXxmlClearParserCtxt", &GXxmlClearParserCtxt);
GX_LDR_TABLE_ENTRY(GXxmlSetupParserForBuffer_entry, "GXxmlSetupParserForBuffer", &GXxmlSetupParserForBuffer);
GX_LDR_TABLE_ENTRY(GXxmlParserFindNodeInfo_entry, "GXxmlParserFindNodeInfo", &GXxmlParserFindNodeInfo);
GX_LDR_TABLE_ENTRY(GXxmlInitNodeInfoSeq_entry, "GXxmlInitNodeInfoSeq", &GXxmlInitNodeInfoSeq);
GX_LDR_TABLE_ENTRY(GXxmlClearNodeInfoSeq_entry, "GXxmlClearNodeInfoSeq", &GXxmlClearNodeInfoSeq);
GX_LDR_TABLE_ENTRY(GXxmlParserFindNodeInfoIndex_entry, "GXxmlParserFindNodeInfoIndex", &GXxmlParserFindNodeInfoIndex);
GX_LDR_TABLE_ENTRY(GXxmlParserAddNodeInfo_entry, "GXxmlParserAddNodeInfo", &GXxmlParserAddNodeInfo);
GX_LDR_TABLE_ENTRY(GXxmlParserWarning_entry, "GXxmlParserWarning", &GXxmlParserWarning);
GX_LDR_TABLE_ENTRY(GXxmlParserError_entry, "GXxmlParserError", &GXxmlParserError);
GX_LDR_TABLE_ENTRY(GXxmlDefaultSAXHandlerInit_entry, "GXxmlDefaultSAXHandlerInit", &GXxmlDefaultSAXHandlerInit);


//libcurl
//curl.h
//main
GX_LDR_TABLE_ENTRY(curl_formadd_entry, "curl_formadd", &curl_formadd);
GX_LDR_TABLE_ENTRY(curl_formget_entry, "curl_formget", &curl_formget);
GX_LDR_TABLE_ENTRY(curl_formfree_entry, "curl_formfree", &curl_formfree);
GX_LDR_TABLE_ENTRY(curl_getenv_entry, "curl_getenv", &curl_getenv);
GX_LDR_TABLE_ENTRY(curl_version_entry, "curl_version", &curl_version);
GX_LDR_TABLE_ENTRY(curl_easy_escape_entry, "curl_easy_escape", &curl_easy_escape);
GX_LDR_TABLE_ENTRY(curl_escape_entry, "curl_escape", &curl_escape);
GX_LDR_TABLE_ENTRY(curl_easy_unescape_entry, "curl_easy_unescape", &curl_easy_unescape);
GX_LDR_TABLE_ENTRY(curl_unescape_entry, "curl_unescape", &curl_unescape);
GX_LDR_TABLE_ENTRY(curl_free_entry, "curl_free", &curl_free);
GX_LDR_TABLE_ENTRY(curl_global_init_entry, "curl_global_init", &curl_global_init);
GX_LDR_TABLE_ENTRY(curl_global_init_mem_entry, "curl_global_init_mem", &curl_global_init_mem);
GX_LDR_TABLE_ENTRY(curl_global_cleanup_entry, "curl_global_cleanup", &curl_global_cleanup);
GX_LDR_TABLE_ENTRY(curl_slist_append_entry, "curl_slist_append", &curl_slist_append);
GX_LDR_TABLE_ENTRY(curl_slist_free_all_entry, "curl_slist_free_all", &curl_slist_free_all);
GX_LDR_TABLE_ENTRY(curl_getdate_entry, "curl_getdate", &curl_getdate);
GX_LDR_TABLE_ENTRY(curl_share_init_entry, "curl_share_init", &curl_share_init);
GX_LDR_TABLE_ENTRY(curl_share_setopt_entry, "curl_share_setopt", &curl_share_setopt);
GX_LDR_TABLE_ENTRY(curl_share_cleanup_entry, "curl_share_cleanup", &curl_share_cleanup);
GX_LDR_TABLE_ENTRY(curl_version_info_entry, "curl_version_info", &curl_version_info);
GX_LDR_TABLE_ENTRY(curl_easy_strerror_entry, "curl_easy_strerror", &curl_easy_strerror);
GX_LDR_TABLE_ENTRY(curl_share_strerror_entry, "curl_share_strerror", &curl_share_strerror);
GX_LDR_TABLE_ENTRY(curl_easy_pause_entry, "curl_easy_pause", &curl_easy_pause);
//easy
GX_LDR_TABLE_ENTRY(curl_easy_init_entry, "curl_easy_init", &curl_easy_init);
GX_LDR_TABLE_ENTRY(curl_easy_setopt_entry, "curl_easy_setopt", &curl_easy_setopt);
GX_LDR_TABLE_ENTRY(curl_easy_perform_entry, "curl_easy_perform", &curl_easy_perform);
GX_LDR_TABLE_ENTRY(curl_easy_cleanup_entry, "curl_easy_cleanup", &curl_easy_cleanup);
GX_LDR_TABLE_ENTRY(curl_easy_getinfo_entry, "curl_easy_getinfo", &curl_easy_getinfo);
GX_LDR_TABLE_ENTRY(curl_easy_duphandle_entry, "curl_easy_duphandle", &curl_easy_duphandle);
GX_LDR_TABLE_ENTRY(curl_easy_reset_entry, "curl_easy_reset", &curl_easy_reset);
GX_LDR_TABLE_ENTRY(curl_easy_recv_entry, "curl_easy_recv", &curl_easy_recv);
GX_LDR_TABLE_ENTRY(curl_easy_send_entry, "curl_easy_send", &curl_easy_send);
//multi
GX_LDR_TABLE_ENTRY(curl_multi_init_entry, "curl_multi_init", &curl_multi_init);
GX_LDR_TABLE_ENTRY(curl_multi_add_handle_entry, "curl_multi_add_handle", &curl_multi_add_handle);
GX_LDR_TABLE_ENTRY(curl_multi_remove_handle_entry, "curl_multi_remove_handle", &curl_multi_remove_handle);
GX_LDR_TABLE_ENTRY(curl_multi_fdset_entry, "curl_multi_fdset", &curl_multi_fdset);
GX_LDR_TABLE_ENTRY(curl_multi_perform_entry, "curl_multi_perform", &curl_multi_perform);
GX_LDR_TABLE_ENTRY(curl_multi_cleanup_entry, "curl_multi_cleanup", &curl_multi_cleanup);
GX_LDR_TABLE_ENTRY(curl_multi_info_read_entry, "curl_multi_info_read", &curl_multi_info_read);
GX_LDR_TABLE_ENTRY(curl_multi_strerror_entry, "curl_multi_strerror", &curl_multi_strerror);
GX_LDR_TABLE_ENTRY(curl_multi_socket_entry, "curl_multi_socket", &curl_multi_socket);
GX_LDR_TABLE_ENTRY(curl_multi_socket_action_entry, "curl_multi_socket_action", &curl_multi_socket_action);
GX_LDR_TABLE_ENTRY(curl_multi_socket_all_entry, "curl_multi_socket_all", &curl_multi_socket_all);
GX_LDR_TABLE_ENTRY(curl_multi_timeout_entry, "curl_multi_timeout", &curl_multi_timeout);
GX_LDR_TABLE_ENTRY(curl_multi_setopt_entry, "curl_multi_setopt", &curl_multi_setopt);
GX_LDR_TABLE_ENTRY(curl_multi_assign_entry, "curl_multi_assign", &curl_multi_assign);


//libcjson
//cJSON.h
GX_LDR_TABLE_ENTRY(cJSON_InitHooks_entry, "cJSON_InitHooks", &cJSON_InitHooks);
GX_LDR_TABLE_ENTRY(cJSON_Parse_entry, "cJSON_Parse", &cJSON_Parse);
GX_LDR_TABLE_ENTRY(cJSON_ParseWithFilters_entry, "cJSON_ParseWithFilters", &cJSON_ParseWithFilters);
GX_LDR_TABLE_ENTRY(cJSON_Print_entry, "cJSON_Print", &cJSON_Print);
GX_LDR_TABLE_ENTRY(cJSON_PrintUnformatted_entry, "cJSON_PrintUnformatted", &cJSON_PrintUnformatted);
GX_LDR_TABLE_ENTRY(cJSON_Delete_entry, "cJSON_Delete", &cJSON_Delete);
GX_LDR_TABLE_ENTRY(cJSON_GetArraySize_entry, "cJSON_GetArraySize", &cJSON_GetArraySize);
GX_LDR_TABLE_ENTRY(cJSON_GetArrayItem_entry, "cJSON_GetArrayItem", &cJSON_GetArrayItem);
GX_LDR_TABLE_ENTRY(cJSON_GetObjectItem_entry, "cJSON_GetObjectItem", &cJSON_GetObjectItem);
GX_LDR_TABLE_ENTRY(cJSON_GetErrorPtr_entry, "cJSON_GetErrorPtr", &cJSON_GetErrorPtr);
GX_LDR_TABLE_ENTRY(cJSON_CreateNull_entry, "cJSON_CreateNull", &cJSON_CreateNull);
GX_LDR_TABLE_ENTRY(cJSON_CreateTrue_entry, "cJSON_CreateTrue", &cJSON_CreateTrue);
GX_LDR_TABLE_ENTRY(cJSON_CreateFalse_entry, "cJSON_CreateFalse", &cJSON_CreateFalse);
GX_LDR_TABLE_ENTRY(cJSON_CreateBool_entry, "cJSON_CreateBool", &cJSON_CreateBool);
GX_LDR_TABLE_ENTRY(cJSON_CreateNumber_entry, "cJSON_CreateNumber", &cJSON_CreateNumber);
GX_LDR_TABLE_ENTRY(cJSON_CreateString_entry, "cJSON_CreateString", &cJSON_CreateString);
GX_LDR_TABLE_ENTRY(cJSON_CreateArray_entry, "cJSON_CreateArray", &cJSON_CreateArray);
GX_LDR_TABLE_ENTRY(cJSON_CreateObject_entry, "cJSON_CreateObject", &cJSON_CreateObject);
GX_LDR_TABLE_ENTRY(cJSON_CreateIntArray_entry, "cJSON_CreateIntArray", &cJSON_CreateIntArray);
GX_LDR_TABLE_ENTRY(cJSON_CreateFloatArray_entry, "cJSON_CreateFloatArray", &cJSON_CreateFloatArray);
GX_LDR_TABLE_ENTRY(cJSON_CreateDoubleArray_entry, "cJSON_CreateDoubleArray", &cJSON_CreateDoubleArray);
GX_LDR_TABLE_ENTRY(cJSON_CreateStringArray_entry, "cJSON_CreateStringArray", &cJSON_CreateStringArray);
GX_LDR_TABLE_ENTRY(cJSON_AddItemToArray_entry, "cJSON_AddItemToArray", &cJSON_AddItemToArray);
GX_LDR_TABLE_ENTRY(cJSON_AddItemToObject_entry, "cJSON_AddItemToObject", &cJSON_AddItemToObject);
GX_LDR_TABLE_ENTRY(cJSON_AddItemReferenceToArray_entry, "cJSON_AddItemReferenceToArray", &cJSON_AddItemReferenceToArray);
GX_LDR_TABLE_ENTRY(cJSON_AddItemReferenceToObject_entry, "cJSON_AddItemReferenceToObject", &cJSON_AddItemReferenceToObject);
GX_LDR_TABLE_ENTRY(cJSON_DetachItemFromArray_entry, "cJSON_DetachItemFromArray", &cJSON_DetachItemFromArray);
GX_LDR_TABLE_ENTRY(cJSON_DeleteItemFromArray_entry, "cJSON_DeleteItemFromArray", &cJSON_DeleteItemFromArray);
GX_LDR_TABLE_ENTRY(cJSON_DetachItemFromObject_entry, "cJSON_DetachItemFromObject", &cJSON_DetachItemFromObject);
GX_LDR_TABLE_ENTRY(cJSON_DeleteItemFromObject_entry, "cJSON_DeleteItemFromObject", &cJSON_DeleteItemFromObject);
GX_LDR_TABLE_ENTRY(cJSON_ReplaceItemInArray_entry, "cJSON_ReplaceItemInArray", &cJSON_ReplaceItemInArray);
GX_LDR_TABLE_ENTRY(cJSON_ReplaceItemInObject_entry, "cJSON_ReplaceItemInObject", &cJSON_ReplaceItemInObject);
GX_LDR_TABLE_ENTRY(cJSON_Duplicate_entry, "cJSON_Duplicate", &cJSON_Duplicate);
GX_LDR_TABLE_ENTRY(cJSON_ParseWithOpts_entry, "cJSON_ParseWithOpts", &cJSON_ParseWithOpts);


//appcore
//gx_apps.h
//OS
GX_LDR_TABLE_ENTRY(gx_os_thread_create_entry, "gx_os_thread_create", &gx_os_thread_create);
GX_LDR_TABLE_ENTRY(gx_os_thread_sleep_entry, "gx_os_thread_sleep", &gx_os_thread_sleep);
GX_LDR_TABLE_ENTRY(gx_os_mutex_create_entry, "gx_os_mutex_create", &gx_os_mutex_create);
GX_LDR_TABLE_ENTRY(gx_os_mutex_delete_entry, "gx_os_mutex_delete", &gx_os_mutex_delete);
GX_LDR_TABLE_ENTRY(gx_os_mutex_lock_entry, "gx_os_mutex_lock", &gx_os_mutex_lock);
GX_LDR_TABLE_ENTRY(gx_os_mutex_unlock_entry, "gx_os_mutex_unlock", &gx_os_mutex_unlock);
GX_LDR_TABLE_ENTRY(gxapps_mutex_lock_entry, "gxapps_mutex_lock", &gxapps_mutex_lock);
GX_LDR_TABLE_ENTRY(gxapps_mutex_unlock_entry, "gxapps_mutex_unlock", &gxapps_mutex_unlock);
GX_LDR_TABLE_ENTRY(gx_get_tick_time_entry, "gx_get_tick_time", &gx_get_tick_time);
//gx string functions
GX_LDR_TABLE_ENTRY(gx_url_encode_entry, "gx_url_encode", &gx_url_encode);
GX_LDR_TABLE_ENTRY(gx_url_decode_entry, "gx_url_decode", &gx_url_decode);
GX_LDR_TABLE_ENTRY(gx_time_itoa_entry, "gx_time_itoa", &gx_time_itoa);
GX_LDR_TABLE_ENTRY(gxapps_strncpy_entry, "gxapps_strncpy", &gxapps_strncpy);
GX_LDR_TABLE_ENTRY(gx_strndup_entry, "gx_strndup", &gx_strndup);
GX_LDR_TABLE_ENTRY(gx_strrstr_entry, "gx_strrstr", &gx_strrstr);
GX_LDR_TABLE_ENTRY(gx_strrev_entry, "gx_strrev", &gx_strrev);
GX_LDR_TABLE_ENTRY(gx_strtok_r_entry, "gx_strtok_r", &gx_strtok_r);
GX_LDR_TABLE_ENTRY(gx_hex2str_entry, "gx_hex2str", &gx_hex2str);
//ssl functions
GX_LDR_TABLE_ENTRY(gx_crc32_entry, "gx_crc32", &gx_crc32);
GX_LDR_TABLE_ENTRY(gx_base64_encode_entry, "gx_base64_encode", &gx_base64_encode);
GX_LDR_TABLE_ENTRY(gx_base64_decode_entry, "gx_base64_decode", &gx_base64_decode);
GX_LDR_TABLE_ENTRY(gx_str_enc_entry, "gx_str_enc", &gx_str_enc);
GX_LDR_TABLE_ENTRY(gx_str_dec_entry, "gx_str_dec", &gx_str_dec);
GX_LDR_TABLE_ENTRY(gx_md5_sign_entry, "gx_md5_sign", &gx_md5_sign);
GX_LDR_TABLE_ENTRY(gx_md5_sign_file_entry, "gx_md5_sign_file", &gx_md5_sign_file);
GX_LDR_TABLE_ENTRY(gx_hmac_sha_sign_entry, "gx_hmac_sha_sign", &gx_hmac_sha_sign);
GX_LDR_TABLE_ENTRY(gx_hmac_sha256_sign_entry, "gx_hmac_sha256_sign", &gx_hmac_sha256_sign);
GX_LDR_TABLE_ENTRY(gx_aes_cbc_encrypt_entry, "gx_aes_cbc_encrypt", &gx_aes_cbc_encrypt);
GX_LDR_TABLE_ENTRY(gx_aes_cbc_decrypt_entry, "gx_aes_cbc_decrypt", &gx_aes_cbc_decrypt);
GX_LDR_TABLE_ENTRY(gx_des_ecb_encrypt_entry, "gx_des_ecb_encrypt", &gx_des_ecb_encrypt);
GX_LDR_TABLE_ENTRY(gx_des_ecb_decrypt_entry, "gx_des_ecb_decrypt", &gx_des_ecb_decrypt);
GX_LDR_TABLE_ENTRY(gx_des_cbc_encrypt_entry, "gx_des_cbc_encrypt", &gx_des_cbc_encrypt);
GX_LDR_TABLE_ENTRY(gx_des_cbc_decrypt_entry, "gx_des_cbc_decrypt", &gx_des_cbc_decrypt);
//libcurl functions
GX_LDR_TABLE_ENTRY(gxapps_curl_set_timeout_entry, "gxapps_curl_set_timeout", &gxapps_curl_set_timeout);
GX_LDR_TABLE_ENTRY(gxapps_curl_download_entry, "gxapps_curl_download", &gxapps_curl_download);
GX_LDR_TABLE_ENTRY(gxapps_curl_free_entry, "gxapps_curl_free", &gxapps_curl_free);
GX_LDR_TABLE_ENTRY(gxapps_curl_httpfunc_entry, "gxapps_curl_httpfunc", &gxapps_curl_httpfunc);
GX_LDR_TABLE_ENTRY(gxapps_curl_async_download_entry, "gxapps_curl_async_download", &gxapps_curl_async_download);
GX_LDR_TABLE_ENTRY(gxapps_curl_async_download_ext_entry, "gxapps_curl_async_download_ext", &gxapps_curl_async_download_ext);
GX_LDR_TABLE_ENTRY(gxapps_curl_multi_async_download_entry, "gxapps_curl_multi_async_download", &gxapps_curl_multi_async_download);
GX_LDR_TABLE_ENTRY(gxapps_curl_async_abort_entry, "gxapps_curl_async_abort", &gxapps_curl_async_abort);
//gx list functions
GX_LDR_TABLE_ENTRY(gx_list_new_entry, "gx_list_new", &gx_list_new);
GX_LDR_TABLE_ENTRY(gx_list_add_item_entry, "gx_list_add_item", &gx_list_add_item);
GX_LDR_TABLE_ENTRY(gx_list_edit_item_entry, "gx_list_edit_item", &gx_list_edit_item);
GX_LDR_TABLE_ENTRY(gx_list_delete_item_entry, "gx_list_delete_item", &gx_list_delete_item);
GX_LDR_TABLE_ENTRY(gx_get_list_from_file_entry, "gx_get_list_from_file", &gx_get_list_from_file);
GX_LDR_TABLE_ENTRY(gx_search_list_type_entry, "gx_search_list_type", &gx_search_list_type);
GX_LDR_TABLE_ENTRY(gx_list_get_mode_entry, "gx_list_get_mode", &gx_list_get_mode);
GX_LDR_TABLE_ENTRY(gx_list_get_mode_method_entry, "gx_list_get_mode_method", &gx_list_get_mode_method);
GX_LDR_TABLE_ENTRY(gx_list_get_method_entry, "gx_list_get_method", &gx_list_get_method);
GX_LDR_TABLE_ENTRY(gx_list_free_entry, "gx_list_free", &gx_list_free);
GX_LDR_TABLE_ENTRY(gx_merge_list_entry, "gx_merge_list", &gx_merge_list);
GX_LDR_TABLE_ENTRY(gx_list_save_to_file_entry, "gx_list_save_to_file", &gx_list_save_to_file);
//stb config functions
GX_LDR_TABLE_ENTRY(gxapps_get_chip_sn_str_entry, "gxapps_get_chip_sn_str", &gxapps_get_chip_sn_str);
GX_LDR_TABLE_ENTRY(gxapps_get_chip_name_str_entry, "gxapps_get_chip_name_str", &gxapps_get_chip_name_str);
GX_LDR_TABLE_ENTRY(gxapps_get_customer_name_str_entry, "gxapps_get_customer_name_str", &gxapps_get_customer_name_str);
GX_LDR_TABLE_ENTRY(gxapps_get_customer_model_str_entry, "gxapps_get_customer_model_str", &gxapps_get_customer_model_str);
GX_LDR_TABLE_ENTRY(gxapps_get_os_str_entry, "gxapps_get_os_str", &gxapps_get_os_str);
GX_LDR_TABLE_ENTRY(gxapps_get_platform_version_str_entry, "gxapps_get_platform_version_str", &gxapps_get_platform_version_str);
GX_LDR_TABLE_ENTRY(gxapps_get_app_version_str_entry, "gxapps_get_app_version_str", &gxapps_get_app_version_str);
GX_LDR_TABLE_ENTRY(gxapps_get_ssl_version_str_entry, "gxapps_get_ssl_version_str", &gxapps_get_ssl_version_str);
GX_LDR_TABLE_ENTRY(gxapps_get_curl_version_str_entry, "gxapps_get_curl_version_str", &gxapps_get_curl_version_str);
GX_LDR_TABLE_ENTRY(gxapps_get_appcore_version_str_entry, "gxapps_get_appcore_version_str", &gxapps_get_appcore_version_str);
GX_LDR_TABLE_ENTRY(gxapps_get_country_str_entry, "gxapps_get_country_str", &gxapps_get_country_str);
GX_LDR_TABLE_ENTRY(gxapps_get_region_str_entry, "gxapps_get_region_str", &gxapps_get_region_str);
GX_LDR_TABLE_ENTRY(gxapps_get_city_str_entry, "gxapps_get_city_str", &gxapps_get_city_str);
//cloud functions
GX_LDR_TABLE_ENTRY(gxapps_check_authorization_entry, "gxapps_check_authorization", &gxapps_check_authorization);
GX_LDR_TABLE_ENTRY(gxapps_get_config_entry, "gxapps_get_config", &gxapps_get_config);
GX_LDR_TABLE_ENTRY(gxapps_get_server_time_entry, "gxapps_get_server_time", &gxapps_get_server_time);
GX_LDR_TABLE_ENTRY(gxapps_get_check_value_entry, "gxapps_get_check_value", &gxapps_get_check_value);
GX_LDR_TABLE_ENTRY(gxapps_get_server_url_entry, "gxapps_get_server_url", &gxapps_get_server_url);
GX_LDR_TABLE_ENTRY(gxapps_get_server_post_url_entry, "gxapps_get_server_post_url", &gxapps_get_server_post_url);
GX_LDR_TABLE_ENTRY(gxapps_get_server_user_agent_entry, "gxapps_get_server_user_agent", &gxapps_get_server_user_agent);
#endif

//gxcore
//gxcore_os_core.h
GX_LDR_TABLE_ENTRY(GxCore_ThreadCreate_entry, "GxCore_ThreadCreate", &GxCore_ThreadCreate);
GX_LDR_TABLE_ENTRY(GxCore_ThreadDelay_entry, "GxCore_ThreadDelay", &GxCore_ThreadDelay);
GX_LDR_TABLE_ENTRY(GxCore_ThreadJoin_entry, "GxCore_ThreadJoin", &GxCore_ThreadJoin);
GX_LDR_TABLE_ENTRY(GxCore_ThreadDetach_entry, "GxCore_ThreadDetach", &GxCore_ThreadDetach);
GX_LDR_TABLE_ENTRY(GxCore_DlOpen_entry, "GxCore_DlOpen", &GxCore_DlOpen);
GX_LDR_TABLE_ENTRY(GxCore_DlSym_entry, "GxCore_DlSym", &GxCore_DlSym);
GX_LDR_TABLE_ENTRY(GxCore_DlClose_entry, "GxCore_DlClose", &GxCore_DlClose);
GX_LDR_TABLE_ENTRY(GxCore_DlError_entry, "GxCore_DlError", &GxCore_DlError);


//gui
//gui_key.h
GX_LDR_TABLE_ENTRY(find_virtualkeys_entry, "find_virtualkeys", &find_virtualkeys);
GX_LDR_TABLE_ENTRY(find_virtualkey_entry, "find_virtualkey", &find_virtualkey);

//gui_core.h
GX_LDR_TABLE_ENTRY(GUI_CreateDialog_entry, "GUI_CreateDialog", &GUI_CreateDialog);
GX_LDR_TABLE_ENTRY(GUI_LinkDialog_entry, "GUI_LinkDialog", &GUI_LinkDialog);
GX_LDR_TABLE_ENTRY(GUI_CheckDialog_entry, "GUI_CheckDialog", &GUI_CheckDialog);
GX_LDR_TABLE_ENTRY(GUI_EndDialog_entry, "GUI_EndDialog", &GUI_EndDialog);
GX_LDR_TABLE_ENTRY(GUI_CreatePrompt_entry, "GUI_CreatePrompt", &GUI_CreatePrompt);
GX_LDR_TABLE_ENTRY(GUI_CheckPrompt_entry, "GUI_CheckPrompt", &GUI_CheckPrompt);
GX_LDR_TABLE_ENTRY(GUI_PromptString_entry, "GUI_PromptString", &GUI_PromptString);
GX_LDR_TABLE_ENTRY(GUI_EndPrompt_entry, "GUI_EndPrompt", &GUI_EndPrompt);
GX_LDR_TABLE_ENTRY(GUI_SetProperty_entry, "GUI_SetProperty", &GUI_SetProperty);
GX_LDR_TABLE_ENTRY(GUI_GetProperty_entry, "GUI_GetProperty", &GUI_GetProperty);
GX_LDR_TABLE_ENTRY(GUI_SetFocusWidget_entry, "GUI_SetFocusWidget", &GUI_SetFocusWidget);
GX_LDR_TABLE_ENTRY(GUI_GetFocusWidget_entry, "GUI_GetFocusWidget", &GUI_GetFocusWidget);
GX_LDR_TABLE_ENTRY(GUI_GetFocusWindow_entry, "GUI_GetFocusWindow", &GUI_GetFocusWindow);
GX_LDR_TABLE_ENTRY(GUI_SetInterface_entry, "GUI_SetInterface", &GUI_SetInterface);
GX_LDR_TABLE_ENTRY(GUI_SignalConnect_entry, "GUI_SignalConnect", &GUI_SignalConnect);
GX_LDR_TABLE_ENTRY(GUI_SendEvent_entry, "GUI_SendEvent", &GUI_SendEvent);
GX_LDR_TABLE_ENTRY(GUI_LoadWidget_entry, "GUI_LoadWidget", &GUI_LoadWidget);
GX_LDR_TABLE_ENTRY(GUI_RegisterWidget_entry, "GUI_RegisterWidget", &GUI_RegisterWidget);
GX_LDR_TABLE_ENTRY(GUI_GetSignal_entry, "GUI_GetSignal", &GUI_GetSignal);
GX_LDR_TABLE_ENTRY(GUI_Load_entry, "GUI_Load", &GUI_Load);
GX_LDR_TABLE_ENTRY(GUI_Unload_entry, "GUI_Unload", &GUI_Unload);

//gui_timer.h
GX_LDR_TABLE_ENTRY(create_timer_entry, "create_timer", &create_timer);
GX_LDR_TABLE_ENTRY(reset_timer_entry, "reset_timer", &reset_timer);
GX_LDR_TABLE_ENTRY(remove_timer_entry, "remove_timer", &remove_timer);
GX_LDR_TABLE_ENTRY(timer_stop_entry, "timer_stop", &timer_stop);

#endif