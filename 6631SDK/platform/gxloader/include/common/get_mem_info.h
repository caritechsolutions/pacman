#ifndef __GET_MEM_INFO_H__
#define __GET_MEM_INFO_H__


#define SPP_BUF		(0x00000001)
#define SVPU_BUF	(0x00000002)
#define OSD_BUF		(0x00000003)


/*
 * type: Indicates the type of memory to get, such as SPP_BUF/SVPU_BUF
 * addr: The address of the obtained memory
 * len : The length of the obtained memory
 *
 * */
struct mem_info {
	unsigned int type;
	unsigned int addr;
	unsigned int len;
};


/*
 * get_mem_info()
 *
 *  DESCRIPTION
 *      Get mem info
 *
 *  PARAMETER
 *	@info
 *	    Indicates the type of memory to get, the information of the memory is stored in this structure
 *
 *  RETURN VALUE
 *      On sucess, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int get_mem_info(struct mem_info *info);

#endif

