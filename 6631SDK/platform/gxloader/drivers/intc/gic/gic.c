#include <sys/assert.h>
#include <linux/kernel.h>
#include <io.h>
#include "stdio.h"
#include "string.h"
#include "../include/intc.h"

/* Offsets from gic.gicc_base */
#define GICC_CTLR		(0x000)
#define GICC_PMR		(0x004)
#define GICC_IAR		(0x00C)
#define GICC_AIAR		(0x020)
#define GICC_EOIR		(0x010)
#define GICC_AEOIR		(0x024)

#define GICC_CTLR_ENABLEGRP0	(1 << 0)
#define GICC_CTLR_ENABLEGRP1	(1 << 1)
#define GICC_CTLR_FIQEN		(1 << 3)

/* Offsets from gic.gicd_base */
#define GICD_CTLR		(0x000)
#define GICD_TYPER		(0x004)
#define GICD_IGROUPR(n)		(0x080 + (n) * 4)
#define GICD_ISENABLER(n)	(0x100 + (n) * 4)
#define GICD_ICENABLER(n)	(0x180 + (n) * 4)
#define GICD_ISPENDR(n)		(0x200 + (n) * 4)
#define GICD_ICPENDR(n)		(0x280 + (n) * 4)
#define GICD_IPRIORITYR(n)	(0x400 + (n) * 4)
#define GICD_ITARGETSR(n)	(0x800 + (n) * 4)
#define GICD_SGIR		(0xF00)

#define GICD_CTLR_ENABLEGRP0	(1 << 0)
#define GICD_CTLR_ENABLEGRP1	(1 << 1)

/* Number of Private Peripheral Interrupt */
#define NUM_PPI	32

/* Number of Software Generated Interrupt */
#define NUM_SGI			16

/* Number of Non-secure Software Generated Interrupt */
#define NUM_NS_SGI		8

/* Number of interrupts in one register */
#define NUM_INTS_PER_REG	32

/* Number of targets in one register */
#define NUM_TARGETS_PER_REG	4

/* Accessors to access ITARGETSRn */
#define ITARGETSR_FIELD_BITS	8
#define ITARGETSR_FIELD_MASK	0xff

/* Maximum number of interrups a GIC can support */
#define GIC_MAX_INTS		1020

#define GICC_IAR_IT_ID_MASK	0x3ff
#define GICC_IAR_CPU_ID_MASK	0x7
#define GICC_IAR_CPU_ID_SHIFT	10

#define GIC_DISTRIBUTOR_OFFSET          0x1000
#define GIC_CPU_INTERFACE_OFFSET        0x2000

#define BIT32(nr)		(1 << (nr))
#define SHIFT_U32(v, shift)     ((v) << (shift))

//#define GIC_DEBUG
#ifdef GIC_DEBUG
#define DMSG printf
#else
#define DMSG
#endif

#define INTC_NUM	32
#define INTC_NUM_MODFIY		// 为了保持和linux上使用的中断号一致，所以在内部自动加上32，这样导致了所有的SGI和PPI中断不能使用

struct gic_data {
	uint32_t gicc_base;
	uint32_t gicd_base;
	uint32_t max_it;
};

static struct gic_data global_gd;
static SLIST_HEAD(, itr_handler) handlers = SLIST_HEAD_INITIALIZER(handlers);

static uint32_t probe_max_it(uint32_t gicc_base, uint32_t gicd_base)
{
	int i;
	uint32_t old_ctlr;
	uint32_t ret = 0;
	const uint32_t max_regs = ((GIC_MAX_INTS + NUM_INTS_PER_REG - 1) / NUM_INTS_PER_REG) - 1;

	/*
	 * Probe which interrupt number is the largest.
	 */
#if defined(CFG_ARM_GICV3)
	old_ctlr = read_icc_ctlr();
	write_icc_ctlr(0);
#else
	old_ctlr = __raw_readl(gicc_base + GICC_CTLR);
	__raw_writel(0, gicc_base + GICC_CTLR);
#endif
	for (i = max_regs; i >= 0; i--) {
		uint32_t old_reg;
		uint32_t reg;
		int b;

		old_reg = __raw_readl(gicd_base + GICD_ISENABLER(i));
		__raw_writel(0xffffffff, gicd_base + GICD_ISENABLER(i));
		reg = __raw_readl(gicd_base + GICD_ISENABLER(i));
		__raw_writel(old_reg, gicd_base + GICD_ICENABLER(i));
		for (b = NUM_INTS_PER_REG - 1; b >= 0; b--) {
			if (BIT32(b) & reg) {
				ret = i * NUM_INTS_PER_REG + b;
				goto out;
			}
		}
	}
out:
#if defined(CFG_ARM_GICV3)
	write_icc_ctlr(old_ctlr);
#else
	__raw_writel(old_ctlr, gicc_base + GICC_CTLR);
#endif
	return ret;
}

