#ifndef __GX_PADMUX_H__
#define __GX_PADMUX_H__

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


