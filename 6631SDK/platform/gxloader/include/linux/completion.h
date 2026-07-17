#ifndef __LINUX_COMPLETION_H__
#define __LINUX_COMPLETION_H__

#include <cyg/kernel/kapi.h>

struct completion {
	unsigned int done;
	cyg_sem_t wait;
};

#endif /* __LINUX_COMPLETION_H__ */

