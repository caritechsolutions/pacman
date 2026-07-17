#ifndef __CYGNUS_PADMUX_H__
#define __CYGNUS_PADMUX_H__

#define __raw_writel(val, addr) \
		(void)((*(volatile unsigned int *)(unsigned int)(addr)) = (unsigned int)(val))

#define __raw_readl(addr) \
	  ({ unsigned int __v 	= (*(volatile unsigned int *) (addr)); __v; })

/**
 * @brief 配置管脚复用功能
 *
 * @param pad_id pin 脚号
 * @param function 复用功能
 * @return int 是否成功
 * @retval 0 成功
 * @retval -1 失败
 */
int padmux_set(int pad_id, int function);

/**
 * @brief 获取管脚复用功能
 *
 * @param pad_id pin 脚号
 * @return int 是否成功
 * @retval 0 成功
 * @retval -1 失败
 */
int padmux_get(int pad_id);

/**
 * @brief 管脚复用功能检查
 *
 * @param pad_id pin 脚号
 * @param function 复用功能
 * @return int 检测是否正常
 * @retval 0 正常
 * @retval -1 异常
 */
int padmux_check(int pad_id, int function);

#endif
