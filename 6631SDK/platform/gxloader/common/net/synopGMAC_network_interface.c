#include "synopGMAC_plat.h"
#include "synopGMAC_network_interface.h"
#include "synopGMAC_Dev.h"

#include "sys/types.h"
#include "net_priv.h"
#include "linux/dma-mapping.h"
#include "linux/errno.h"

#include <net.h>

#define SIOCDEVPRIVATE 0x89F0
#define IOCTL_READ_REGISTER  SIOCDEVPRIVATE+1
#define IOCTL_WRITE_REGISTER SIOCDEVPRIVATE+2
#define IOCTL_READ_IPSTRUCT  SIOCDEVPRIVATE+3
#define IOCTL_READ_RXDESC    SIOCDEVPRIVATE+4
#define IOCTL_READ_TXDESC    SIOCDEVPRIVATE+5
#define IOCTL_POWER_DOWN     SIOCDEVPRIVATE+6

//static struct timer_list synopGMAC_cable_unplug_timer;
static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not
static int tx_desc = TRANSMIT_DESC_SIZE;
static int rx_desc = RECEIVE_DESC_SIZE;
static int rmii_flag = 3;

/*These are the global pointers for their respecive structures*/
extern synopGMACPciNetworkAdapter * synopGMACadapter;
extern struct net_dev             * synopGMACnetdev;

extern u8 gx_mac_addr[6];
extern u8* synopGMACMappedAddr;
extern u32 synopGMACMappedAddrSize;

extern void flush_dcache(void);

/**
 * Function used to detect the cable plugging and unplugging.
 * This function gets scheduled once in every second and polls
 * the PHY register for network cable plug/unplug. Once the
 * connection is back the GMAC device is configured as per
 * new Duplex mode and Speed of the connection.
 * @param[in] u32 type but is not used currently.
 * \return returns void.
 * \note This function is tightly coupled with Linux 2.6.xx.
 * \callgraph
 */

static void synopGMAC_linux_cable_fes_function(synopGMACdevice * gmacdev)
{
	u16 data;

	synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, PHY_CONTROL_REG, &data);

	if(data & Mii_phy_status_speed)
		gmacdev->Speed = SPEED100;
	else
		gmacdev->Speed = SPEED10;

	if(gmacdev->Speed == SPEED100)
		TR("Link is with 100M Speed \n");
	if(gmacdev->Speed == SPEED10)
		TR("Link is with 10M Speed \n");

	synopGMAC_setup_fes(gmacdev);
}

static void synopGMAC_linux_powerup_mac(synopGMACdevice *gmacdev)
{
	GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
	if( synopGMAC_is_magic_packet_received(gmacdev))
		TR("GMAC wokeup due to Magic Pkt Received\n");
	if(synopGMAC_is_wakeup_frame_received(gmacdev))
		TR("GMAC wokeup due to Wakeup Frame Received\n");
	//Disable the assertion of PMT interrupt
	synopGMAC_pmt_int_disable(gmacdev);
	//Enable the mac and Dma rx and tx paths
	synopGMAC_rx_enable(gmacdev);
	synopGMAC_enable_dma_rx(gmacdev);

	synopGMAC_tx_enable(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);
}

/**
 * This sets up the transmit Descriptor queue in ring or chain mode.
 * This function is tightly coupled to the platform and operating system
 * Device is interested only after the descriptors are setup. Therefore this function
 * is not included in the device driver API. This function should be treated as an
 * example code to design the descriptor structures for ring mode or chain mode.
 * This function depends on the pcidev structure for allocation consistent dma-able memory in case of linux.
 * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
 *	- Allocates the memory for the descriptors.
 *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
 *	- Initialize the Busy and Next descriptors to first descriptor address.
 *	- Initialize the last descriptor with the endof ring in case of ring mode.
 *	- Initialize the descriptors in chain mode.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] pointer to pci_device structure.
 * @param[in] number of descriptor expected in tx descriptor queue.
 * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
 * \return 0 upon success. Error code upon failure.
 * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
 *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
 *  user should for gmacdev->TxDescCount to see how many descriptors are there in the chain. Should continue further
 *  only if the number of descriptors in the chain meets the requirements
 */

