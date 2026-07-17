#include <spi.h>
#include <linux/errno.h>

static struct list_head g_spi_head;
static struct list_head g_sflash_head;

void device_list_init(void)
{
	INIT_LIST_HEAD(&g_spi_head);
	INIT_LIST_HEAD(&g_sflash_head);
}

static void spi_scan_boardinfo(struct spi_master *master)
{
	struct spi_device *spi_dev;
	struct list_head *pos;
	struct sflash_master *sflash;
	list_for_each(pos,&g_sflash_head)
	{
		sflash=list_entry(pos,struct sflash_master ,list);
		if(sflash->bus_num == master->bus_num)
		{
			spi_dev=&(sflash->spi);
			if(spi_dev->chip_select >= master->num_chipselect)
				continue;
			spi_dev->master=master;
			break;
		}
	}
}

static void sflash_scan_boardinfo(struct sflash_master *master)
{
	struct spi_device *spi_dev;
	struct list_head *pos;
	struct spi_master *spi;
	list_for_each(pos,&g_spi_head)
	{
		spi=list_entry(pos,struct spi_master ,list);
		if(spi->bus_num == master->bus_num)
		{
			spi_dev=&(master->spi);
			if(spi_dev->chip_select >= spi->num_chipselect)
				continue;
			spi_dev->master=spi;
			break;
		}

	}
}

int spi_register_master(struct spi_master *master)
{
	int   status = -ENODEV;

	if (!master)
		return -ENODEV;

	/* even if it's just one always-selected device, there must
	 *      * be at least one chipselect
	 *           */
	if (master->num_chipselect == 0)
		return -EINVAL;

	/* convention:  dynamically assigned bus IDs count down from the max */
	if (master->bus_num < 0) {
		return -EINVAL;
	}

	/* register the device, then userspace will see it.
	 *      * registration fails if the bus ID is in use.
	 *           */
	//printf("registered spi master bus_num=%d\n", master->bus_num);
	list_add_tail(&(master->list),&g_spi_head);

	spi_scan_boardinfo(master);
	status = 0; 
	return status;
}

int spi_flash_register_master(struct sflash_master *master)
{

	if (!master)
		return -ENODEV;

	/* even if it's just one always-selected device, there must
	 *      * be at least one chipselect
	 *           */
	if (master->spi.chip_select < 0)
		return -EINVAL;

	/* convention:  dynamically assigned bus IDs count down from the max */
	if (master->bus_num < 0) {
		return -EINVAL;
	}

	/* register the device, then userspace will see it.
	 *      * registration fails if the bus ID is in use.
	 *           */
	//printf("registered spi flash master bus_num=%d\n", master->bus_num);
	list_add_tail(&(master->list),&g_sflash_head);

	sflash_scan_boardinfo(master);
	return 0;
}
