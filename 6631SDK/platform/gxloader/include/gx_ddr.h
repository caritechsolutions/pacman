#ifndef __GX_DDR_H__
#define __GX_DDR_H__

/*
 * gx_ddr_get_phase_test
 * 返回值 
 * 0: 表示 DDR 相位测试正常,DDR 正常
 * 1: 表示 DDR 相位偏窄，DDR 不稳定
 * 2: 表示 DDR 相位测试失败,DDR异常
 * */
int gx_ddr_get_phase_test(void);

#endif
