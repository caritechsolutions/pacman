#ifndef ASSERT_H
#define ASSERT_H
#include "stdio.h"

#ifdef NDEBUG
#define assert(expr)	do { } while (0)
#else
#define assert(expr) \
	do { \
		if (!(expr)) { \
			printf("failed---> %s : %s : %d\n", __FILE__, __func__, __LINE__); \
			while(1); \
		} \
	} while (0)
#endif

#define COMPILE_TIME_ASSERT(x) \
	do { \
		switch (0) { case 0: case ((x) ? 1: 0): default : break; } \
	} while (0)

#endif
