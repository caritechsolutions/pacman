#include "fat.h"
#include "usb.h"
#include "stdio.h"
#include "flash.h"

#define UPDATE_FILE "/recovery.rcv"
#define BIN_LENGTH (0x800000)
#define BLOCK_SIZE (0x10000)

#define USB_STATE_BASE_ADDR  (0xa0900050)
#define USB_STATE_0 (*(volatile unsigned int *)(USB_STATE_BASE_ADDR))
#define USB_STATE_1 (*(volatile unsigned int *)(USB_STATE_BASE_ADDR + 0x04))
#define USB_STATE_2 (*(volatile unsigned int *)(USB_STATE_BASE_ADDR + 0x08))
#define USB_STATE_3 (*(volatile unsigned int *)(USB_STATE_BASE_ADDR + 0x0c))

#define WDT_BASE_ADDR                                           (0xa020B000)
#define rWDT_CTRL            (*(volatile unsigned int *)(WDT_BASE_ADDR))
#define rWDT_MATCH           (*(volatile unsigned int *)(WDT_BASE_ADDR + 0x04))

#define XTAL_CLOCK_FREQ                   27000000
/* Init and enable WDT */
#define WDT_INIT(SysClk)                                              \
    do{                                                               \
        rWDT_MATCH = ((SysClk/1000000 - 1) << 16)|(0x10000 - 10000);  \
        rWDT_CTRL = 3;                                                \
    }while(0)

#define MAX_PARTITON (10)

int au_usb_stor_curr_dev = -1; /* current device */

static void flash_update(unsigned int flash_addr, const unsigned char *buf, unsigned int flash_len)
{
	//app_log_debug("Erase flash address: 0x%x, len: %d\n", flash_addr, flash_len);
	gxflash_erasedata(flash_addr, flash_len);
	//app_log_debug("Write to flash address: 0x%x, len: %d\n", flash_addr, flash_len);
	gxflash_pageprogram(flash_addr, (uint8_t*)buf, flash_len);
}

int usb_recover_proc(void)
{
    unsigned char *pbuf = NULL;
	block_dev_desc_t *stor_dev;
	int res = 0;
    unsigned int block_num = 0;
    unsigned int i = 0;
    unsigned int partition = 0;
    volatile unsigned int delay_cnt = 0;

    //must delay before init
    while(delay_cnt++ < 0x7000000);

	/* start USB */
	if (usb_stop() < 0) {
		app_log_error("usb_stop failed\n");
		return -1;
	}

	if (usb_init() < 0) {
		app_log_error("usb_init failed\n");
		return -1;
	}
	/*
	 * check whether a storage device is attached (assume that it's
	 * a USB memory stick, since nothing else should be attached).
	 */
	au_usb_stor_curr_dev = usb_stor_scan(0);
	if (au_usb_stor_curr_dev == -1) {
		//app_log_min("No device found. Not initialized?\n");
		res = -1;
		goto xit;
	}
	/* check whether it has a partition table */
	stor_dev = usb_stor_get_dev(0);
	if (stor_dev == NULL) {
		app_log_min("uknown device type\n");
		res = -1;
		goto xit;
	}

    if((pbuf = (unsigned char*)malloc(BIN_LENGTH)) == NULL)
    {
        app_log_error("malloc failed!\n");
        res = -1;
        goto xit;
    }

    for(partition = 1; partition <= MAX_PARTITON; partition++)
    {
        if (fat_register_device(stor_dev, partition) == 0)
        {
            if((file_fat_read(UPDATE_FILE, pbuf, BIN_LENGTH)) != -1)
            {
                break;
            }
        }
    }

    if(partition > MAX_PARTITON)
    {
        res = -1;
        goto xit;
    }

    block_num = BIN_LENGTH / BLOCK_SIZE;
    if((BIN_LENGTH % BLOCK_SIZE) > 0)
        block_num++;

    if(block_num > 2)
    {
        unsigned int usb_a = (USB_STATE_1 & 0x01);
        unsigned int usb_b = (USB_STATE_3 & 0x01);

        app_log_min("____________usb update____________\n");

        //app_log_debug("[%s]%d usb_a = %d, usb_b = %d \n", __func__, __LINE__, usb_a, usb_b);

        block_num -= 2;
        for(i = 0; i < block_num; i++)
        {
            flash_update((i+2) * BLOCK_SIZE, pbuf + ((i+2) * BLOCK_SIZE), BLOCK_SIZE);
            //app_log_debug("[%s]%d update proc %d%%\n", __func__, __LINE__, i*100/block_num);
            _fd650_show_num(i*100/block_num);
        }
        _fd650_show_num(100);

        usb_stop();
        while(1)
        {
            //if (usb_stor_scan(0) == -1)
            if(((USB_STATE_1 & 0x01) != usb_a) || ((USB_STATE_3 & 0x01) != usb_b))
            {
                //app_log_info("USB Device Plug!!!!!\n");
                WDT_INIT(XTAL_CLOCK_FREQ);
                while(1)
                {
                }
            }
            else
            {
                //app_log_info("USB device Wait!!!!!\n");
            }
        }
    }
 xit:
	return res;
}

