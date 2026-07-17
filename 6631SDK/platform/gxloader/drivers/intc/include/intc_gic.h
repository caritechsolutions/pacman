#ifndef __INTC_GIC_H
#define __INTC_GIC_H
#include "sys/types.h"
#include "intc.h"

/* gic init */
void gic_op_init(uint32_t gic_base);
/* show interrupt state in gic */
void gic_op_dump_state(void);
/* interrupt handle fun of irq */
void gic_op_irq_handle(void);
/* interrupt handle fun of fiq */
void gic_op_fiq_handle(void);
/* request a interrupt */
int gic_op_request(uint32_t interrupt, enum interrupt_type type,
		interrupt_handler_t handler, void *data);
/* free a interrupt */
void gic_op_free(uint32_t interrupt);
/* enable a interrupt */
void gic_op_enable(uint32_t interrupt);
/* disable a interrupt */
void gic_op_disable(uint32_t interrupt);
/* disable all interrupt in gic */
void gic_op_all_disable(void);

#endif /*__INTC_GIC_H*/
