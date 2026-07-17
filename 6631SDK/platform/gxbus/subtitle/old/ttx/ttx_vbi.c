#include "module/ttx/gxttx.h"
#include "av/gxav_vpu_propertytypes.h"
#include "av/gxav_module_property.h"
#include "av/avapi.h"
#define	BUFFER_NUM	(20) // TODO: 100

typedef struct  
{
	V_DATA_VPU VBI_DATA_VPU[BUFFER_NUM];
} VBI_BUFFER_VPU;

extern int g_device_handle;
extern int vpu_handle;

VBI_BUFFER_VPU *vbi_buffer_data;
VBI_BUFFER_VPU *vbi_buffer=NULL;
VBI_BUFFER_VPU *vbi_buffer_tem=NULL;

V_DATA_VPU  STUFF_DATA;

uint16_t VBI_BUFFER_PTR;

uint8_t  VBI_BUFFER_PTR_num;
static uint8_t chVBIttxDataCopy;
uint32_t nEmptyBufferForBuffer1[13],nEmptyBufferForBuffer2[13];
void ttx_clear_vbi_buffer(void)
{
	uint16_t wVaryj,wVaryt;
	for(wVaryt = 0;wVaryt<BUFFER_NUM;wVaryt++)
	{
		for(wVaryj=0;wVaryj<32;wVaryj++)
		{
			memset(&(vbi_buffer_data->VBI_DATA_VPU[wVaryt].DATA[wVaryj][1]),0,48);
		}
	}
	
}

#define _REG_GET_BYTE0(reg)  ((reg)  &  0xFF)
#define _REG_GET_BYTE1(reg)  (((reg) >> 8) & 0xFF)
#define _REG_GET_BYTE2(reg)  (((reg) >> 16) & 0xFF)
#define _REG_GET_BYTE3(reg)  (((reg) >> 24) & 0xFF)
#define VBI_ADDR_MASK 0x3ffffff
#define VBI_ADDR_OFFSET 0

#define CHANGE_ENDIAN_32(reg) do {                 \
	unsigned int Reg = (reg);                  \
	Reg  = ((_REG_GET_BYTE0(Reg) << 24) |       \
	(_REG_GET_BYTE1(Reg) << 16) |               \
	(_REG_GET_BYTE2(Reg) << 8 ) |               \
	(_REG_GET_BYTE3(Reg))) ;                    \
	(reg)   =   (Reg) ;                        \
}while(0)

#define GX_SET_FEILD_E(reg,mask,val,offset) do {   \
    unsigned int tmpVal  = *(unsigned int*)(reg);  \
    CHANGE_ENDIAN_32(tmpVal) ;                     \
    tmpVal =(tmpVal&(~(mask)))|(((val)<<(offset))& (mask)) ;  \
    CHANGE_ENDIAN_32(tmpVal) ;                     \
    *(unsigned int*)(reg) = (tmpVal);              \
}while(0)


void teletext_init_vbi_buffer(void)
{
	uint16_t i,j,t;
	uint32_t tmp;
	//初始化vbi填充数据段
	VBI_BUFFER_PTR = 0;
	memset(&nEmptyBufferForBuffer1[1],0x0,48);
	memset(&nEmptyBufferForBuffer2[1],0x0,48);
	nEmptyBufferForBuffer1[0] = 0x30000000+((uint32_t)((&nEmptyBufferForBuffer1[0]))&0x3ffffff);//protect if the speed of vpu is faster than software
	nEmptyBufferForBuffer2[0] = 0x30000000+((uint32_t)((&nEmptyBufferForBuffer2[0]))&0x3ffffff);
	CHANGE_ENDIAN_32(nEmptyBufferForBuffer1[0]);
	CHANGE_ENDIAN_32(nEmptyBufferForBuffer2[0]);
	vbi_buffer_data = vbi_buffer;
	ttx_clear_vbi_buffer();
	for(t =0;t<BUFFER_NUM;t++)
	{
		for(j =0;j<31;j++)
		{
			i =j+1;
			tmp = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[t].DATA[i][0])))&0x3ffffff; 
			tmp = tmp |0x30000000;
			CHANGE_ENDIAN_32(tmp);
			vbi_buffer_data->VBI_DATA_VPU[t].DATA[j][0] = tmp;
		}

	}

	for(t =0;t<BUFFER_NUM-1;t++)
	{
		i =t+1;
		tmp = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[i].DATA[0][0])))&0x3ffffff; 
		tmp = tmp |0x30000000;
		CHANGE_ENDIAN_32(tmp);
		vbi_buffer_data->VBI_DATA_VPU[t].DATA[31][0] = tmp;
	}
	vbi_buffer_data->VBI_DATA_VPU[t].DATA[31][0]=nEmptyBufferForBuffer1[0];

	

	vbi_buffer_data = vbi_buffer_tem;
	ttx_clear_vbi_buffer();
	for(t =0;t<BUFFER_NUM;t++)
	{
		for(j =0;j<31;j++)
		{
			i =j+1;
			tmp = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[t].DATA[i][0])))&0x3ffffff; 
			tmp = tmp |0x30000000;
			CHANGE_ENDIAN_32(tmp);
			vbi_buffer_data->VBI_DATA_VPU[t].DATA[j][0] = tmp;
		}

	}

	for(t =0;t<BUFFER_NUM-1;t++)
	{
		i =t+1;
		tmp = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[i].DATA[0][0])))&0x3ffffff; 
		tmp = tmp |0x30000000;
		CHANGE_ENDIAN_32(tmp);
		vbi_buffer_data->VBI_DATA_VPU[t].DATA[31][0] = tmp;
	}
	vbi_buffer_data->VBI_DATA_VPU[t].DATA[31][0]=nEmptyBufferForBuffer2[0];
	VBI_BUFFER_PTR_num = 0;	
}


