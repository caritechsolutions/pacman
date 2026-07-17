#ifndef __GXCAS_DESCRAMBLER_H__
#define __GXCAS_DESCRAMBLER_H__

#include "gxtype.h"
#include "dvbhal/gxdescrambler_hal.h"
#include "gxsecure/CA/gxifcp_api.h"

#define GXCA_MAX_DESCRAMBLER_NUM (8)

/**
 * GxCas_Deschal_Init() - 初始化descrambler hal模块
 * descrambler hal模块入口，只需要初始化一次,descrambler hal模块支持多线程
 *
 *\param param 初始化参数
 *
 *\return
 *\retval 0  初始化成功
 *\retval -1 初始化失败
 */
int32_t GxCas_Deschal_Init(GxDescInitParam *param);

/**
 * GxCas_Deschal_Open() - 申请一个descrambler
 * 支持申请多个descrambler同时工作，并且可以切换不同模式
 *
 *\param mod: descrambler硬件模块类型
 *\param id : 对应的解扰器模块id号(Demux_ID, GP_ID)
 *\param alg: 当前descrambler使用的算法配置
 *
 *\return
 *\retval != -1 申请成功
 *\retval -1    申请失败
 */
handle_t GxCas_Deschal_Open(GxDescMod mod, uint32_t id, GxDescAlg alg);

/**
 * GxCas_Deschal_Close() - 关闭一个descrambler
 * 使用完毕需要及时关闭descrambler，否则可能会导致申请不到descrambler
 *
 *\param handle: descrambler handle
 *
 *\return void
 */
void GxCas_Deschal_Close(handle_t handle);

/**
 * GxDescConfig() - 配置descrambler
 *
 *\param handle: descrambler handle
 *\param alg   : 当前descrambler使用的算法配置    gxdocref GxDescAlg
 *\param klm   : 当前descrambler使用的KLM模块     gxdocref GxDescKLM
 *\param klm_level : 当前descrambler使用的KLM模块阶数
 *
 *\return void
 */
void GxCas_Deschal_Config(handle_t handle, GxDescAlg alg, GxDescKLM klm, int klm_level);

/**
 * GxDescSelectRootkey() - 选择使用第几个ROOTKEY
 *
 *\param handle: descrambler handle
 *\param rootkeyid : rootkey index
 *\param rootkeysub: rootkey subid
 *
 *\return
 *\retval 0  成功
 *\retval -1 失败
 */
int32_t GxCas_Deschal_SelectRootkey(handle_t handle, uint32_t rootkeyid, uint32_t rootkeysub);

/**
 * GxDescSetKn() - 配置一次待解数据
 * 该接口允许多次调用,需要根据实际情况确定key_level和data的对应关系
 *
 *\param handle: descrambler handle
 *\param key_level: 配置的数据工作在第几次运算
 *\param alg: 使用该次数据需要的算法              gxdocref GxDescKLMAlg
 *\param EKn: 输入数据
 *\param len: 输入数据长度
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_SetKn(handle_t handle, uint32_t key_level, GxDescKLMAlg alg, uint8_t* EKn, uint32_t len);

/**
 * GxCas_Deschal_SetCw() - 配置加密cw数据
 * 此接口将根据handle对应的descrambler配置计算出最终的明文cw，并配置到解扰器
 * 此接口仅会同时更新EVEN CW/ODD CW，使用MSR3 klm时API只会参照param->ifcp_set_even属性
 *
 *\param handle: descrambler handle
 *\param pid: 被加扰数据的pid
 *\param alg: 使用ecw需要的算法             gxdocref GxDescKLMAlg
 *\param param: cw数据及相关参数            gxdocref GxDescCWParam
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_SetCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param);

/**
 * GxCas_Deschal_SetEvenCw() - 配置加密cw数据
 * 此接口将根据handle对应的descrambler配置计算出最终的明文cw，并配置到解扰器
 * 此接口仅更新EVEN CW
 *
 *\param handle: descrambler handle
 *\param pid: 被加扰数据的pid
 *\param alg: 使用ecw需要的算法             gxdocref GxDescKLMAlg
 *\param param: cw数据及相关参数            gxdocref GxDescCWParam
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_SetEvenCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param);

/**
 * GxCas_Deschal_SetOddCw() - 配置加密cw数据
 * 此接口将根据handle对应的descrambler配置计算出最终的明文cw，并配置到解扰器
 * 此接口仅更新ODD CW
 *
 *\param handle: descrambler handle
 *\param pid: 被加扰数据的pid
 *\param alg: 使用ecw需要的算法             gxdocref GxDescKLMAlg
 *\param param: cw数据及相关参数            gxdocref GxDescCWParam
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_SetOddCw(handle_t handle, uint16_t pid, GxDescKLMAlg alg, GxDescCWParam *param);

/**
 * GxCas_Deschal_UpdateTDparam() - 更新一些CA厂商特有的TD参数
 *
 *\param handle: descrambler handle
 *\param param: cw数据及相关参数            gxdocref GxDescCWParam
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_UpdateTDparam(handle_t handle, GxDescCWParam *param);

/**
 * GxCas_Deschal_AddPID() - 添加PID到解扰器
 *
 *\param handle: descrambler handle
 *\param pid:    用户指定的PID
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_AddPID(handle_t handle, uint16_t pid);

/**
 * GxCas_Deschal_DelPID() - 删除解扰器配对的PID
 *
 *\param handle: descrambler handle
 *\param pid:    用户指定的PID
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int32_t GxCas_Deschal_DelPID(handle_t handle, uint16_t pid);

int32_t GxCas_Deschal_Init(GxDescInitParam *param);
int32_t GxCas_Desc_Init(uint32_t dmxid);
int32_t GxCas_Desc_Open(void);
void GxCas_Desc_Close(int32_t descid);
int32_t GxCas_Desc_SetCW(int32_t descid, uint16_t pid, uint8_t *oddkey, uint8_t *evenkey, uint32_t keyLen);
/**
 * GxCas_Deschal_IFCP_LoadAPP() - irdeto ca, load ifcp image
 *
 *\param GxIfcpApp: ifcp param
 *
 *\return
 *\retval  0 配置成功
 *\retval -1 配置失败
 */
int GxCas_Deschal_IFCP_LoadAPP(GxIfcpApp *app);
#endif