// initial cpu if only, mainly use for secondary cpu setup cpu interface
static void gic_cpu_init(void)
{
	struct gic_data *gd = &global_gd;

#if defined(CFG_ARM_GICV3)
	assert(gd->gicd_base);
#else
	assert(gd->gicd_base && gd->gicc_base);
#endif

	/* per-CPU interrupts config:
	 * ID0-ID7(SGI)   for Non-secure interrupts
	 * ID8-ID15(SGI)  for Secure interrupts.
	 * All PPI config as Non-secure interrupts.
	 */
	__raw_writel(0xffff00ff, gd->gicd_base + GICD_IGROUPR(0));

	/* Set the priority mask to permit Non-secure interrupts, and to
	 * allow the Non-secure world to adjust the priority mask itself
	 */
#if defined(CFG_ARM_GICV3)
	write_icc_pmr(0x80);
	write_icc_ctlr(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 |
		       GICC_CTLR_FIQEN);
#else
	__raw_writel(0x80, gd->gicc_base + GICC_PMR);

	/* Enable GIC */
	__raw_writel(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 | GICC_CTLR_FIQEN,
		gd->gicc_base + GICC_CTLR);
#endif
}

static void gic_init_base_addr(struct gic_data *gd, uint32_t gic_base)
{
	gd->gicd_base = gic_base + GIC_DISTRIBUTOR_OFFSET;
	gd->gicc_base = gic_base + GIC_CPU_INTERFACE_OFFSET;
	gd->max_it = probe_max_it(gd->gicc_base, gd->gicd_base);
}

static void gic_it_add(struct gic_data *gd, uint32_t it, enum interrupt_type type)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);
	uint32_t orig_value = 0;
	uint32_t new_value = 0;

	/* Disable the interrupt */
	__raw_writel(mask, gd->gicd_base + GICD_ICENABLER(idx));
	/* Make it non-pending */
	__raw_writel(mask, gd->gicd_base + GICD_ICPENDR(idx));
	/* Assign it to group0 or group1 */
	orig_value = __raw_readl(gd->gicd_base + GICD_IGROUPR(idx));

	new_value = (orig_value & ~mask);

	__raw_writel(new_value, gd->gicd_base + GICD_IGROUPR(idx));
}

static void gic_it_set_cpu_mask(struct gic_data *gd, uint32_t it, uint8_t cpu_mask)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);
	uint32_t target, target_shift;

	/* Route it to selected CPUs */
	target = __raw_readl(gd->gicd_base + GICD_ITARGETSR(it / NUM_TARGETS_PER_REG));
	target_shift = (it % NUM_TARGETS_PER_REG) * ITARGETSR_FIELD_BITS;
	target &= ~(ITARGETSR_FIELD_MASK << target_shift);
	target |= cpu_mask << target_shift;
	__raw_writel(target, gd->gicd_base + GICD_ITARGETSR(it / NUM_TARGETS_PER_REG));
}

static void gic_it_set_prio(struct gic_data *gd, uint32_t it, uint8_t prio)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);

	/* Set prio it to selected CPUs */
	DMSG("prio: writing 0x%x to 0x%x",
		prio, gd->gicd_base + GICD_IPRIORITYR(0) + it);
	__raw_writeb(prio, gd->gicd_base + GICD_IPRIORITYR(0) + it);
}

static void gic_it_enable(struct gic_data *gd, uint32_t it)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);

	if (it >= NUM_SGI) {
		/*
		 * Not enabled yet, except Software Generated Interrupt
		 * which is implementation defined
		 */
		assert(!(__raw_readl(gd->gicd_base + GICD_ISENABLER(idx)) & mask));
	}

	/* Enable the interrupt */
	__raw_writel(mask, gd->gicd_base + GICD_ISENABLER(idx));
}

static void gic_it_disable(struct gic_data *gd, uint32_t it)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);

	/* Disable the interrupt */
	__raw_writel(mask, gd->gicd_base + GICD_ICENABLER(idx));
}

static void gic_it_set_pending(struct gic_data *gd, uint32_t it)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = BIT32(it % NUM_INTS_PER_REG);

	/* Should be Peripheral Interrupt */
	assert(it >= NUM_SGI);

	/* Raise the interrupt */
	__raw_writel(mask, gd->gicd_base + GICD_ISPENDR(idx));
}

static void gic_it_raise_sgi(struct gic_data *gd, uint32_t it,
		uint8_t cpu_mask, uint8_t group)
{
	uint32_t mask_id = it & 0xf;
	uint32_t mask_group = group & 0x1;
	uint32_t mask_cpu = cpu_mask & 0xff;
	uint32_t mask = (mask_id | SHIFT_U32(mask_group, 15) | SHIFT_U32(mask_cpu, 16));

	/* Should be Software Generated Interrupt */
	assert(it < NUM_SGI);

	/* Raise the interrupt */
	__raw_writel(mask, gd->gicd_base + GICD_SGIR);
}

