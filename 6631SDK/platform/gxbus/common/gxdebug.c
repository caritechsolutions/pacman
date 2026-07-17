#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "gxdebug.h"

//#define HAVE_PORTING_DEBUG
static GXDBG_CONF_T debug_config[GXDBG_MODULE_MAX];

static int debug_inited = 0;

#define MSG_BUFFER_MAX 100
#define MSG_LEN_MAX 512

/******************************************************************************
 * 进行默认设置
 *****************************************************************************/
static void debug_config_set_default(void)
{
	int i;
	GXDBG_CONF_T *config = debug_config;
	config->m_level = (char)GXDBG_LEVEL_WARN;
	config++;

	for(i = 1;i < GXDBG_MODULE_MAX;i++)
	{
		config->m_level =(char)GXDBG_LEVEL_NONE;   //默认设置各模块打印级别遵从整体打印级别
		config++;
	}
	return;
}

/******************************************************************************
 * 函数原型：int gx_debug_set_level (PORTING_MODULE_DEBUG_T id, PORTING_DEBUG_LEVEL_T lv)
 * 功能说明：根据模块的id，在缓存数据块中找到该模块数据保存位置，更新该模块设置的debug输出级别，
 * 然后将缓存数据块内容提交，调用NVRAM模块中ipanel_gx_nvram_burn接口写入flash。
 * 输入参数：
 * 　　　id：模块号，为0时进行全局设置
 *           lv：模块设置的debug级别
 * 输出参数：
 * 　　　无
 * 返回值：
 * 　　　GXDBG_OK ：成功
 * 　　　GXDBG_ERR：失败
 *****************************************************************************/
int gx_debug_set_level (GXDBG_MODULE_T id, GXDBG_LEVEL_T lv)
{
	/* 判断输入参数有效性*/
	if((id < GXDBG_MODULE_DEFAULT)||(id > GXDBG_MODULE_OTHER))
	{
		gxlogd("[gx_debug_set_level]not this module id=%d exit\n",id);
		return GXDBG_ERR;
	}

	if((lv < GXDBG_LEVEL_NONE)||(lv > GXDBG_LEVEL_FATAL))
	{
		gxlogd("[gx_debug_set_level]cannot set module level %d\n",lv);
		return GXDBG_ERR;
	}
	if(id == GXDBG_MODULE_DEFAULT)
	{
		if(lv == GXDBG_LEVEL_NONE)
		{
			gxlogd("[gx_debug_set_level]cannot set GXDBG_MODULE_DEFAULT module level %d\n",lv);
			return GXDBG_ERR;
		}
	}
	/*设置模块打印级别*/
	debug_config[id].m_level = (char)lv;
	return GXDBG_OK;
}

/******************************************************************************
 * 函数原型：int gx_debug_get_level (PORTING_MODULE_DEBUG_T id, int * lv)
 * 功能说明：根据模块id，在缓存数据块中找到该模块数据保存位置，获取该模块设置的debug输出级别，存放在lv指向的空间中。
 * 输入参数：
 * 　　　id：模块号，为0时返回全局设置
 * 输出参数：
 * 　　　lv：指向存有模块debug等级信息的数据块
 * 返回值：
 * 　　　GXDBG_OK ：成功
 * 　　　GXDBG_ERR：失败
 *
 *****************************************************************************/
int gx_debug_get_level (GXDBG_MODULE_T id, int *lv)
{
	/* 判断输入参数有效性*/
	if((id < GXDBG_MODULE_DEFAULT)||(id > GXDBG_MODULE_OTHER))
	{
           gxlogd("[gx_debug_get_level]not this module id=%d exit\n",id);
           return GXDBG_ERR;
    }

	/*设置模块打印级别*/
	int i = id;
	*lv = debug_config[i].m_level;
	return GXDBG_OK;
}

/******************************************************************************
 * 函数原型：int gx_debug_set_output(ORT_DBG_OUTPUT_MODE_T param)
 * 功能说明：根据输入的param值，设置模块的打印输出级别,并将更新后模块打印输出级别信息写入到flash。
 * 输入参数：
 *          pram   ：模块输出方式，默认为串口输出；
 * 返回值：
 *       GXDBG_OK ：成功
 * 　　  GXDBG_ERR：失败
 *
 *****************************************************************************/
int gx_debug_set_output(GXDBG_OUTPUT_MODE_T param)
{
	//int ret;

	return GXDBG_OK;
}

/******************************************************************************
 * 函数原型：int gx_debug_get_output(ORT_DBG_OUTPUT_MODE_T *param)
 * 功能说明：获取调试信息当前的输出方式，存放在param指向的空间中。
 * 输出参数：
 *         param：指向用于返回模块输出方式的数据空间。
 * 返回值：
 *         GXDBG_OK ：成功
 * 　　　  GXDBG_ERR：失败
 *
 *****************************************************************************/
