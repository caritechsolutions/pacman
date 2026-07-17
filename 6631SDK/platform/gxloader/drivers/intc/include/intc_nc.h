#ifndef __INTC_NC_H
#define __INTC_NC_H
#include "sys/types.h"
#include "intc.h"

/* nc init */
void nc_op_init(uint32_t nc_base);
/* show interrupt state in nc */
void nc_op_dump_state(void);
/* interrupt handle fun of irq */
void nc_op_irq_handle(void);
/* interrupt handle fun of fiq */
void nc_op_fiq_handle(void);
/* request a interrupt */
int nc_op_request(uint32_t interrupt, enum interrupt_type type,
		interrupt_handler_t handler, void *data);
/* free a interrupt */
void nc_op_free(uint32_t interrupt);
/* enable a interrupt */
void nc_op_enable(uint32_t interrupt);
/* disable a interrupt */
void nc_op_disable(uint32_t interrupt);
/* disable all interrupt in nc */
void nc_op_all_disable(void);

#endif /*__INTC_NC_H*/