s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice * gmacdev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;

	DmaDesc *first_desc = NULL;
	DmaDesc *second_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;

	if(desc_mode == RINGMODE){
		TR("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
		first_desc = plat_alloc_consistent_dmaable_memory (sizeof(DmaDesc) * no_of_desc,&dma_addr);
		if(first_desc == NULL){
			TR("Error in Tx Descriptors memory allocation\n");
			return -ESYNOPGMACNOMEM;
		}
		gmacdev->TxDescCount = no_of_desc;
		gmacdev->TxDesc      = first_desc;
		gmacdev->TxDescDma   = dma_addr;

		for(i =0; i < gmacdev -> TxDescCount; i++){
			synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
			TR("%02d %08x %08x\n",i, (unsigned int)(gmacdev->TxDesc + i), dma_addr + i*sizeof(DmaDesc));
		}
	}
	else{
		//Allocate the head descriptor
		first_desc = plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc),&dma_addr);
		if(first_desc == NULL){
			TR("Error in Tx Descriptor Memory allocation in Ring mode\n");
			return -ESYNOPGMACNOMEM;
		}
		gmacdev->TxDesc       = first_desc;
		gmacdev->TxDescDma    = dma_addr;

		TR("Tx===================================================================Tx\n");
		first_desc->buffer2   = gmacdev->TxDescDma;
		first_desc->data2     = (u32)gmacdev->TxDesc;

		gmacdev->TxDescCount = 1;

		for(i =0; i <(no_of_desc-1); i++){
			second_desc = plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc),&dma_addr);
			if(second_desc == NULL){
				TR("Error in Tx Descriptor Memory allocation in Chain mode\n");
				return -ESYNOPGMACNOMEM;
			}
			first_desc->buffer2  = dma_addr;
			first_desc->data2    = (u32)second_desc;

			second_desc->buffer2 = gmacdev->TxDescDma;
			second_desc->data2   = (u32)gmacdev->TxDesc;

			synopGMAC_tx_desc_init_chain(first_desc);
			TR("%02d %08x %08x %08x %08x %08x %08x %08x \n",gmacdev->TxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1,first_desc->buffer2, first_desc->data1, first_desc->data2);
			gmacdev->TxDescCount += 1;
			first_desc = second_desc;
		}

		synopGMAC_tx_desc_init_chain(first_desc);
		TR("%02d %08x %08x %08x %08x %08x %08x %08x \n",gmacdev->TxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1,first_desc->buffer2, first_desc->data1, first_desc->data2);
		TR("Tx===================================================================Tx\n");
	}

	flush_dcache();

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc  = 0;

	return -ESYNOPGMACNOERR;
}


/**
 * This sets up the receive Descriptor queue in ring or chain mode.
 * This function is tightly coupled to the platform and operating system
 * Device is interested only after the descriptors are setup. Therefore this function
 * is not included in the device driver API. This function should be treated as an
 * example code to design the descriptor structures in ring mode or chain mode.
 * This function depends on the pcidev structure for allocation of consistent dma-able memory in case of linux.
 * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
 *	- Allocates the memory for the descriptors.
 *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
 *	- Initialize the Busy and Next descriptors to first descriptor address.
 *	- Initialize the last descriptor with the endof ring in case of ring mode.
 *	- Initialize the descriptors in chain mode.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] pointer to pci_device structure.
 * @param[in] number of descriptor expected in rx descriptor queue.
 * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
 * \return 0 upon success. Error code upon failure.
 * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
 *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
 *  user should for gmacdev->RxDescCount to see how many descriptors are there in the chain. Should continue further
 *  only if the number of descriptors in the chain meets the requirements
 */
s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice * gmacdev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;

	DmaDesc *first_desc = NULL;
	DmaDesc *second_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;

	if(desc_mode == RINGMODE){
		TR("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
		first_desc = plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc) * no_of_desc, &dma_addr);
		if(first_desc == NULL){
			TR("Error in Rx Descriptor Memory allocation in Ring mode\n");
			return -ESYNOPGMACNOMEM;
		}
		gmacdev->RxDescCount = no_of_desc;
		gmacdev->RxDesc      = first_desc;
		gmacdev->RxDescDma   = dma_addr;

		for(i =0; i < gmacdev -> RxDescCount; i++){
			synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
			TR("%02d %08x %08x\n",i, (unsigned int)(gmacdev->RxDesc + i), dma_addr + i*sizeof(DmaDesc));
		}
	}
	else{
		//Allocate the head descriptor
		first_desc = plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc),&dma_addr);
		if(first_desc == NULL){
			TR("Error in Rx Descriptor Memory allocation in Ring mode\n");
			return -ESYNOPGMACNOMEM;
		}
		gmacdev->RxDesc       = first_desc;
		gmacdev->RxDescDma    = dma_addr;

		TR("Rx===================================================================Rx\n");
		first_desc->buffer2   = gmacdev->RxDescDma;
		first_desc->data2     = (u32) gmacdev->RxDesc;

		gmacdev->RxDescCount = 1;

		for(i =0; i < (no_of_desc-1); i++){
			second_desc = plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc),&dma_addr);
			if(second_desc == NULL){
				TR("Error in Rx Descriptor Memory allocation in Chain mode\n");
				return -ESYNOPGMACNOMEM;
			}
			first_desc->buffer2  = dma_addr;
			first_desc->data2    = (u32)second_desc;

			second_desc->buffer2 = gmacdev->RxDescDma;
			second_desc->data2   = (u32)gmacdev->RxDesc;

			synopGMAC_rx_desc_init_chain(first_desc);
			TR("%02d  %08x %08x %08x %08x %08x %08x %08x \n",gmacdev->RxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1,first_desc->buffer2, first_desc->data1, first_desc->data2);
			gmacdev->RxDescCount += 1;
			first_desc = second_desc;
		}
		synopGMAC_rx_desc_init_chain(first_desc);
		TR("%02d  %08x %08x %08x %08x %08x %08x %08x \n",gmacdev->RxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1,first_desc->buffer2, first_desc->data1, first_desc->data2);
		TR("Rx===================================================================Rx\n");
	}
	flush_dcache();

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;

	gmacdev->BusyRxDesc   = 0;

	return -ESYNOPGMACNOERR;
}

