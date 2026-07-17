/*
 * NationalChip WiFi-Display/Miracast Project
 * Copyright (C) 2001-2024 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * gxwfd.h - WiFi-Display/Miracast API
 *
 */
#ifndef GX_WFD_H
#define GX_WFD_H

/** \addtogroup wfd
 *  @{
 */

/**
 * WFD事件，所有事件都在WFD线程上调用
 **/
typedef enum {
    WFD_EVENT_NULL = 0,                 ///< 空事件
    /*
        WFD的事件流，正常的事件流是：
        WFD_EVENT_START -> WFD_EVENT_CONNECTED ->
        WFD_EVENT_RESOLUTION_SET -> WFD_EVENT_HDCP_PORT -> WFD_EVENT_START_PLAYER
    */
    WFD_EVENT_START,                    ///< 启动完成事件
    WFD_EVENT_STOP,                     ///< 关闭完成事件
    WFD_EVENT_CONNECT_SUCC,             ///< 与Source连接成功
    WFD_EVENT_CONNECT_FAIL,             ///< 与Source连接失败
    WFD_EVENT_DISCONNECTED,             ///< 与Source断开连接
    WFD_EVENT_RESOLUTION_SET,           ///< Source设置了分辨率 WFD_RESOLUTION
    WFD_EVENT_HDCP_PORT,                ///< 报告HDCP端口
    WFD_EVENT_START_PLAYER,             ///< 开始播放
    WFD_EVENT_STOP_PLAYER,              ///< 结束播放
    WFD_EVENT_TIMEOUT,                  ///< RTSP层的超时

    /* WiFi的事件流 */
    WFD_EVENT_WIFI_READY,               ///< WiFi初始化成功
    WFD_EVENT_WIFI_INVALID,             ///< 没有发现WiFi设备
    WFD_EVENT_WIFI_SET_SSID_DONE,       ///< 设置SSID完成之后
    WFD_EVENT_WIFI_PLUGOUT,             ///< 发现P2P设备被移除

    /*
        P2P方式事件流，正常的事件流是：
        WFD_EVENT_P2P_SCAN_START -> WFD_EVENT_P2P_GO_NEGO_SUCC -> WFD_EVENT_P2P_WSC_EXCHANGE_SUCC
        -> WFD_EVENT_P2P_CONNECT_SUCC -> WFD_EVENT_DHCP_BEGIN -> WFD_EVENT_DHCP_SUCC
        -> WFD_EVENT_CONNECTED
     */
    WFD_EVENT_P2P_SCAN_START,           ///< P2P开始扫描
    WFD_EVENT_P2P_GO_NEGO_SUCC,       	///< GO Nego成功，会进入WSC Exchange阶段（data是对方的SSID字符串）
    WFD_EVENT_P2P_GO_NEGO_FAIL,         ///< GO Nego失败
    WFD_EVENT_P2P_GO_INVITE_SUCC,       ///< （data是对方的SSID字符串）
    WFD_EVENT_P2P_GO_INVITE_FAIL,       ///<
    WFD_EVENT_P2P_WSC_EXCHANGE_SUCC,    ///< WSC Exchange成功，会进入P2P连接阶段（data是对方的SSID字符串）
    WFD_EVENT_P2P_WSC_EXCHANGE_FAIL,    ///< WSC Exchange失败
    WFD_EVENT_P2P_CONNECT_SUCC,         ///< P2P连接成功，会进入通过DHCP获取IP阶段（data是对方的SSID字符串）
    WFD_EVENT_P2P_CONNECT_FAIL,         ///< P2P连接失败
    WFD_EVENT_P2P_DISCONNECT,           ///< Source断开P2P连接
    WFD_EVENT_P2P_DHCP_BEGIN,           ///< 开始通过DHCP获得IP地址
    WFD_EVENT_P2P_DHCP_SUCC,            ///< 成功通过DHCP获得IP地址，会开始连接RTSP服务
    WFD_EVENT_P2P_DHCP_FAIL,            ///< 通过DHCP获得IP地址失败
    WFD_EVENT_P2P_TIMEOUT,              ///< P2P连接过程发生超时情况

    /*
        MICE方式事件流，TODO
     */
    WFD_EVENT_MICE_CONNECTED,           ///< MICE连接成功
    WFD_EVENT_MICE_DISCONNECTED,        ///< MICE断开连接
    WFD_EVENT_MICE_SOURCE_READY,        ///< MICE收到SOURCE准备好消息（data是对方的FriendlyName字符串）
    WFD_EVENT_MICE_STOP_PROJECT,        ///< MICE收到Stop投影消息
    WFD_EVENT_MAX,
} WFD_EVENT;

/**
 * @brief 事件回调函数定义
 *
 * @param event 事件类型
 * @param data 事件所携带的信息
 * @return 返回结果（目前不支持）
 */
typedef int (*WFD_EVENT_CALLBACK)(const WFD_EVENT event , const void *data);

/**
 * @brief 系统初始化，初始化内部的数据结构
 *
 * @return int
 */
int gxwfd_init(const char *dev);

/**
 * @brief 系统结束
 * @details 如果正在运行，它会做stop操作
 *
 * @return int
 */
int gxwfd_exit(void);

/**
 * @brief 启动后台线程
 *
 * @param callback
 * @param dev 设备名
 * @param ssid 友好名称
 * @param channel 频道
 * @return int
 */
int gxwfd_start(WFD_EVENT_CALLBACK callback, const char *ssid, int channel);

/**
 * @brief 关闭后台线程
 *
 * @return int
 */
int gxwfd_stop(void);

/*------------ TODO -----------*/

/**
 * HDCP2密钥
 **/
typedef struct {
    unsigned char aes_key[16];
    unsigned char riv[8];
} WFD_HDCP2_KEY;

/**
 * @brief 更新HDCP密钥
 *
 * @param key
 * @return int
 */
int gxwfd_update_hdcp_key(WFD_HDCP2_KEY *key);

/** @}*/
#endif // GX_WFD_H
