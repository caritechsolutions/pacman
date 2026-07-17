/** 
 ***********************************************************************
 *                          CONFIDENTIAL                               *
 *        Hangzhou Nationalchip Science and Technology Co. Ltd.        *
 *                       All Rights Reserved.                          *
 ***********************************************************************
 * @file pll_mini.c
 * @brief 
 * @author leo
 * @date 2023-11-13
 */
#include <gxhwlib_registers.h>
#include <cpu_config.h>
#include <io.h>

#ifndef CLKMGR_VERI// Default
  #define MY_PRINTF(...) 
  #define MY_SVSETSCOPE() 
  #define MY_XTAL_FREQ     24
#else // ForClockManagerVerify
  #include <svdpi.h>
  extern int get_xtal_define(); 
  ///scope
  svScope my_scope;
  #define MY_PRINTF(...)  io_printf(__VA_ARGS__)
  #define MY_SVSETSCOPE() svSetScope(my_scope)
  #define MY_XTAL_FREQ    get_xtal_define()
  void save_my_scope(){
    my_scope = svGetScope();
    MY_PRINTF("cpu_config_task scope %s\n",svGetNameFromScope(svGetScope()));  }
#endif

/* routine.c */
void SDelay(unsigned int delay_by)
{
	volatile int i;
	volatile int j;

	for(i=0; i<delay_by; i++)
	{
		for(j=0; j<10; j++)
		{
			j=j;
		}
	}
}

static void cpu_config_write(unsigned int addr,unsigned int data,unsigned int prot)
{
  volatile unsigned int *point;

  point = (unsigned int *)addr;
  *point = data;
}

static void cpu_config_read(unsigned int *data,unsigned int addr,unsigned int prot)
{
  volatile unsigned int *point;

  point = (unsigned int *)addr;
  *data = *point;
}

static void cpu_delay_cycle(unsigned int n)
{
  SDelay(n);
}


static void cpu_write(unsigned int addr, unsigned int data){
  cpu_config_write(addr, data, 0);
  //MY_PRINTF("cpu_config_write, addr=[%0x], data=[%0x]... \n", addr, data);
}

static void cpu_read(unsigned int addr, unsigned int *data){
  cpu_config_read(data, addr, 0);
  //MY_PRINTF("cpu_config_read, addr=[%0x], data=[%0x]... \n", addr, *data);
}

extern void serial_put_no_mmu(char);
extern void serial_init(int, int);