/**
 * This gives up the receive Descriptor queue in ring or chain mode.
 * This function is tightly coupled to the platform and operating system
 * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
 * is completely handled by the operating system, this call is kept outside the device driver Api.
 * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
 * and network buffer deallocation.
 * This function depends on the pcidev structure for dma-able memory deallocation for both descriptor memory and the
 * network buffer memory under linux.
 * The responsibility of this function is to
 *     - Free the network buffer memory if any.
 *     - Fee the memory allocated for the descriptors.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] pointer to pci_device structure.
 * @param[in] number of descriptor expected in rx descriptor queue.
 * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
 * \return 0 upon success. Error code upon failure.
 * \note No referece should be made to descriptors once this function is called. This function is invoked when the device is closed.
 */
void synopGMAC_giveup_rx_desc_queue(synopGMACdevice * gmacdev, u32 desc_mode)
{
	s32 i;

	DmaDesc *first_desc = NULL;
	dma_addr_t first_desc_dma_addr;
	u32 status;
	dma_addr_t dma_addr1;
	dma_addr_t dma_addr2;
	u32 length1;
	u32 length2;
	u32 data1;
	u32 data2;

	flush_dcache();

	if(desc_mode == RINGMODE){
		for(i =0; i < gmacdev -> RxDescCount; i++){
			synopGMAC_get_desc_data(gmacdev->RxDesc + i, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
			if((length1 != 0) && (data1 != 0)){
				dma_unmap_single((volatile void *)dma_addr1, 0, DMA_FROM_DEVICE);
				skb_free((struct sk_buff *) data1);
				TR("(Ring mode) rx buffer1 %08x of size %d from %d rx descriptor is given back\n",data1, length1, i);
			}
			if((length2 != 0) && (data2 != 0)){
				dma_unmap_single((volatile void *)dma_addr2, 0, DMA_FROM_DEVICE);
				skb_free((struct sk_buff *) data2);
				TR("(Ring mode) rx buffer2 %08x of size %d from %d rx descriptor is given back\n",data2, length2, i);
			}
		}
		plat_free_consistent_dmaable_memory((sizeof(DmaDesc) * gmacdev->RxDescCount),gmacdev->RxDesc,gmacdev->RxDescDma); //free descriptors memory
		TR("Memory allocated %08x  for Rx Desriptors (ring) is given back\n",(u32)gmacdev->RxDesc);
	}
	else{
		TR("rx-------------------------------------------------------------------rx\n");
		first_desc          = gmacdev->RxDesc;
		first_desc_dma_addr = gmacdev->RxDescDma;
		for(i =0; i < gmacdev -> RxDescCount; i++){
			synopGMAC_get_desc_data(first_desc, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
			TR("%02d %08x %08x %08x %08x %08x %08x %08x\n",i,(u32)first_desc,first_desc->status,first_desc->length,first_desc->buffer1,first_desc->buffer2,first_desc->data1,first_desc->data2);
			if((length1 != 0) && (data1 != 0)){
				dma_unmap_single((volatile void *)dma_addr1, 0, DMA_FROM_DEVICE);
				skb_free((struct sk_buff *) data1);
				TR("(Chain mode) rx buffer1 %08x of size %d from %d rx descriptor is given back\n",data1, length1, i);
			}
			plat_free_consistent_dmaable_memory((sizeof(DmaDesc)),first_desc,first_desc_dma_addr); //free descriptors
			TR("Memory allocated %08x for Rx Descriptor (chain) at  %d is given back\n",data2,i);

			first_desc = (DmaDesc *)data2;
			first_desc_dma_addr = dma_addr2;
		}

		TR("rx-------------------------------------------------------------------rx\n");
	}
	gmacdev->RxDesc    = NULL;
	gmacdev->RxDescDma = 0;
}

/**
 * This gives up the transmit Descriptor queue in ring or chain mode.
 * This function is tightly coupled to the platform and operating system
 * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
 * is completely handled by the operating system, this call is kept outside the device driver Api.
 * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
 * and network buffer deallocation.
 * This function depends on the pcidev structure for dma-able memory deallocation for both descriptor memory and the
 * network buffer memory under linux.
 * The responsibility of this function is to
 *     - Free the network buffer memory if any.
 *     - Fee the memory allocated for the descriptors.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] pointer to pci_device structure.
 * @param[in] number of descriptor expected in tx descriptor queue.
 * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
 * \return 0 upon success. Error code upon failure.
 * \note No reference should be made to descriptors once this function is called. This function is invoked when the device is closed.
 */
void synopGMAC_giveup_tx_desc_queue(synopGMACdevice * gmacdev, u32 desc_mode)
{
	s32 i;

	DmaDesc *first_desc = NULL;
	dma_addr_t first_desc_dma_addr;
	u32 status;
	dma_addr_t dma_addr1;
	dma_addr_t dma_addr2;
	u32 length1;
	u32 length2;
	u32 data1;
	u32 data2;

	flush_dcache();

	if(desc_mode == RINGMODE){
		for(i =0; i < gmacdev -> TxDescCount; i++){
			synopGMAC_get_desc_data(gmacdev->TxDesc + i,&status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
			if((length1 != 0) && (data1 != 0)){
				dma_unmap_single((volatile void *)dma_addr1,0,DMA_TO_DEVICE);
				skb_free((struct sk_buff *) data1);
				TR("(Ring mode) tx buffer1 %08x of size %d from %d rx descriptor is given back\n",data1, length1, i);
			}
			if((length2 != 0) && (data2 != 0)){
				dma_unmap_single((volatile void *)dma_addr2,0,DMA_TO_DEVICE);
				skb_free((struct sk_buff *) data2);
				TR("(Ring mode) tx buffer2 %08x of size %d from %d rx descriptor is given back\n",data2, length2, i);
			}
		}
		plat_free_consistent_dmaable_memory((sizeof(DmaDesc) * gmacdev->TxDescCount),gmacdev->TxDesc,gmacdev->TxDescDma); //free descriptors
		TR("Memory allocated %08x for Tx Desriptors (ring) is given back\n",(u32)gmacdev->TxDesc);
	}
	else{
		TR("tx-------------------------------------------------------------------tx\n");
		first_desc          = gmacdev->TxDesc;
		first_desc_dma_addr = gmacdev->TxDescDma;
		for(i =0; i < gmacdev -> TxDescCount; i++){
			synopGMAC_get_desc_data(first_desc, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
			TR("%02d %08x %08x %08x %08x %08x %08x %08x\n",i,(u32)first_desc,first_desc->status,first_desc->length,first_desc->buffer1,first_desc->buffer2,first_desc->data1,first_desc->data2);
			if((length1 != 0) && (data1 != 0)){
				dma_unmap_single((volatile void *)dma_addr1,0,DMA_TO_DEVICE);
				skb_free((struct sk_buff *) data2);
				TR("(Chain mode) tx buffer1 %08x of size %d from %d rx descriptor is given back\n",data1, length1, i);
			}
			plat_free_consistent_dmaable_memory((sizeof(DmaDesc)),first_desc,first_desc_dma_addr); //free descriptors
			TR("Memory allocated %08x for Tx Descriptor (chain) at  %d is given back\n",data2,i);

			first_desc = (DmaDesc *)data2;
			first_desc_dma_addr = dma_addr2;
		}
		TR("tx-------------------------------------------------------------------tx\n");

	}
	gmacdev->TxDesc    = NULL;
	gmacdev->TxDescDma = 0;
}

/**
 * Function to handle housekeeping after a packet is transmitted over the wire.
 * After the transmission of a packet DMA generates corresponding interrupt
 * (if it is enabled). It takes care of returning the sk_buff to the linux
 * kernel, updating the networking statistics and tracking the descriptors.
 * @param[in] pointer to net_device structure.
 * \return void.
 * \note This function runs in interrupt context
 */
void synop_handle_transmit_over(struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	s32 desc_index;
	u32 data1, data2;
	u32 status;
	u32 length1, length2;
	u32 dma_addr1, dma_addr2;
	adapter = netdev->priv;
	if(adapter == NULL){
		TR("Unknown Device\n");
		return;
	}

	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR("GMAC device structure is missing\n");
		return;
	}

	/*Handle the transmit Descriptors*/
	do {
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
		TR0("func:%s,desc_index=%d,length1=%d,data1=0x%x\n", __func__, desc_index, length1, data1);
		//desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
		if(desc_index >= 0 && data1 != 0){
			TR("Finished Transmit at Tx Descriptor %d for skb 0x%08x and buffer = %08x whose status is %08x \n", desc_index,data1,dma_addr1,status);

			dma_unmap_single((volatile void *)dma_addr1,length1,DMA_TO_DEVICE);
			skb_free((struct sk_buff *)data1);	//del by huangjb20121210, net.c will free it.

			if(synopGMAC_is_desc_valid(status)){
				adapter->synopGMACNetStats.tx_bytes += length1;
				adapter->synopGMACNetStats.tx_packets++;
			}
			else {
				TR("Error in Status %08x\n",status);
				adapter->synopGMACNetStats.tx_errors++;
				adapter->synopGMACNetStats.tx_aborted_errors += synopGMAC_is_tx_aborted(status);
				adapter->synopGMACNetStats.tx_carrier_errors += synopGMAC_is_tx_carrier_error(status);
			}
		}	adapter->synopGMACNetStats.collisions += synopGMAC_get_tx_collision_count(status);
	} while(desc_index >= 0);
}

/**
 * Function to Receive a packet from the interface.
 * After Receiving a packet, DMA transfers the received packet to the system memory
 * and generates corresponding interrupt (if it is enabled). This function prepares
 * the sk_buff for received packet after removing the ethernet CRC, and hands it over
 * to linux networking stack.
 *	- Updataes the networking interface statistics
 *	- Keeps track of the rx descriptors
 * @param[in] pointer to net_device structure.
 * \return void.
 * \note This function runs in interrupt context.
 */

void synop_handle_received_data(struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	s32 desc_index;

	u32 data1;
	u32 data2;
	u32 len;
	u32 status;
	u32 dma_addr1;
	u32 dma_addr2;
	//u32 length;

	struct sk_buff *skb; //This is the pointer to hold the received data
#ifdef DEBUG
	u32 i;
#endif

	TR("%s\n",__FUNCTION__);

	adapter = netdev->priv;
	if(adapter == NULL){
		TR("Unknown Device\n");
		return;
	}

	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR("GMAC device structure is missing\n");
		return;
	}

	/*Handle the Receive Descriptors*/
	do{
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status,&dma_addr1,NULL, &data1,&dma_addr2,NULL,&data2);

		if(desc_index >= 0 && data1 != 0){
			TR("Received Data at Rx Descriptor %d for skb 0x%08x whose status is %08x\n",desc_index,data1,status);
			/*At first step unmapped the dma address*/
			dma_unmap_single((volatile void *)dma_addr1,0,DMA_FROM_DEVICE);

			skb = (struct sk_buff *)data1;
			if(synopGMAC_is_rx_desc_valid(status)){
				len =  synopGMAC_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes
				TR0("func %s line%d:skb=0x%x,len=%d\n", __func__, __LINE__, skb, len);
#ifdef DEBUG
				printk("%s: len: %x\n", __FUNCTION__, len + 4);

				printk("gary:\n");
				for(i=0; i < (len + 4); i++) {
					printk("%02x ", skb->data[i]);
					if(((i+1)%8) == 0) {
						printk("\n");
					}
				}
				printk("gary\n");
#endif
				flush_dcache();
				skb_put(skb);

				skb->dev = netdev;

				adapter->synopGMACNetStats.rx_packets++;
				adapter->synopGMACNetStats.rx_bytes += len;
			}
			else{
				/*Now the present skb should be set free*/
				skb_free(skb);
				printk(KERN_CRIT "s: %08x\n",status);
				adapter->synopGMACNetStats.rx_errors++;
				adapter->synopGMACNetStats.collisions       += synopGMAC_is_rx_frame_collision(status);
				adapter->synopGMACNetStats.rx_crc_errors    += synopGMAC_is_rx_crc(status);
				adapter->synopGMACNetStats.rx_frame_errors  += synopGMAC_is_frame_dribbling_errors(status);
				adapter->synopGMACNetStats.rx_length_errors += synopGMAC_is_rx_frame_length_errors(status);
			}

			//Now lets allocate the skb for the emptied descriptor
			skb = skb_alloc(MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC);
			if(skb == NULL){
				TR("SKB memory allocation failed \n");
				adapter->synopGMACNetStats.rx_dropped++;
			}

			dma_addr1 = dma_map_single(skb->data,MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC,DMA_FROM_DEVICE);
			desc_index = synopGMAC_set_rx_qptr(gmacdev,dma_addr1, MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC, (u32)skb,0,0,0);

			if (desc_index < 0) {
				TR("Cannot set Rx Descriptor for skb %08x\n",(u32)skb);
				skb_free(skb);
			}
		}
	}while(desc_index >= 0);
}

/**
 * Interrupt service routing.
 * This is the function registered as ISR for device interrupts.
 * @param[in] interrupt number.
 * @param[in] void pointer to device unique structure (Required for shared interrupts in Linux).
 * @param[in] pointer to pt_regs (not used).
 * \return Returns IRQ_NONE if not device interrupts IRQ_HANDLED for device interrupts.
 * \note This function runs in interrupt context
 *
 */
int synopGMAC_intr_handler(s32 intr_num, void * dev_id)//, struct pt_regs *regs)
{
	struct net_device *netdev;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	u32 interrupt,dma_status_reg;
	s32 status;
	u32 dma_addr;

	netdev  = (struct net_device *) dev_id;
	if(netdev == NULL){
		TR("Unknown Device\n");
		return -1;
	}

	adapter  = netdev->priv;
	if(adapter == NULL){
		TR("Adapter Structure Missing\n");
		return -1;
	}

	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR("GMAC device structure Missing\n");
		return -1;
	}

	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaStatus);
	if(dma_status_reg == 0)
		return IRQ_NONE;

	synopGMAC_disable_interrupt_all(gmacdev);

	TR("%s:Dma Status Reg: 0x%08x\n",__FUNCTION__,dma_status_reg);

	if(dma_status_reg & GmacPmtIntr){
		TR("%s:: Interrupt due to PMT module\n",__FUNCTION__);
		synopGMAC_linux_powerup_mac(gmacdev);
		synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,GmacPmtIntr);//added by fangjl 20111208
	}

	if(dma_status_reg & GmacMmcIntr){
		TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
		TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev));
		TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev));
		synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,GmacMmcIntr);//added by fangjl 20111208
	}

	if(dma_status_reg & GmacLineIntfIntr){
		TR("%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__);
	}

	/*Now lets handle the DMA interrupts*/
	interrupt = synopGMAC_get_interrupt_type(gmacdev);
	TR("%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt);

	if(interrupt & synopGMACDmaError){
		u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;//after soft reset, configure the MAC address to default value
		/*u8 mac_addr0[6];
		  mac_addr0[0] = gx_mac_addr[0];
		  mac_addr0[1] = gx_mac_addr[1];
		  mac_addr0[2] = gx_mac_addr[2];
		  mac_addr0[3] = gx_mac_addr[3];
		  mac_addr0[4] = gx_mac_addr[4];
		  mac_addr0[5] = gx_mac_addr[5];		*/
		TR("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
		synopGMAC_disable_dma_tx(gmacdev);
		synopGMAC_disable_dma_rx(gmacdev);

		synopGMAC_take_desc_ownership_tx(gmacdev);
		synopGMAC_take_desc_ownership_rx(gmacdev);

		synopGMAC_init_tx_rx_desc_queue(gmacdev);

		synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip

		synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr0);
		synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
		synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward);
		synopGMAC_init_rx_desc_base(gmacdev);
		synopGMAC_init_tx_desc_base(gmacdev);
		synopGMAC_mac_init(gmacdev);
		synopGMAC_enable_dma_rx(gmacdev);
		synopGMAC_enable_dma_tx(gmacdev);
		synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntBusError);//added by fangjl 20111208
	}

	if(intr_num == HANDLE_RX){
		if(interrupt & synopGMACDmaRxNormal){
			synopGMAC_disable_dma_rx(gmacdev);
			flush_dcache();
			TR("%s:: Rx Normal \n", __FUNCTION__);
			synop_handle_received_data(netdev);
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntRxCompleted);//added by fangjl 20111208
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntRxStopped);
			synopGMAC_enable_dma_rx(gmacdev);
		}

		if(interrupt & synopGMACDmaRxAbnormal){
			TR("%s::Abnormal Rx Interrupt Seen\n",__FUNCTION__);
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				adapter->synopGMACNetStats.rx_over_errors++;
				/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
				synopGMAC_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
			}
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntRxNoBuffer);//added by fangjl 20111208
		}

		if(interrupt & synopGMACDmaRxStopped){
			TR("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				adapter->synopGMACNetStats.rx_over_errors++;
				do{
					struct sk_buff *skb;
					skb = skb_alloc(MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC);
					if(skb == NULL){
						TR("%s::ERROR in skb buffer allocation Better Luck Next time\n",__FUNCTION__);
						break;
						//			return -ESYNOPGMACNOMEM;
					}

					dma_addr = dma_map_single(skb->data,sizeof(skb),DMA_FROM_DEVICE);
					status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC, (u32)skb,0,0,0);
					TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
					if(status < 0){
						skb_free(skb);
					}

				}while(status >= 0);

				synopGMAC_enable_dma_rx(gmacdev);
			}
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntRxStopped);//added by fangjl 20111208
		}
	}

	if(intr_num == HANDLE_TX){
		if(interrupt & synopGMACDmaTxNormal){
			//xmit function has done its job
			TR0("%s::Finished Normal Transmission \n",__FUNCTION__);
			synop_handle_transmit_over(netdev);//Do whatever you want after the transmission is over
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntTxCompleted);//added by fangjl 20111208
		}

		if(interrupt & synopGMACDmaTxAbnormal){
			TR0("%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synop_handle_transmit_over(netdev);
			}
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntTxUnderflow);//added by fangjl 20111208
		}

		if(interrupt & synopGMACDmaTxStopped){
			TR0("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synopGMAC_disable_dma_tx(gmacdev);
				synopGMAC_take_desc_ownership_tx(gmacdev);

				synopGMAC_enable_dma_tx(gmacdev);
				//netif_wake_queue(netdev);
				TR("%s::Transmission Resumed\n",__FUNCTION__);
			}
			synopGMACWriteReg((u32 *)gmacdev->DmaBase, DmaStatus ,DmaIntTxStopped);//added by fangjl 20111208
		}
	}
	/* Enable the interrrupt before returning from ISR*/
	return IRQ_HANDLED;
}