uint8_t teletext_vbi_inistial(void)//初始化解复用，和VPU数据读取，TV数据读取
{
#if 0
	VBI_CTRL = 0x30000000;	
	teletext_init_vbi_buffer();	
	//初始化TV
	VBI_TLX_CTRL1 = 0x7FFF8FFF;
	VBI_TLX_CTRL2 = 0xF3;
	VBI_ADDR = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff;
	chVBIttxDataCopy=0;
#endif
	int ret=0;
	GxVpuProperty_VbiCreateBuffer vbi_buffer_1;
	vbi_buffer_1.unit_data_len = 1664;
	vbi_buffer_1.unit_num = BUFFER_NUM*2;
	vbi_buffer_1.buffer = 0;
	ret = GxAVGetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_VbiCreateBuffer,
						  &vbi_buffer_1,
						  sizeof(GxVpuProperty_VbiCreateBuffer));
	*(volatile uint32_t*)(0xd1100054) |= 0x30000000;
	*(volatile uint32_t*)(0xd1104078) = 0x7FFF8FFF;
	*(volatile uint32_t*)(0xd110407C)= 0xF3;
	gxlogd("\n********VbiCreateBuffer ret:%d*********\n",ret);
	vbi_buffer_tem = (VBI_BUFFER_VPU*)(vbi_buffer_1.buffer);
	vbi_buffer = (VBI_BUFFER_VPU*)(vbi_buffer_tem+1);
	teletext_init_vbi_buffer();
	chVBIttxDataCopy=0;


	return ret;
}
uint8_t teletext_vbi_destroy(void)
{
	int ret=0;
	GxVpuProperty_VbiDestroyBuffer vbi_buffer_1;

	if(vbi_buffer)
	{
		vbi_buffer_1.unit_data_len = 1664;
		vbi_buffer_1.unit_num = BUFFER_NUM*2;
		vbi_buffer_1.buffer = vbi_buffer;
		ret = GxAVSetProperty(g_device_handle,
							  vpu_handle,
							  GxVpuPropertyID_VbiDestroyBuffer,
							  &vbi_buffer_1,
							  sizeof(GxVpuProperty_VbiDestroyBuffer));
		vbi_buffer = NULL;
	}
	gxlogd("\n********VbiDestroyBuffer ret:%d*********\n",ret);
	return ret;

}
void VBI_memory_copy(void)
{
	GxVpuProperty_VbiReadAddress VbiReadAddress;
//	int ret=0;
	volatile uint32_t nVBIaddPre;
	VBI_BUFFER_PTR=0;
	nVBIaddPre = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff;
	if(nVBIaddPre==(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff))
		vbi_buffer_data = vbi_buffer_tem;
	else
		vbi_buffer_data = vbi_buffer;
	ttx_clear_vbi_buffer();
	if(1 > chVBIttxDataCopy)
		chVBIttxDataCopy++;//make sure there are at least two buffers are full of data
	else
	{
		if(nVBIaddPre==(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff))
		{
			nEmptyBufferForBuffer2[0]= 0x30000000+(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff);
			CHANGE_ENDIAN_32(nEmptyBufferForBuffer2[0]);	
			vbi_buffer_tem->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]
				=0x30000000+(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff);
			CHANGE_ENDIAN_32(vbi_buffer_tem->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]);
			nEmptyBufferForBuffer1[0] = 0x30000000+(((uint32_t)(&nEmptyBufferForBuffer1[0]))&0x3ffffff);
			CHANGE_ENDIAN_32(nEmptyBufferForBuffer1[0]);
			for(nVBIaddPre=0;nVBIaddPre<20;nVBIaddPre++)//if the speed of VPU is slower than demux,delay sometime to miss data as few as possible
			{	
				
					 GxAVGetProperty(g_device_handle,
						  vpu_handle,
						  GxVpuPropertyID_VbiReadAddress,
						  &VbiReadAddress,
						  sizeof(GxVpuProperty_VbiReadAddress));
				if(((*(volatile uint32_t*)(0xd1100054) &0x3ffffff)<(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][13])))&0x3ffffff))
					&&((*(volatile uint32_t*)(0xd1100054) &0x3ffffff)>(((uint32_t)(&(vbi_buffer->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff)));
				else
					break;
			}
			if(nVBIaddPre==20)
				gxlogd("---------------VBI_CTRL=0x%x-----------\n",(int)(VbiReadAddress.read_address));
			vbi_buffer->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]=nEmptyBufferForBuffer1[0];
		}
		else
		{
			nEmptyBufferForBuffer1[0]= 0x30000000+(((uint32_t)(&(vbi_buffer_tem->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff);
			CHANGE_ENDIAN_32(nEmptyBufferForBuffer1[0]);
			vbi_buffer->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]
				=0x30000000+(((uint32_t)(&(vbi_buffer_tem->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff);
			CHANGE_ENDIAN_32(vbi_buffer->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]);
			nEmptyBufferForBuffer2[0] = 0x30000000+(((uint32_t)(&nEmptyBufferForBuffer2[0]))&0x3ffffff);
			CHANGE_ENDIAN_32(nEmptyBufferForBuffer2[0]);
			for(nVBIaddPre=0;nVBIaddPre<20;nVBIaddPre++)
			{
					GxAVGetProperty(g_device_handle,
				  vpu_handle,
				  GxVpuPropertyID_VbiReadAddress,
				  &VbiReadAddress,
				  sizeof(GxVpuProperty_VbiReadAddress));
				if(((*(volatile uint32_t*)(0xd1100054)&0x3ffffff)<(((uint32_t)(&(vbi_buffer_tem->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][13])))&0x3ffffff))
					&&((*(volatile uint32_t*)(0xd1100054)&0x3ffffff)>(((uint32_t)(&(vbi_buffer_tem->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff)));
				else
					break;
			}
			if(nVBIaddPre==20)
				gxlogd("---------------VBI_CTRL=0x%x-----------\n",(int)(VbiReadAddress.read_address));
			vbi_buffer_tem->VBI_DATA_VPU[BUFFER_NUM-1].DATA[31][0]=nEmptyBufferForBuffer2[0];
		}
	}
}

void teletext_vbi_copy(volatile uint8_t*vbi_read_ptr,uint16_t vbi_len)
{
#if 1
	//int ret=0;
	uint32_t j;
	volatile uint8_t *vpu_buffer_write_ptr;
	uint8_t vbi_line;
	vpu_buffer_write_ptr = vbi_read_ptr;
	while(1)
	{	

		if (vbi_len <46) 
		{
			break;
		}
		if (*vbi_read_ptr != 0xff) //指向data_unit
		{
			vbi_read_ptr++;
			if (*vbi_read_ptr != 0x2c) //data_unit_length
			{
				vbi_read_ptr+=45;
				vbi_len-=46;
				continue;
			}			
			vbi_read_ptr++;	
			if((*vbi_read_ptr&0x20)!=0x20)
				vbi_line= (*vbi_read_ptr &0x1f)+16;
			else
				vbi_line = *vbi_read_ptr &0x1f;
			vbi_line -= 7;
			if(vbi_line>=32)
			{
				vbi_read_ptr+=44;
				vbi_len-=46;
				//gxlogd("\n---------vbi out range----\n");
				continue;
			}
			if(vbi_line>=VBI_BUFFER_PTR_num)
				VBI_BUFFER_PTR_num = vbi_line;
			else
			{
				VBI_BUFFER_PTR_num=vbi_line;
				VBI_BUFFER_PTR++;
				if(BUFFER_NUM==VBI_BUFFER_PTR)
				{
					if(0==chVBIttxDataCopy)
					{
						//VBI_CTRL|= 0x80000000;
						GxVpuProperty_VbiEnable  VbiEnable;
						VbiEnable.enable = 1;
						GxAVSetProperty(
							g_device_handle,
							vpu_handle,
						 	 GxVpuPropertyID_VbiEnable,
							  &VbiEnable,
						  	sizeof(GxVpuPropertyID_VbiEnable));
					}	
					VBI_memory_copy();
					//nVBIaddPre = ((uint32_t)(&(vbi_buffer_data->VBI_DATA_VPU[0].DATA[0][0])))&0x3ffffff;			
				}
			}
			vpu_buffer_write_ptr = (volatile uint8_t*)(&(vbi_buffer_data->VBI_DATA_VPU[VBI_BUFFER_PTR].DATA[VBI_BUFFER_PTR_num][1]));
			vbi_read_ptr++;//framing_code

			*vpu_buffer_write_ptr =0xaa;										
			vpu_buffer_write_ptr ++;
			*vpu_buffer_write_ptr =0xaa;		
			vpu_buffer_write_ptr ++;
			
			for (j = 0;j<43;j++)

			{					
				*vpu_buffer_write_ptr =*vbi_read_ptr;	
				vbi_read_ptr ++;
				vpu_buffer_write_ptr ++;								
			}	
			*vpu_buffer_write_ptr = 0;
		}	
		else
		{
			vbi_read_ptr+=46;
		}
		vbi_len-=46;
		
	}
#endif		
}
