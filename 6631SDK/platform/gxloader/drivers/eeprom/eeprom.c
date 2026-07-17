#include "stdio.h"
#include "string.h"
#include "linux/errno.h"
#include "sys/types.h"
#include "i2c.h"
#include "eeprom.h"

/*
 * As seen through Linux I2C, differences between the most common types of I2C
 * memory include:
 * - How much memory is available (usually specified in bit)?
 * - What write page size does it support?
 * - Special flags (16 bit addresses, read_only, world readable...)?
 *
 * If you set up a custom eeprom type, please double-check the parameters.
 * Especially page_size needs extra care, as you risk data loss if your value
 * is bigger than what the chip actually supports!
 */

struct at24_platform_data {
	unsigned int byte_len;  /* size (sum of all addr) */
	unsigned short page_size;  /* for writes */
	unsigned char flags;
#define AT24_FLAG_ADDR16        0x80    /* address pointer is 16 bit */
#define AT24_FLAG_READONLY      0x40    /* sysfs-entry will be read-only */
#define AT24_FLAG_IRUGO         0x20    /* sysfs-entry will be world-readable */
#define AT24_FLAG_TAKE8ADDR     0x10    /* take always 8 addresses (24c00) */
};

/*
 * I2C EEPROMs from most vendors are inexpensive and mostly interchangeable.
 * Differences between different vendor product lines (like Atmel AT24C or
 * MicroChip 24LC, etc) won't much matter for typical read/write access.
 * There are also I2C RAM chips, likewise interchangeable. One example
 * would be the PCF8570, which acts like a 24c02 EEPROM (256 bytes).
 *
 * However, misconfiguration can lose data. "Set 16-bit memory address"
 * to a part with 8-bit addressing will overwrite data. Writing with too
 * big a page size also loses data. And it's not safe to assume that the
 * conventional addresses 0x50..0x57 only hold eeproms; a PCF8563 RTC
 * uses 0x51, for just one example.
 *
 * Accordingly, explicit board-specific configuration data should be used
 * in almost all cases. (One partial exception is an SMBus used to access
 * "SPD" data for DRAM sticks. Those only use 24c02 EEPROMs.)
 *
 * So this driver uses "new style" I2C driver binding, expecting to be
 * told what devices exist. That may be in arch/X/mach-Y/board-Z.c or
 * similar kernel-resident tables; or, configuration data coming from
 * a bootloader.
 *
 * Other than binding model, current differences from "eeprom" driver are
 * that this one handles write access and isn't restricted to 24c02 devices.
 * It also handles larger devices (32 kbit and up) with two-byte addresses,
 * which won't work on pure SMBus systems.
 */

struct at24_data {
	struct at24_platform_data chip;

	u8 *writebuf;
	unsigned write_max;
	unsigned num_addresses;
	unsigned char chip_addr;
	void *dev;
};

static struct at24_data g_at24;
/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
static unsigned io_limit = 128;  //Maximum bytes per I/O (default 128)

/*
 * Specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned write_timeout = 25;  //Time (in ms) to try writes (default 25)"

/* i2c */
struct i2c_device_id {
	char *name;
	unsigned int byte_len;   /* size (sum of all addr) */
	unsigned short page_size;  /* for writes */
	unsigned char flags;
};

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))

static const struct i2c_device_id at24_ids[] = {
	/* needs 8 addresses as A0-A2 are ignored */
	{ "24c00",   128 / 8,     1,   AT24_FLAG_TAKE8ADDR },
	/* old variants can't be handled with this generic entry! */
	{ "24c01",   1024 / 8,    4,   0 },
	{ "24c02",   2048 / 8,    8,   0 },
	/* spd is a 24c02 in memory DIMMs */
	{ "spd",     2048 / 8,    8,   AT24_FLAG_READONLY | AT24_FLAG_IRUGO },
	{ "24c04",   4096 / 8,    16,   0 },
	/* 24rf08 quirk is handled at i2c-core */
	{ "24c08",   8192 / 8,    16,  0 },
	{ "24c16",   16384 / 8,   16,  0 },
	{ "24c32",   32768 / 8,   32,  AT24_FLAG_ADDR16 },
	{ "24c64",   65536 / 8,   32,  AT24_FLAG_ADDR16 },
	{ "24c128",  131072 / 8,  64,  AT24_FLAG_ADDR16 },
	{ "24c256",  262144 / 8,  64,  AT24_FLAG_ADDR16 },
	{ "24c512",  524288 / 8,  128, AT24_FLAG_ADDR16 },
	{ "24c1024", 1048576 / 8, 256, AT24_FLAG_ADDR16 },
	{ NULL,      0,           0,   0 }  /* END OF LIST */
};