/**
 * Function used when the interface is opened for use.
 * We register synopGMAC_linux_open function to linux open(). Basically this
 * function prepares the the device for operation . This function is called whenever ifconfig (in Linux)
 * activates the device (for example "ifconfig eth0 up"). This function registers
 * system resources needed
 *	- Attaches device to device specific structure
 *	- Programs the MDC clock for PHY configuration
 *	- Check and initialize the PHY interface
 *	- ISR registration
 *	- Setup and initialize Tx and Rx descriptors
 *	- Initialize MAC and DMA
 *	- Allocate Memory for RX descriptors (The should be DMAable)
 *	- Initialize one second timer to detect cable plug/unplug
 *	- Configure and Enable Interrupts
 *	- Enable Tx and Rx
 *	- start the Linux network queue interface
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */

s32 synopGMAC_linux_open(struct net_device *netdev)
{
	s32 status = 0;
	s32 retval = 0;
	//s32 reserve_len=2;
	u32 dma_addr;
	struct sk_buff *skb;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	TR0("%s called \n",__FUNCTION__);
	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;

	/*Now platform dependent initialization.*/

	/*Lets reset the IP*/
	TR("adapter= %08x gmacdev = %08x netdev = %08x\n",(u32)adapter,(u32)gmacdev,(u32)netdev);

	status = synopGMAC_check_phy_init(gmacdev, rmii_flag);
	if (status) {
		TR0("Error in synopGMAC_check_phy_init.\n");
	}

	/*Set up the tx and rx descriptor queue/ring*/

	synopGMAC_setup_tx_desc_queue(gmacdev,tx_desc, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

	synopGMAC_setup_rx_desc_queue(gmacdev,rx_desc, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2);                      //pbl32 incr with rxthreshold 128
	synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward |DmaTxSecondFrame|DmaRxThreshCtrl128);

	/*Initialize the mac interface*/

	synopGMAC_mac_init(gmacdev);
	synopGMAC_promisc_enable(gmacdev);
	synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	do{
		skb = skb_alloc(MTU_LEN + ETHERNET_HEADER + ETHERNET_CRC);
		if(skb == NULL){
			TR0("ERROR in skb buffer allocation\n");
			break;
		}
		dma_addr = dma_map_single(skb->data, skb->len ,DMA_FROM_DEVICE);
		status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb->len, (u32)skb,0,0,0);
		if(status < 0)
		{
			skb_free(skb);
		}

	}while(status >= 0);


	synopGMAC_clear_interrupt(gmacdev);
	/*
	   Disable the interrupts generated by MMC and IPC counters.
	   If these are not disabled ISR should be modified accordingly to handle these interrupts.
	   */
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	synopGMAC_disable_interrupt_all(gmacdev);	//added by fangjl 20111130 disable interrupts
	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);

	if(rmii_flag == 1)
	{
		synopGMAC_linux_cable_fes_function(gmacdev);
	}

	return retval;
}