static uint32_t gic_read_iar(struct gic_data *gd)
{
#if defined(CFG_ARM_GICV3)
	return read_icc_iar0();
#else
	return __raw_readl(gd->gicc_base + GICC_IAR);
#endif
}

static uint32_t gic_read_aiar(struct gic_data *gd)
{
#if defined(CFG_ARM_GICV3)
	return read_icc_aiar0();
#else
	return __raw_readl(gd->gicc_base + GICC_AIAR);
#endif
}

static void gic_write_eoir(struct gic_data *gd, uint32_t eoir)
{
#if defined(CFG_ARM_GICV3)
	write_icc_eoir0(eoir);
#else
	__raw_writel(eoir, gd->gicc_base + GICC_EOIR);
#endif
}

static void gic_write_aeoir(struct gic_data *gd, uint32_t aeoir)
{
#if defined(CFG_ARM_GICV3)
	write_icc_aeoir0(aeoir);
#else
	__raw_writel(aeoir, gd->gicc_base + GICC_AEOIR);
#endif
}

static bool gic_it_is_enabled(struct gic_data *gd, uint32_t it)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);
	return !!(__raw_readl(gd->gicd_base + GICD_ISENABLER(idx)) & mask);
}

static bool gic_it_get_group(struct gic_data *gd, uint32_t it)
{
	uint32_t idx = it / NUM_INTS_PER_REG;
	uint32_t mask = 1 << (it % NUM_INTS_PER_REG);
	return !!(__raw_readl(gd->gicd_base + GICD_IGROUPR(idx)) & mask);
}

static uint32_t gic_it_get_target(struct gic_data *gd, uint32_t it)
{
	uint32_t reg_idx = it / NUM_TARGETS_PER_REG;
	uint32_t target_shift = (it % NUM_TARGETS_PER_REG) *
				ITARGETSR_FIELD_BITS;
	uint32_t target_mask = ITARGETSR_FIELD_MASK << target_shift;
	uint32_t target =
		__raw_readl(gd->gicd_base + GICD_ITARGETSR(reg_idx)) & target_mask;

	target = target >> target_shift;
	return target;
}

static struct itr_handler *find_handler(uint32_t it)
{
	struct itr_handler *h;

        SLIST_FOREACH(h, &handlers, link)
		if (h->it == it)
		return h;

	return NULL;
}

static void git_it_handle(uint32_t it)
{
	struct gic_data *gd = &global_gd;
	struct itr_handler *h = find_handler(it);
	uint32_t vector = it;

	if (!h) {
		printf("error : not found interrupt %d, and disable it!\n", it);
		gic_it_disable(gd, it);
		return;
	}

#ifdef INTC_NUM_MODFIY
	vector -= INTC_NUM;
#endif
	if ((h->handler)(vector, h->data) != HANDLED) {
		printf("error : interrupt %d handle error, and disable it!\n", it);
		gic_it_disable(gd, it);
	}
}

void gic_op_init(uint32_t gic_base)
{
	uint32_t n;
	struct gic_data *gd = &global_gd;

	memset(&global_gd, 0, sizeof(struct gic_data));

	gic_init_base_addr(gd, gic_base);

	for (n = 0; n <= gd->max_it / NUM_INTS_PER_REG; n++) {

		/* Disable interrupts */
		__raw_writel(0xffffffff, gd->gicd_base + GICD_ICENABLER(n));

		/* Make interrupts non-pending */
		__raw_writel(0xffffffff, gd->gicd_base + GICD_ICPENDR(n));
	}

	/* Set the priority mask to permit Non-secure interrupts, and to
	 * allow the Non-secure world to adjust the priority mask itself
	 */
#if defined(CFG_ARM_GICV3)
	write_icc_pmr(0x80);
	write_icc_ctlr(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 |
		       GICC_CTLR_FIQEN);
#else
	__raw_writel(0x80, gd->gicc_base + GICC_PMR);

	/* Enable GIC */
	__raw_writel(GICC_CTLR_ENABLEGRP0, gd->gicc_base + GICC_CTLR);
#endif
	__raw_writel(__raw_readl(gd->gicd_base + GICD_CTLR) | GICD_CTLR_ENABLEGRP0, gd->gicd_base + GICD_CTLR);
}

