#ifndef _GUI_TIMER_H__
#define _GUI_TIMER_H__

/** \addtogroup <label> 
 * @{
 */

#include <stdio.h>
#include <stdlib.h>

//#include "SDL.h"
//#include <wchar.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "gxcore.h"

__BEGIN_DECLS

/**
 *
 * 定时器模式：
 */
typedef enum {
	TIMER_ONCE, /**< 运行一次即删除（应用不用删除）*/
	TIMER_REPEAT, /**< 运行多次，知道remove_timer函数被调用 */
	TIMER_REPEAT_NOW /**< 创建完后即运行一次，接下来与TIMER_REPEAT一样 */
}TIMER_FLAG;

/**
 *
 * \brief 定时器回调函数原型
 */
typedef int (*timer_event) (void *userdata);


/**
 *
 * \brief 定时器结构
 */
typedef struct eventlist{
	timer_event  event;   /**<回调函数体 */
	void*        data;  /**<回调参数 */
	int trigger_time;  /**<触发时间 */
	int delta_time;  /**<定时时间 */
	int flag;  /**<定时器模式 */
	int active;  /**<定时器是否激活 */
	int handle;  /**<定时器句柄 */
	struct eventlist* next;  /**<用于定时器链表 */
}event_list;

/**
 *
 * 创建定时器
 *\param event 定时时间函数
 *\param time 定时时间(毫秒)
 *\param data 定时器执行参数
 *\param flag 定时器执行模式
 *\return 定时器句柄
 *\retval 非NULL 定时器执行成功
 *\retval NULL 定时失败
 */
event_list* create_timer(timer_event event, int time, void *data,  TIMER_FLAG flag);

/**
 * 重启定时器
 *\param pTimer 定时器句柄
 *\return 是否成功重启定时器
 *\retval 0 重启成功
 *\retval 非0 重启定时器失败
 */
int reset_timer(event_list *pTimer);

/**
 * 移除定时器
 * \param pTimer 定时器句柄
 * \return 是否成功移除定时器
 * \retval 0 成功移除定时器
 * \retval 非0 移除定时器失败
 *
 */
int remove_timer(event_list *pTimer);
int timer_exec(void);

/**
 * 定时器停止（stop的能重新reset，remove的不能重新reset）
 * \param pTime 定时器句柄
 * \return 是否成功停止定时器
 * \retval 0 成功停止定时器
 * \retval 非0 停止定时器失败
 */
int timer_stop(event_list *pTimer);

/**
 * 移除所有定时器（注意野指针管理）
 * \param 无
 * \return 移除是否成功
 * \retval 0 移除成功
 * \retval 非0 移除失败
 */
int remove_timer_list(void);

/**
 * 延时函数
 * \param ms 延时时间（毫秒）
 * \return 无
 */
void gui_delay(unsigned int ms);

/**
 * 获取当前时间
 * \param 无
 * \return 当前时间（毫秒）
 */
unsigned int gui_get_current_time(void);

__END_DECLS

/** @}*/

#endif