/**
 * Function used when the interface is closed.
 *
 * This function is registered to linux stop() function. This function is
 * called whenever ifconfig (in Linux) closes the device (for example "ifconfig eth0 down").
 * This releases all the system resources allocated during open call.
 * system resources int needs
 *	- Disable the device interrupts
 *	- Stop the receiver and get back all the rx descriptors from the DMA
 *	- Stop the transmitter and get back all the tx descriptors from the DMA
 *	- Stop the Linux network queue interface
 *	- Free the irq (ISR registered is removed from the kernel)
 *	- Release the TX and RX descripor memory
 *	- De-initialize one second timer rgistered for cable plug/unplug tracking
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */

s32 synopGMAC_linux_close(struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;

	TR0("%s\n",__FUNCTION__);
	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	if(adapter == NULL){
		TR0("OOPS adapter is null\n");
		return -1;
	}

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR0("OOPS gmacdev is null\n");
		return -1;
	}


	/*Disable all the interrupts*/
	synopGMAC_disable_interrupt_all(gmacdev);

	TR("the synopGMAC interrupt has been disabled\n");

	/*Disable the reception*/
	synopGMAC_disable_dma_rx(gmacdev);
	synopGMAC_take_desc_ownership_rx(gmacdev);
	TR("the synopGMAC Reception has been disabled\n");

	/*Disable the transmission*/
	synopGMAC_disable_dma_tx(gmacdev);
	synopGMAC_take_desc_ownership_tx(gmacdev);

	TR("the synopGMAC Transmission has been disabled\n");
	TR("the synopGMAC interrupt handler has been removed\n");

	/*Free the Rx Descriptor contents*/
	TR("Now calling synopGMAC_giveup_rx_desc_queue \n");
	synopGMAC_giveup_rx_desc_queue(gmacdev, RINGMODE);
	TR("Now calling synopGMAC_giveup_tx_desc_queue \n");
	synopGMAC_giveup_tx_desc_queue(gmacdev, RINGMODE);

	TR("Freeing the cable unplug timer\n");

	return -ESYNOPGMACNOERR;
}