int gx_debug_get_output(GXDBG_OUTPUT_MODE_T *param)
{
	//int ret;
	*param = GXDBG_OUTPUT_SERIAL;

	return GXDBG_OK;
}

/******************************************************************************
 * 函数原型：int gx_debug_output(PORTING_MODULE_DEBUG_T id, PORTING_DEBUG_LEVEL_T level,
 *                         const char *file, int line, const char *function, const char *fmt,….)
 * 功能说明：根据id和level的组合，判断是否符合输出条件。条件符合时，将file、line、function
 *           和fmt后的实际debug信息，进行组合后输出。
 * 输入参数：
 * 　　id      ：模块号
 * 　　level   ：打印级别
 *     file    ：文件名及路径
 *     line    ：当前打印所在行数
 *     function：当前打印所在函数
 *     fmt     ：格式化字符串。参数说明参照标准的gxlogd函数
 * 输出参数：
 * 　　无
 * 返回值：
 *      >0: 实际输出的字符数；
 *      =0：不符合打印级别设置，不做真实输出。
 *          GXDBG_ERR: 函数异常返回。
 *******************************************************************************/
int gx_debug_output(GXDBG_MODULE_T id, GXDBG_LEVEL_T  level, const char *file, int line, const char *function, const char *fmt,...)
{
	int ret = 0;
	int mlevel = -2,glevel = -2;
	va_list args;

	const char *level_str[] = {
			"VERB",
			"INFO",
			"WARN",
			"FAIL",
			"ERROR",
			"FATAL"};

	if(debug_inited == 0)
		gx_debug_init();

	if((level < GXDBG_LEVEL_VERB)||(level > GXDBG_LEVEL_FATAL))
	{
		gxlogd("cannot match the level %d\n",level);
		return GXDBG_ERR;

	}

	ret = gx_debug_get_level(id, &mlevel);

	if((ret == GXDBG_ERR)||(mlevel < GXDBG_LEVEL_NONE)||(mlevel > GXDBG_LEVEL_FATAL))
	{
		return GXDBG_ERR;
	}

	if(mlevel == GXDBG_LEVEL_NONE)
	{
		ret = gx_debug_get_level(0,&glevel);
		if((ret == GXDBG_ERR)||(glevel <= GXDBG_LEVEL_NONE)||(glevel > GXDBG_LEVEL_FATAL))
		{
			return GXDBG_ERR;
		}
		if(level < glevel)
			return 0;
	}
	else
	{
		if(level < mlevel)
			return 0;
	}

	/* 组织打印语句内容*/
	if (level < 6)
		gxlogf ("[%s]", level_str[level]);


	if (file) {
		gxlogf("[%s", file);
		if (line > 0)
			gxlogf(":%d",line);
		gxlogf("]");
	}
	else if (line > 0)
		gxlogf ("[%d]",line);

	if(function != NULL)
		gxlogf( "[%s]",function);

	va_start(args,fmt);
	vprintf(fmt, args);
	va_end(args);

	return ret;
}

/******************************************************************************
 * 函数原型：int gx_debug_init(VOID)
 * 功能说明： 模块启动时从flash中读取各个模块打印级别设置信息,并将读取到的数据放置在专用的缓存空间debug_config中。
 * 当读取flash中数据失败或系统不支持读取flash内设置数据时，初始化操作会自动将缓存空间中各个模块的打印级别设置为
 * 默认值，同时提醒用户flash中没有保存模块打印级别设置的配置文件。
 *
 * 输入参数：
 *        无
 * 返回值：
 *       GXDBG_OK ：成功
 * 　　 GXDBG_ERR：失败
 *****************************************************************************/
int gx_debug_init(void)
{
	memset(debug_config,0,sizeof(debug_config));

	debug_config_set_default();

	gx_debug_set_level(GXDBG_MODULE_DEFAULT,GXDBG_LEVEL_VERB);

	gx_debug_set_level(GXDBG_MODULE_PLAYER,GXDBG_LEVEL_VERB);

	debug_inited = 1;

	return GXDBG_OK;
}


/******************************************************************************
 * 函数原型：int gx_debug_exit(VOID)
 * 功能说明： 模块退出，不做其他操作。
 * 输入参数：
 *        无
 * 返回值：
 *        GXDBG_OK ：成功
 * 　　   GXDBG_ERR：失败
******************************************************************************/
int gx_debug_exit(void)
{
	return GXDBG_OK;
}

int gx_debug_ctr(int flag)
{
	if(1<flag)
	{
		return GXDBG_ERR;
	}
	return GXDBG_OK;
}