//scorpio
#define COLD_RESET_REG_ADDR      (CONFIG_BASE + CONFIG_MPEG_CLD_RST_NORM)
#define COLD_RST_SET_ADDR        (CONFIG_BASE + CONFIG_MPEG_CLD_RST_1SET)
#define COLD_RST_CLEAR_ADDR      (CONFIG_BASE + CONFIG_MPEG_CLD_RST_1CLR)
#define HOT_RST_REG_ADDR         (CONFIG_BASE + CONFIG_MPEG_HOT_RST_NORM)
#define HOT_RST_SET_ADDR         (CONFIG_BASE + CONFIG_MPEG_HOT_RST_1SET)
#define HOT_RST_CLEAR_ADDR       (CONFIG_BASE + CONFIG_MPEG_HOT_RST_1CLR)
#define CLOCK_GATE_REG0_ADDR     (CONFIG_BASE + CONFIG_CLOCK_GATE_REG0)
#define CLOCK_GATE_SET0_ADDR     (CONFIG_BASE + CONFIG_CLOCK_GATE_SET0)
#define CLOCK_GATE_CLEAR0_ADDR   (CONFIG_BASE + CONFIG_CLOCK_GATE_CLEAR0)
#define CLOCK_GATE_REG2_ADDR     (CONFIG_BASE + CLOCK_GATE_CONFIG2_REG)
#define CLOCK_GATE_SET2_ADDR     (CONFIG_BASE + CLOCK_GATE_CONFIG2_REG_1SET)
#define CLOCK_GATE_CLEAR2_ADDR   (CONFIG_BASE + CLOCK_GATE_CONFIG2_REG_1CLR)
#define CLOCK_GATE_REG3_ADDR     (CONFIG_BASE + CLOCK_GATE_REG3)
#define CLOCK_GATE_SET3_ADDR     (CONFIG_BASE + CLOCK_GATE_SET3)
#define CLOCK_GATE_CLEAR3_ADDR   (CONFIG_BASE + CLOCK_GATE_CLEAR3)
#define CLOCK_GATE_REG4_ADDR     (CONFIG_BASE + CLOCK_GATE_REG4)
#define CLOCK_GATE_SET4_ADDR     (CONFIG_BASE + CLOCK_GATE_SET4)
#define CLOCK_GATE_CLEAR4_ADDR   (CONFIG_BASE + CLOCK_GATE_CLEAR4)
#define DTO_CONFIG1_ADDR         (CONFIG_BASE + CONFIG_DTO1_CONFIG)
#define DTO_CONFIG2_ADDR         (CONFIG_BASE + CONFIG_DTO2_CONFIG)
#define DTO_CONFIG10_ADDR        (CONFIG_BASE + CONFIG_DTO10_CONFIG)
#define DTO_CONFIG11_ADDR        (CONFIG_BASE + CONFIG_DTO11_CONFIG)
#define DTO_CONFIG12_ADDR        (CONFIG_BASE + CONFIG_DTO12_CONFIG)
#define DTO_CONFIG13_ADDR        (CONFIG_BASE + CONFIG_DTO13_CONFIG)
#define DTO_CONFIG15_ADDR        (CONFIG_BASE + CONFIG_DTO15_CONFIG) // [M:1117] 0x5c->0x60
#define PLL_LOCK_IN_ADDR         (CONFIG_BASE + CONFIG_PLL_CONFIG_IN)
#define CLOCK_DIV_CONFIG1_ADDR   (CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG)
#define CLOCK_DIV_CONFIG2_ADDR   (CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG2)
#define CLOCK_DIV_CONFIG3_ADDR   (CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG3)
#define CLOCK_DIV_CONFIG4_ADDR   (CONFIG_BASE + CONFIG_DIV4_CONFIG)
#define CLOCK_DIV_CONFIG5_ADDR   (CONFIG_BASE + CONFIG_DIV5_CONFIG)
#define CLOCK_DIV_CONFIG6_ADDR   (CONFIG_BASE + CONFIG_DIV6_CONFIG)
#define CLOCK_DIV_CONFIG7_ADDR   (CONFIG_BASE + CONFIG_DIV7_CONFIG)
#define CLOCK_DIV_CONFIG8_ADDR   (CONFIG_BASE + CONFIG_DIV8_CONFIG)
#define CLOCK_SOURCE1_ADDR       (CONFIG_BASE + CONFIG_SOURCE1_SEL)
#define CLOCK_SOURCE2_ADDR       (CONFIG_BASE + CONFIG_SOURCE2_SEL)
#define CLOCK_SOURCE3_ADDR       (CONFIG_BASE + CONFIG_SOURCE3_SEL)

#define DTO_PLL_CONFIG_ADDR      (CONFIG_BASE + CONFIG_PLL1_CONFIG)
#define DDR_PLL_CONFIG_ADDR      (CONFIG_BASE + CONFIG_PLL2_CONFIG)
#define CPU_PLL_CONFIG_ADDR      (CONFIG_BASE + CONFIG_PLL3_CONFIG)


struct reg_field{
  unsigned int addr;
  unsigned int site;
  unsigned int bits;
  unsigned int data;
};

