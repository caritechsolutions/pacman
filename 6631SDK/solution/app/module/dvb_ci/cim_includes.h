#ifndef __CIM_INCLUDES_H_INCLUDED__
#define __CIM_INCLUDES_H_INCLUDED__

#define _CIM_DEBUG_ENABLE_ (1)


typedef unsigned char cim_u8;   // 8 位无符号整型
typedef unsigned int  cim_uint; // 无符合整型
typedef char          cim_char; // 字符型
typedef void          cim_void; // 空类型
typedef unsigned int  cim_bool; // 布尔型
typedef int           cim_int;//

#ifndef TRUE
#define TRUE (0 == 0)
#endif

#ifndef FALSE
#define FALSE !TRUE
#endif

// 入口项状态
enum ENTRY_STATE
{
	ES_NO_CARD,              // 卡座内 CAM 卡
	ES_INITIALIZING,         // 正在初始化
	ES_ESTABLISHED,          // 确定状态
	ES_WAIT,                 // 等待建立 MMI
	ES_TIMEOUT,              // 建立 MMI 超时
};

// 人机接口状态
enum MMI_STATE
{
	MS_EXIT  = 0x00,         // 退出状态
	MS_MENU  = 0x09,         // 菜单状态
	MS_LIST  = 0x0C,         // 列表状态
	MS_ENQ   = 0x07,         // 查询状态
	MS_DELAY = 0x01,         // 延时状态
};

// 字符串索引
enum STRING_INDEX
{
	INDEX_ENQ_TEXT    = 0,   // 请求字符串
	INDEX_TITLE       = 0,   // 主标题
	INDEX_SUB_TITLE,         // 子标题
	INDEX_BOTTOM_TEXT,       // 底部文本
	INDEX_ITEM_TEXT,         // 选项文本
};

// 给CIM模块更新PMT
enum UPDATE_PMT_CIM_NUM
{
	UPDATE_PMT_CIM_0 = 0,
	UPDATE_PMT_CIM_1 = 1,
	UPDATE_PMT_CIM_2 = 2,
	UPDATE_PMT_CIM_NULL = 4,
};

// 模块的执行线程，永远都不会返回
extern cim_void cim_daemon_thread(cim_void);

// 测试状态是否发生改变
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 改变时(即需要刷新界面时)返回 TRUE
extern cim_bool cim_is_state_changed(cim_uint nSlot);

// 获取状态
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回当前显示状态
extern cim_uint cim_get_state(cim_uint nSlot);

// 获取条件接收模块名字
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回名字首地址
extern const cim_char *cim_get_name(cim_uint nSlot);

// 打开人机接口会话
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
extern cim_void cim_open_mmi(cim_uint nSlot);

// 更新 PMT 表
// nID  PP 标识
// pPMT PMT 表的原始数据
extern cim_void cim_update_pmt(const cim_u8 *pPMT,int len, cim_uint nCimNum);

// 检查模式是否发生改变
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 变化时返回 TRUE
cim_bool cim_is_mode_changed(cim_uint nSlot);

// 获取当前显示模式
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回当前显示模式
cim_uint cim_get_mode(cim_uint nSlot);

// 获取界面延迟关闭的时间
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回延时时间，单位秒
cim_uint cim_get_delay_time(cim_uint nSlot);

// 获取菜单项数
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回菜单项数
cim_uint cim_get_menu_cnt(cim_uint nSlot);

// 获取列表项数
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回列表项数
cim_uint cim_get_item_cnt(cim_uint nSlot);

// 检查是否需要隐藏输入
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 当需要隐藏时，返回 TRUE
cim_bool cim_is_blind_answer(cim_uint nSlot);

// 获取输入长度
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// 返回长度
cim_uint cim_get_answer_length(cim_uint nSlot);

// 获取字符串
// nSlot  要测试的通道号，应小于 _CIM_CAM_CNT_
// nIndex 字符串的序号
// 返回字符串指针
const cim_char *cim_get_text(cim_uint nSlot, cim_uint nIndex);

// 设置选择
// nSlot  要测试的通道号，应小于 _CIM_CAM_CNT_
// nRef   选中的选项序号，从 1 开始
cim_void cim_set_choice(cim_uint nSlot, cim_uint nRef);

// 设置应答
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
// pBuff 数据缓冲指针，为 NULL 时被认为是取消应答
// nLen  数据长度，大于 _CIM_MAX_INPUT_SIZE_ 时将被忽略
cim_void cim_set_answer(cim_uint nSlot, const cim_char *pBuff, cim_uint nLen);

// 关闭 MMI
// nSlot 要测试的通道号，应小于 _CIM_CAM_CNT_
cim_void cim_close_mmi(cim_uint nSlot);

#endif // #ifndef __CIM_INCLUDES_H_INCLUDED__
///////////////////////////////////////////////////////////////////////////////
// end of cim_includes.h