/**
 * Function to transmit a given packet on the wire.
 * Whenever Linux Kernel has a packet ready to be transmitted, this function is called.
 * The function prepares a packet and prepares the descriptor and
 * enables/resumes the transmission.
 * @param[in] pointer to sk_buff structure.
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and Error code on failure.
 * \note structure sk_buff is used to hold packet in Linux networking stacks.
 */
s32 synopGMAC_linux_xmit_frames(struct sk_buff *skb, struct net_device *netdev, int async)
{
	s32 status = 0;
#ifdef DEBUG
	s32 i =0;
#endif
	u32 offload_needed = 0;
	u32 dma_addr;
	//u32 flags;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	TR("%s called \n",__FUNCTION__);
	if(skb == NULL){
		TR0("skb is NULL What happened to Linux Kernel? \n ");
		return -1;
	}

	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	if(adapter == NULL)
		return -1;

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL)
		return -1;

	/*Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/
	dma_addr = dma_map_single(skb->data,skb->len,DMA_TO_DEVICE);
	flush_dcache();
#ifdef DEBUG
	printk("%s: skb->len: %x\n", __FUNCTION__, skb->len);

	printk("gary:\n");
	for(i=0; i < skb->len; i++) {
		printk("%02x ", skb->data[i]);
		if(((i+1)%8) == 0) {
			printk("\n");
		}
	}
	printk("gary\n");
#endif
	TR0("func:%s,skb->len=%d,skb=0x%x\n", __func__, skb->len, skb);
	status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, skb->len, (u32)skb,0,0,0,offload_needed);
	if(status < 0){
		TR0("%s No More Free Tx Descriptors.skb=0x%x,skb->len=%d\n",__FUNCTION__,skb,skb->len);
		skb_free(skb);
		return -EBUSY;
	}

	/*Now force the DMA to start transmission*/
	synopGMAC_resume_dma_tx(gmacdev);

	return -ESYNOPGMACNOERR;
}

