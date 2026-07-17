#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include "sys/types.h"


enum interrupt_type {
	IRQ,
	FIQ,
};

enum interrupt_return {
	HANDLED,
	NONE,
};

typedef enum interrupt_return (*interrupt_handler_t) (uint32_t interrupt, void *data);

/*
 * gx_interrupt_init()
 *
 *  DESCRIPTION
 *      Initialize the interrupt controller
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_interrupt_init(void);

/*
 * gx_request_interrupt()
 *
 *  DESCRIPTION
 *      Request a interrupt
 *
 *  PARAMETER
 *      @interrupt
 *           The number of the interrupt to be requested
 *      @interrupt_type
 *           The type of the interrupt to be requested，IRQ or FIQ
 *      @interrupt_handler_t
 *           The interrupt processing function
 *      @pdata
 *           Private data
 *
 *  RETURN VALUE
 *      On sucess, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int gx_request_interrupt(uint32_t interrupt, enum interrupt_type type,
		interrupt_handler_t handler, void *data);

/*
 * gx_free_interrupt()
 *
 *  DESCRIPTION
 *      Free the interrupt that has been Requested
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_free_interrupt(uint32_t interrupt);

/*
 * gx_enable_irq()
 *
 *  DESCRIPTION
 *      Enable the IRQ Interrupt
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_enable_irq(void);

/*
 * gx_enable_fiq()
 *
 *  DESCRIPTION
 *      Enable the FIQ Interrupt
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_enable_fiq(void);

/*
 * gx_disable_irq()
 *
 *  DESCRIPTION
 *      Disable the IRQ Interrupt
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_disable_irq(void);

/*
 * gx_disable_fiq()
 *
 *  DESCRIPTION
 *      Disable the FIQ Interrupt
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_disable_fiq(void);

/*
 * gx_mask_interrupt()
 *
 *  DESCRIPTION
 *      Mask Interrupt
 *
 *  PARAMETER
 *   @interrupt
 *       The interrupt to be Masked
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_mask_interrupt(uint32_t interrupt);

/*
 * gx_unmask_interrupt()
 *
 *  DESCRIPTION
 *      Unmask Interrupt
 *
 *  PARAMETER
 *   @interrupt
 *       The interrupt to be Unmasked
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_unmask_interrupt(uint32_t interrupt);

/*
 * gx_irq_handler()
 *
 *  DESCRIPTION
 *      The interrupt handler function of irq
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_irq_handler(void);

/*
 * gx_fiq_handler()
 *
 *  DESCRIPTION
 *      The interrupt handler function of fiq
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_fiq_handler(void);

/*
 * gx_disable_all_interrupt()
 *
 *  DESCRIPTION
 *      Disable all Interrupt, include FIQ and IRQ
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_disable_all_interrupt(void);

#endif