/*-------------------------------------------------------------------------*/

/*
 * This routine supports chips which consume multiple I2C addresses. It
 * computes the addressing information to be used for a given r/w request.
 * Assumes that sanity checks for offset happened at sysfs-layer.
 */
static unsigned char at24_translate_offset(struct at24_data *at24,
		unsigned int *offset)
{
	unsigned int i;

	if (at24->chip.flags & AT24_FLAG_ADDR16) {
		i = *offset >> 16;
		*offset &= 0xffff;
	} else {
		i = *offset >> 8;
		*offset &= 0xff;
	}

	return at24->chip_addr+i;
}
static ssize_t at24_eeprom_read(struct at24_data *at24, unsigned char *buf,
		unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	unsigned char dummy_addr;
	int status, i;

	memset(msg, 0, sizeof(msg));

	/*
	 * REVISIT some multi-address chips don't rollover page reads to
	 * the next slave address, so we may need to truncate the count.
	 * Those chips might need another quirk flag.
	 *
	 * If the real hardware used four adjacent 24c02 chips and that
	 * were misconfigured as one 24c08, that would be a similar effect:
	 * one "eeprom" file not four, but larger reads would fail when
	 * they crossed certain pages.
	 */

	/*
	 * Slave address and byte offset derive from the offset. Always
	 * set the byte address; on a multi-master board, another master
	 * may have changed the chip's "current" address pointer.
	 */
	dummy_addr = at24_translate_offset(at24, &offset);

	if (count > io_limit)
		count = io_limit;

	/*
	 * When we have a better choice than SMBus calls, use a combined
	 * I2C message. Write address; then read up to io_limit data bytes.
	 * Note that read page rollover helps us here (unlike writes).
	 * msgbuf is u8 and will cast to our needs.
	 */
	i = 0;
	if (at24->chip.flags & AT24_FLAG_ADDR16)
		msgbuf[i++] = offset >> 8;
	msgbuf[i++] = offset;

	msg[0].addr = dummy_addr;
	msg[0].buf = msgbuf;
	msg[0].flags = I2C_M_REV_DIR_ADDR;
	msg[0].len = i;

	msg[1].addr = dummy_addr;
	msg[1].flags = I2C_M_RD | I2C_M_NO_RD_ACK;
	msg[1].buf = buf;
	msg[1].len = count;

	status = gx_i2c_transfer(at24->dev, msg, 2);

	if (status == 0)
		return count;
	else if (status >= 0)
		return -EIO;
	else
		return status;
}