struct Reg{
  unsigned int addr;
  unsigned int data;
  unsigned int active;
  unsigned int modify;
};
static struct Reg reg_array[40]; // Real:35
static struct reg_field
  clk_gate_table[] = {
  { CLOCK_GATE_REG3_ADDR  , 20,  1, 0x0 }, // clock_arm_core_pre_en
  //scorpio
  { CLOCK_GATE_REG0_ADDR  , 28,  1, 0x0 }, // clock_denali_phy_en
  { CLOCK_GATE_REG0_ADDR  , 27,  1, 0x0 }, // clock_denali_controller_en
  { CLOCK_GATE_REG0_ADDR  , 16,  1, 0x0 }, // clock_APB2_2_en
  { CLOCK_GATE_REG0_ADDR  , 15,  1, 0x0 }, // clock_APB2_0_en
  { CLOCK_GATE_REG2_ADDR  , 12,  1, 0x1 }, // clock_scpu_en
  { CLOCK_GATE_REG2_ADDR  , 11,  1, 0x1 }, // clock_APB2_4_en

  { CLOCK_GATE_REG3_ADDR  , 21,  1, 0x0 }, // clock_ahb_pre_en
  { CLOCK_GATE_REG3_ADDR  , 19,  1, 0x1 }, // clock_scpu_pre_en
  { CLOCK_GATE_REG3_ADDR  , 16,  1, 0x1 }, // clock_apb2_4_pre_en
  { CLOCK_GATE_REG3_ADDR  , 15,  1, 0x0 }, // clock_apb2_2_pre_en
  { CLOCK_GATE_REG3_ADDR  ,  7,  1, 0x0 }, // clock_apb2_0_pre_en
  { CLOCK_GATE_REG4_ADDR  ,  1,  1, 0x0 }, // clock_ssi_pre_en
  { CLOCK_GATE_REG4_ADDR  ,  2,  1, 0x0 }, // clock_ssi_en

},clk_mux_table[] = {
  { CLOCK_SOURCE1_ADDR    , 24,  1, 0x1 }, // clock_cpu_sel
  //scorpio
  { CLOCK_SOURCE3_ADDR    ,  7,  1, 0x0 }, // clock_pll_dto_cpu_sel // 0112, 1->0, update for rtl 1228

  { CLOCK_SOURCE1_ADDR    , 26,  1, 0x1 }, // clock_denali_sel
  { CLOCK_SOURCE1_ADDR    , 25,  1, 0x1 }, // clock_ddr_sel
  { CLOCK_SOURCE1_ADDR    , 21,  1, 0x1 }, // clock_APB2_4_sel
  { CLOCK_SOURCE1_ADDR    , 20,  1, 0x1 }, // clock_APB2_2_sel
  { CLOCK_SOURCE1_ADDR    , 19,  1, 0x1 }, // clock_APB2_0_sel

  { CLOCK_SOURCE2_ADDR    , 19,  1, 0x1 }, // clock_scpu_source
  { CLOCK_SOURCE3_ADDR    ,  6,  1, 0x1 }, // clock_ssi_source
},clk_div_table[] = {
  { CLOCK_DIV_CONFIG2_ADDR,  0,  4, 0x03 }, // cpu_clk_div_ratio_core_config          clock_DTO_CPU[594MHZ] -> 198MHZ
  { CLOCK_DIV_CONFIG2_ADDR,  4,  4, 0x07 }, // ck_clk_div_ratio_core                  clock_DTO_CPU[594MHZ] -> 118.8MHZ //[M:1117] DIV_ADDR7->DIV_ADDR2
  { CLOCK_DIV_CONFIG5_ADDR, 24,  6, 0x08 }, // scpu_clk_div_config                    1188MHZ -> 148.5MHZ //[M:0130] 1188MHZ -> 132MHZ
  { CLOCK_DIV_CONFIG7_ADDR,  0,  6, 0x05 }, // ssi_clk_div_ratio                      1188MHZ -> 198MHZ
},clk_dto_table[] = {
  //scorpio
  { DTO_CONFIG12_ADDR     ,  0, 30, 0x20000000 }, // DTO value clock_DTO_CPU          1188MHZ -> 594MHZ

  { DTO_CONFIG10_ADDR     ,  0, 30, 0x01745D17 }, // DTO value clock_APB2_0           1188MHZ -> 59.4MHZ
  { DTO_CONFIG11_ADDR     ,  0, 30, 0x0199999a }, // DTO value clock_APB2_2           1188MHZ -> 29.7MHZ
  { DTO_CONFIG13_ADDR     ,  0, 30, 0x0aaaaaab }, // DTO value clock_APB2_4           1188MHZ -> 198MHZ
},clk_rst_table[] = {
  //scorpio
  { DTO_CONFIG10_ADDR     , 30,  2, 0x0 }, // DTO reset clock_APB2_0
  { DTO_CONFIG11_ADDR     , 30,  2, 0x0 }, // DTO reset clock_APB2_2
  { DTO_CONFIG12_ADDR     , 30,  2, 0x0 }, // DTO reset clock_DTO_CPU
  { DTO_CONFIG13_ADDR     , 30,  2, 0x0 }, // DTO reset clock_APB2_4

  { CLOCK_DIV_CONFIG5_ADDR, 30,  2, 0x0 }, // scpu_clk_div_config
  { CLOCK_DIV_CONFIG7_ADDR,  6,  2, 0x0 }, // ssi_clk_div_ratio
};
//struct reg_field clk_mux_bind_tables[][] ={{
//}};

