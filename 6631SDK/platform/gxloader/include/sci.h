#ifndef __SCI_H__
#define __SCI_H__

typedef enum {
	SCI_POL_LOW,
	SCI_POL_HIGH,
} SCI_POL;

int gxsci_init_vcc_polarity(SCI_POL vcc);
#endif