static ssize_t at24_bin_read(unsigned char *buf, unsigned int off, size_t count)
{
	ssize_t retval = 0;

	if (!count)
		return count;

	while (count) {
		ssize_t	status;

		status = at24_eeprom_read(&g_at24, buf, off, count);
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	return retval;
}
extern void mdelay(unsigned int msec);
/*
 * REVISIT: export at24_bin{read,write}() to let other kernel code use
 * eeprom data. For example, it might hold a board's Ethernet address, or
 * board-specific calibration data generated on the manufacturing floor.
 */


/*
 * Note that if the hardware write-protect pin is pulled high, the whole
 * chip is normally write protected. But there are plenty of product
 * variants here, including OTP fuses and partial chip protect.
 *
 * We only use page mode writes; the alternative is sloooow. This routine
 * writes at most one page.
 */
static ssize_t at24_eeprom_write(struct at24_data *at24, unsigned char *buf,
		unsigned int offset, size_t count)
{
	unsigned char dummy_addr;
	struct i2c_msg msg;
	ssize_t status;
	unsigned long timeout;
	unsigned next_page;

	/* Get corresponding I2C address and adjust offset */
	dummy_addr = at24_translate_offset(at24, &offset);

	/* write_max is at most a page */
	if (count > at24->write_max)
		count = at24->write_max;

	/* Never roll over backwards, to the start of this page */
	next_page = roundup(offset + 1, at24->chip.page_size);
	if (offset + count > next_page)
		count = next_page - offset;

	/* If we'll use I2C calls for I/O, set up the message */
	{
		int i = 0;

		msg.addr = dummy_addr;
		msg.flags = 0;

		/* msg.buf is u8 and casts will mask the values */
		msg.buf = at24->writebuf;
		if (at24->chip.flags & AT24_FLAG_ADDR16)
			msg.buf[i++] = offset >> 8;

		msg.buf[i++] = offset;
		memcpy(&msg.buf[i], buf, count);
		msg.len = i + count;
	}

	/*
	 * Writes fail if the previous one didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	for (timeout=0; timeout<write_timeout; timeout++) {
		status = gx_i2c_transfer(at24->dev, &msg, 1);
		if (status == 0)
			status = count;

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		mdelay(1);
	}
	return -ETIMEDOUT;
}

static ssize_t at24_bin_write(unsigned char *buf, unsigned int off, size_t count)
{
	ssize_t retval = 0;

	if (!count)
		return count;

	while (count) {
		ssize_t	status;

		status = at24_eeprom_write(&g_at24, buf, off, count);
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	return retval;
}

/*-------------------------------------------------------------------------*/

static int at24_probe(unsigned int bus_id, unsigned char chip_addr, char *eeprom_name)
{
	struct at24_platform_data chip;
	int err;
	unsigned i, num_addresses;

	if (eeprom_name == NULL) {
		err = -ENODEV;
		goto err_out;
	}
	
	for(i = 0; at24_ids[i].name; i++) {
		if(strcmp(at24_ids[i].name, eeprom_name) == 0) {

			chip.byte_len = at24_ids[i].byte_len;
			chip.flags = at24_ids[i].flags;
			/*
			 * This is slow, but we can't know all eeproms, so we better
			 * play safe. Specifying custom eeprom-types via platform_data
			 * is recommended anyhow.
			 */
			chip.page_size = at24_ids[i].page_size;
			break;
		}
	}
	if (!at24_ids[i].name) {
		err = -ENOMEM;
		goto err_out;
	}

	if (chip.flags & AT24_FLAG_TAKE8ADDR)
		num_addresses = 8;
	else
		num_addresses = DIV_ROUND_UP(chip.byte_len,
			(chip.flags & AT24_FLAG_ADDR16) ? 65536 : 256);

	g_at24.chip = chip;
	g_at24.num_addresses = num_addresses;
	if (chip.page_size > io_limit)
		g_at24.write_max = io_limit;
	else
		g_at24.write_max = chip.page_size;

	/* buffer (data + address at the beginning) */
	g_at24.writebuf = malloc(g_at24.write_max + 2);
	if (!g_at24.writebuf) {
		err = -ENOMEM;
		goto err_out;
	}
	g_at24.chip_addr = chip_addr;

	g_at24.dev = gx_i2c_open(bus_id);
	if (g_at24.dev == NULL) {
		err = -ENOMEM;
		goto err_out;
	}

	return 0;

err_out:
	printf("probe error %d\n", err);
	return err;
}

int eeprom_init(unsigned int bus_id, unsigned char chip_addr, char *eeprom_name)
{
	return at24_probe(bus_id, chip_addr, eeprom_name);
}

ssize_t eeprom_write(unsigned char *buf, unsigned int off, size_t count)
{
	return at24_bin_write(buf, off, count);
}

ssize_t eeprom_read(unsigned char *buf, unsigned int off, size_t count)
{
	return at24_bin_read(buf, off, count);
}