/**
 * Function to initialize the Linux network interface.
 *
 * Linux dependent Network interface is setup here. This provides
 * an example to handle the network dependent functionality.
 *
 * return Returns 0 on success and Error code on failure.
 */
s32 synopGMAC_init_network_interface(struct net_device *netdev)
{
	TR("Now Going to Call register_netdev to register the network interface for GMAC core\n");
	/*
	   Lets allocate and set up an ethernet device, it takes the sizeof the private structure. This is mandatory as a 32 byte
	   allignment is required for the private data structure.
	   */
	synopGMACadapter = (synopGMACPciNetworkAdapter *)plat_alloc_memory(sizeof(synopGMACPciNetworkAdapter));
	if(!synopGMACadapter){
		TR0("Error in Memory Allocataion \n");
	}

	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter->synopGMACdev = (synopGMACdevice *) plat_alloc_memory(sizeof (synopGMACdevice));
	if(!synopGMACadapter->synopGMACdev){
		TR0("Error in Memory Allocataion \n");
	}

	netdev->priv = synopGMACadapter;

	synopGMACadapter->synopGMACnetdev = netdev;

	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	synopGMAC_attach(synopGMACadapter->synopGMACdev,(u32) synopGMACMappedAddr + MACBASE,(u32) synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE);

	check_rtl8201fl_flag(synopGMACadapter->synopGMACdev);
	check_rmii_flag(synopGMACadapter->synopGMACdev, &rmii_flag);
	synopGMAC_reset(synopGMACadapter->synopGMACdev);

	if (rmii_flag == 1) {
		synopGMAC_setup_0050a1b0(synopGMACadapter->synopGMACdev);
	}

	synopGMAC_reset(synopGMACadapter->synopGMACdev);

	/*Lets read the version of ip in to device structure*/
	synopGMAC_read_version(synopGMACadapter->synopGMACdev);

	/*Check for Phy initialization*/
	synopGMAC_set_mdc_clk_div(synopGMACadapter->synopGMACdev,GmiiCsrClk3);
	synopGMACadapter->synopGMACdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(synopGMACadapter->synopGMACdev);

	synopGMAC_set_mac_addr(synopGMACadapter->synopGMACdev,GmacAddr0High,GmacAddr0Low, netdev->dev_addr); //added by fangjl 20111201
	//synopGMAC_get_mac_addr(synopGMACadapter->synopGMACdev,GmacAddr0High,GmacAddr0Low, netdev->dev_addr);
	return 0;
}


/**
 * Function to initialize the Linux network interface.
 * Linux dependent Network interface is setup here. This provides
 * an example to handle the network dependent functionality.
 * \return Returns 0 on success and Error code on failure.
 */
void synopGMAC_exit_network_interface(struct net_device *netdev)
{
	void* tmp;

	tmp = (void*) synopGMACadapter->synopGMACdev;

	TR0("Now Calling network_unregister\n");

	plat_free_memory(tmp);
}