static struct reg_field clk_inv_table[] = {
  { CLOCK_SOURCE2_ADDR    ,  6,  1, 0x1 }, // clock_vpu_dac_soure INV 
  { CLOCK_SOURCE2_ADDR    ,  9,  1, 0x1 }, // adc_clk_inv_sel     INV
  { CLOCK_SOURCE2_ADDR    , 10,  1, 0x1 }  // clock_ADC_clk_edge  INV 
};



static unsigned int get_clk_gate_table_size(void){
  return sizeof(clk_gate_table)/sizeof(struct reg_field);
}
static unsigned int get_clk_mux_table_size(void){
  return sizeof(clk_mux_table)/sizeof(struct reg_field);
}
static unsigned int get_clk_div_table_size(void){
  return sizeof(clk_div_table)/sizeof(struct reg_field);
}
static unsigned int get_clk_dto_table_size(void){
  return sizeof(clk_dto_table)/sizeof(struct reg_field);
}
static unsigned int get_clk_inv_table_size(void){
  return sizeof(clk_inv_table)/sizeof(struct reg_field);
}
//unsigned int get_clk_mux_bind_table_size(void){
//  return sizeof(clk_mux_bind_tables)/sizeof(clk_mux_bind_tables[0]);
//}



/** 
 * @brief field set函数
 *   仅对模型操作，不操作实际寄存器dd
 * @param addr 地址
 * @param site 起始位
 * @param bits 宽度
 * @param data 更新值
 * @return id 
 */
static unsigned int reg_set(unsigned int addr, unsigned int site, unsigned int bits, unsigned int data){
#ifdef CLKMGR_VERI
  unsigned int i;
  unsigned int num = sizeof(reg_array)/sizeof(struct Reg);
  for(i=0; i<num; i++){
    if((!reg_array[i].active)||(reg_array[i].active && reg_array[i].addr == addr)){
      reg_array[i].addr   = addr;
      reg_array[i].data   &= ~((((unsigned long)1<<bits)-1)<<site); // old_data,mask[site+bits-1:site]
      data                &=   (((unsigned long)1<<bits)-1);        // new_data,mask[bits-1:0], <<site
      reg_array[i].data   |=  data<<site;                           // merge old_data,new_data
      reg_array[i].active  = 1;  
      reg_array[i].modify  = 1; // 更新modify标志位，须update方法清掉
      return i;
    }
  }
#else
  unsigned int tmp_data;
  cpu_read(addr, &tmp_data);
  tmp_data       &= ~((((unsigned long)1<<bits)-1)<<site);
  data           &=   (((unsigned long)1<<bits)-1);
  tmp_data       |=   data<<site;
  cpu_write(addr, tmp_data);
#endif
}


/** 
 * @brief 覆写所有已激活且被修改的寄存器
 *   须配合set使用来共同维护modify标志位
 */
static void reg_update_all(void){
#ifdef CLKMGR_VERI
  unsigned int data;
  unsigned int i;
  unsigned int num = sizeof(reg_array)/sizeof(struct Reg);
  for(i=0; i<num; i++){
    if(reg_array[i].active && reg_array[i].modify){
      cpu_write(reg_array[i].addr, reg_array[i].data);
      reg_array[i].modify = 0;
    }
  }
#endif
}

/** 
 * @brief 时钟配置主函数
 */
static void clk_reg_init_all(void){
  unsigned int i;
  struct reg_field  field;

  // div & dto, reset
  for(i=0; i<sizeof(clk_rst_table)/sizeof(struct reg_field); i++){
    field = clk_rst_table[i];
    reg_set(field.addr, field.site, field.bits, field.data);}
  reg_update_all();
  // div & dto, config
  for(i=0; i<sizeof(clk_div_table)/sizeof(struct reg_field); i++){
    field = clk_div_table[i];
    reg_set(field.addr, field.site, field.bits,  field.data);}
  for(i=0; i<sizeof(clk_dto_table)/sizeof(struct reg_field); i++){
    field = clk_dto_table[i];
    reg_set(field.addr, field.site, field.bits,  field.data);}
  // div & dto, reset release
  for(i=0; i<sizeof(clk_rst_table)/sizeof(struct reg_field); i++){
    field = clk_rst_table[i];
    reg_set(field.addr, field.site, field.bits, ~field.data);}
  // mux config
  for(i=0; i<sizeof(clk_mux_table)/sizeof(struct reg_field); i++){
    field = clk_mux_table[i];
    reg_set(field.addr, field.site, field.bits,  field.data);}
  reg_update_all();

  // gate,  config
  for(i=0; i<sizeof(clk_gate_table)/sizeof(struct reg_field); i++){
    field = clk_gate_table[i];
    reg_set(field.addr, field.site, field.bits,  field.data);}
  reg_update_all();
  MY_PRINTF("clk_reg_init_all end...\n");
}


