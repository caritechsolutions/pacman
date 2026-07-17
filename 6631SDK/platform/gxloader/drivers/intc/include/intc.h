#ifndef __INTC_DRIVER_H__
#define __INTC_DRIVER_H__
#include <sys/types.h>
#include <sys/queue.h>

enum interrupt_type {
	IRQ,
	FIQ,
};

enum interrupt_return {
	HANDLED,
	NONE,
};

typedef enum interrupt_return (*interrupt_handler_t) (uint32_t interrupt, void *data);

struct itr_handler {
	uint32_t it;
	uint32_t controller_num;
	enum interrupt_type type;
	interrupt_handler_t handler;
	void *data;
	SLIST_ENTRY(itr_handler) link;
};

#endif /*__INTC_DRIVER_H__*/
