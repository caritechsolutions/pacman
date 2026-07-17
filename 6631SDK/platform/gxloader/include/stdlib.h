#ifndef __STDLIB_H__
#define __STDLIB_H__

/* 如下三个函数的实现不标准，因此没有作为标准 c 库提供给 gxloader（实现放在了 ate/ 中），
 * 如果后续有标准的实现，直接将实现放到 gxloader 中对应的目录即可 */
void qsort (void *base, unsigned num, unsigned width, int (*comp)(const void *, const void *));
long atol(const char *nptr);
char *getenv(const char *name);

void exit(int status);

#endif