void gic_op_dump_state(struct gic_data *gd)
{
	int i;

#if defined(CFG_ARM_GICV3)
	printf("GICC_CTLR: 0x%x", read_icc_ctlr());
#else
	printf("GICC_CTLR: 0x%x", __raw_readl(gd->gicc_base + GICC_CTLR));
#endif
	printf("GICD_CTLR: 0x%x", __raw_readl(gd->gicd_base + GICD_CTLR));

	for (i = 0; i < (int)gd->max_it; i++) {
		if (gic_it_is_enabled(gd, i)) {
			printf("irq%d: enabled, group:%d, target:0x%x", i,
			     gic_it_get_group(gd, i), gic_it_get_target(gd, i));
		}
	}
}

void gic_op_irq_handle(void)
{
	uint32_t aiar;
	uint32_t id;
	struct gic_data *gd = &global_gd;

	aiar = gic_read_iar(gd);
	id = aiar & GICC_IAR_IT_ID_MASK;

	if (id < gd->max_it)
		git_it_handle(id);
	else
		printf("ignoring interrupt %d", id);

	gic_write_eoir(gd, aiar);
}

void gic_op_fiq_handle(void)
{
	uint32_t iar;
	uint32_t id;
	struct gic_data *gd = &global_gd;

	iar = gic_read_iar(gd);
	id = iar & GICC_IAR_IT_ID_MASK;

	if (id < gd->max_it)
		git_it_handle(id);
	else
		printf("ignoring interrupt %d", id);

	gic_write_eoir(gd, iar);
}

int gic_op_request(uint32_t interrupt, enum interrupt_type type,
		interrupt_handler_t handler, void *data)
{
	int ret = -1;
	struct gic_data *gd = &global_gd;
	struct itr_handler *h = malloc(sizeof(struct itr_handler));
	if (!h) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		goto exit;
	}

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	h->it = interrupt;
	h->handler = handler;
	h->data = data;
	h->type = type;

	if (h->it >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		goto exit;
	}

	gic_it_add(gd, h->it, type);
	/* Set the CPU mask to deliver interrupts to any online core */
	gic_it_set_cpu_mask(gd, h->it, 0xff);
	gic_it_set_prio(gd, h->it, 0x1);

	SLIST_INSERT_HEAD(&handlers, h, link);

	ret = 0;
exit:
	if ((ret < 0) && (h)) {
		free(h);
		h = NULL;
	}
	return ret;
}

void gic_op_free(uint32_t interrupt)
{
	struct gic_data *gd = &global_gd;
	struct itr_handler *h = NULL;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	h = find_handler(interrupt);
	if (!h)
		return;

	SLIST_REMOVE(&handlers, h, itr_handler, link);

	gic_it_disable(gd, interrupt);
	free(h);
	h = NULL;
}

void gic_op_enable(uint32_t interrupt)
{
	struct gic_data *gd = &global_gd;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	if (interrupt >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		return;
	}

	gic_it_enable(gd, interrupt);
}

void gic_op_disable(uint32_t interrupt)
{
	struct gic_data *gd = &global_gd;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	if (interrupt >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		return;
	}

	gic_it_disable(gd, interrupt);
}

void gic_op_all_disable(void)
{
	uint32_t n = 0;
	struct gic_data *gd = &global_gd;

	for (n = 0; n <= gd->max_it / NUM_INTS_PER_REG; n++) {
		/* Disable interrupts */
		__raw_writel(0xffffffff, gd->gicd_base + GICD_ICENABLER(n));
		/* Make interrupts non-pending */
		__raw_writel(0xffffffff, gd->gicd_base + GICD_ICPENDR(n));
	}
#if defined(CFG_ARM_GICV3)
	write_icc_pmr(0x0);
	write_icc_ctlr(0x0);
#else
	__raw_writel(0x0, gd->gicc_base + GICC_PMR);
	__raw_writel(0x0, gd->gicc_base + GICC_CTLR);
#endif
	__raw_writel(0x0, gd->gicd_base + GICD_CTLR);
}

static void gic_op_raise_pi(uint32_t interrupt)
{
	struct gic_data *gd = &global_gd;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	if (interrupt >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		return;
	}

	gic_it_set_pending(gd, interrupt);
}

static void gic_op_raise_sgi(uint32_t interrupt, uint8_t cpu_mask)
{
	struct gic_data *gd = &global_gd;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	if (interrupt >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		return;
	}

	if (interrupt < NUM_NS_SGI)
		gic_it_raise_sgi(gd, interrupt, cpu_mask, 1);
	else
		gic_it_raise_sgi(gd, interrupt, cpu_mask, 0);
}

static void gic_op_set_affinity(uint32_t interrupt, uint8_t cpu_mask)
{
	struct gic_data *gd = &global_gd;

#ifdef INTC_NUM_MODFIY
	interrupt += INTC_NUM;
#endif
	if (interrupt >= gd->max_it) {
		printf("error: %s %s %d\n", __FILE__, __func__, __LINE__);
		return;
	}

	gic_it_set_cpu_mask(gd, interrupt, cpu_mask);
}