static void pll_config_sm40(unsigned int ADDR, unsigned int FBDIV,unsigned int  REFDIV, unsigned int POSTDIV1, unsigned int POSTDIV2){
  unsigned int lock_status, data;
  // XIN=27MHZ, FOUT=972HMZ
  // FBDIV=36, REFDIV=1, POSTDIV1=1,POSTDIV2=1
  cpu_write(ADDR, (1<<26|1<<27|1<<28|1<<29|1<<30));//pd 
  cpu_read(ADDR,   &data);
  cpu_write(ADDR, (REFDIV<<20|POSTDIV2<<16|POSTDIV1<<12|FBDIV|data));
  cpu_read(ADDR,   &data);
  cpu_write(ADDR, (data&0x03ffffff));//release power down bypass=0;
  //MY_PRINTF("pll_config_sm40 DIV[%.3f]\n", (float)FBDIV/(float)REFDIV/(float)(POSTDIV1*POSTDIV2));
}

static void pll_config(void){
  unsigned int lock_status, data, xtal_define, ddr_fre;
  xtal_define = MY_XTAL_FREQ;
  ddr_fre = DDR_FREQUENCY_CONFIG;

  if(xtal_define == 24){
    pll_config_sm40(DTO_PLL_CONFIG_ADDR,  99, 2, 1, 1); // 24MHZ-1188MHZ
    if (ddr_fre == 533000000) {
      pll_config_sm40(DDR_PLL_CONFIG_ADDR,  22, 1, 1, 1); // 24MHZ-528MHZ
    } else {
      pll_config_sm40(DDR_PLL_CONFIG_ADDR,  27, 1, 1, 1); // 24MHZ-648MHZ
    }
    pll_config_sm40(CPU_PLL_CONFIG_ADDR, 45, 1, 2, 1); // 24MHZ-540MHZ 
  }
  else{
    pll_config_sm40(DTO_PLL_CONFIG_ADDR,  44, 1, 1, 1); // 27MHZ-1188MHZ
    if (ddr_fre == 533000000) {
      pll_config_sm40(DDR_PLL_CONFIG_ADDR,  176, 3, 3, 1); // 27MHZ-528MHZ
    } else {
      pll_config_sm40(DDR_PLL_CONFIG_ADDR,  24, 1, 1, 1); // 27MHZ-648MHZ
    }
    pll_config_sm40(CPU_PLL_CONFIG_ADDR,  20, 1, 1, 1); // 27MHZ-540MHZ
  }
  while(1) {
    cpu_read(PLL_LOCK_IN_ADDR, &data);
    lock_status=(data&0x07);
    if(lock_status==0x07) break;
  }

  MY_PRINTF("-----------PLL_CONFIG---XTAL %0dMHZ-------------\n",xtal_define);
}


void gx_setup_pll_mini_controller(void)
{
    MY_PRINTF("gx_setup_pll_mini_controller begin...\n");

    __raw_writel(0, CONFIG_BASE + CONFIG_SOURCE1_SEL);
    __raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL);
    __raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL2);

    serial_init(1, GX_EXT_CLOCK);
    serial_put_no_mmu('R');

    pll_config();
    clk_reg_init_all();
    cpu_delay_cycle(20);

#ifdef ENABLE_SECURE_ALIGN
    *(volatile unsigned int *)(CONFIG_BASE + CONFIG_CPU_SDC_AXI_8_62) |= 3;
#endif

    serial_init(1, GX_UART_CLOCK);
    serial_put_no_mmu('U');

  MY_PRINTF("gx_setup_pll_mini_controller end...\n");
}
