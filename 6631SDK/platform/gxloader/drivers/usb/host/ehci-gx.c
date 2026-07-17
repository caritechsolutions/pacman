#include <sys/types.h>
#include <common/io.h>
#include <usb.h>

#include "ehci.h"
#include "gxhwlib_registers.h"

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	*hccr = (struct ehci_hccr *)(REG_BASE_USB_EHCI);
	*hcor = (struct ehci_hcor *)((uint32_t) *hccr +
		HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	return 0;
}