#ifndef __VOD_CONTROL_API_H_
#define __VOD_CONTROL_API_H_

typedef void (*vodcontrolcallback)(void);


/******************************************************************
说明:
初始化VOD模块内容
名称:
 STB_VOD_Init
参数:
protocol:使用的控制协议
play_type:业务类型
model_priority:模块任务级别
callback:事件回掉函数
********************************************************************/
int32_t STB_VOD_Init(void);

int32_t STB_VOD_Exit(void);
/**********************************************************
说明：
开始点播一部影片，通过调用本接口，VOD将会和SERVER进行指令交互，如果指令交互成功并且开始播放节目，函数成功返回，否则返回错误代码。
定义：
int32_t STB_VOD_Play( vod_start_param_t* param, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
参数：

返回：
VOD错误代码
*********************************************************/
int32_t STB_VOD_Play( vod_start_param_t* param, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime );

/**********************************************************
说明：
暂停当前播放的节目到某一点或者当前正在播放的点。
定义：
int32_t STB_VOD_Pause(uint32_t position, uint32_t time_out);
参数：
Position:如果是0，表示立即暂停到当前播放的点，如果>0，表示当节目播放到position点的时候暂停。
Time_out:超时时间，如果为0，则使用使用VOD默认超时时间。
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Pause(uint32_t position, uint32_t time_out);

/**********************************************************
说明：
调用本接口时候，节目应该是处在暂停状态，否则可能出错。
定义：
int32_t STB_VOD_Resume ( uint32_t position, uint32_t time_out );
参数:
Position:如果为0，表示节目从暂停点开始播放，如果不为0，表示节目从position点开始播放，而不是从暂停的点开始播放。此时操作相当于resume(0) + seek(position),有些SERVER支持这种暂停和恢复的操作模式。
Time_out:超时时间，单位豪秒。
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Resume ( uint32_t position, uint32_t time_out );


/**********************************************************
说明：
节目跳转操作。
定义：
int32_t STB_VOD_Seek(uint8_t* dstposition, VOD_RANGE_TYPE_t rtype, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
参数：
Dstposition:节目要跳转的目的时间点。
rtype:range 字段的类型，目前支持两种 npt类型＆clock类型
Time_out：同上
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Seek(char* dstposition, VOD_RANGE_TYPE_t rtype, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime );


/**********************************************************
说明：
节目跳转操作。
定义：
int32_t STB_VOD_Seek_Ex(uint32_t srcposition, uint32_t dstposition, uint32_t time_out);
参数：
Srcposition: 如果=0 表示节目从当前正在播放的点跳转到dstposition点开始播放。
如果>0表示当节目播放到srcposition点的时候，直接跳转到
dstposition继续进行播放。
Dstposition:节目要跳转的目的时间点。
Time_out：同上
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Seek_Ex(uint32_t srcposition, uint32_t dstposition, uint32_t time_out);

/**********************************************************
说明：
本函数用于调整正在播放的节目的播放速度。正常速度为0，大于0的速度表示快速向前进行播放，小于0的速度表示快速向后进行播放。
定义：
int32_t STB_VOD_Scale( int speed, uint32_t time_out );//speed
参数：
Speed:目标播放速度，服务器可能只支持几种播放速度，当设置一个speed后，服务器会选择最接近speed的速度进行播放。
Time_out:同上
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Scale( int speed, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime );

/**********************************************************
说明：
退出正在播放的节目。
定义：
int32_t STB_VOD_Stop( uint32_t time_out );
参数：
无
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Stop( uint32_t time_out );

/**********************************************************
说明：
进行getparameter操作
定义：
int32_t STB_VOD_Getparam( uint32_t time_out );
参数：
无
返回：
VOD错误代码

*********************************************************/
int32_t STB_VOD_Getparam( uint32_t time_out );

/**********************************************************
说明：
此接口用于获得当前播放的节目的时间信息，主要包括开始时间，结束时间，当前音频解码器时间，当前视频解码器时间，在使用本函数的时候，如果只想获得其中的某个参数而不是全部，可以把不想取的参数用NULL。
定义：

参数：
Start:返回当前节目的开始时间，在点播中应该是相对时间，通常为0，在时移中应该是绝对时间。如果不想获取本参数，在调用的时候可以使用NULL.
End:返回当前节目的结束时间，在点播中应该是相对时间，通常为0，在时移中应该是绝对时间。如果不想获取本参数，在调用的时候可以使用NULL.
current:当前播放时间
Aud_dec_time:返回音频解码器当前时间，如果不想获得本参数，可是使用NULL。
Vid_dec_time: 返回视频解码器当前时间，如果不想获得本参数，可以使用NULL。

*********************************************************/
int32_t STB_VOD_Info_Position(int32_t* start, int32_t* end, int32_t * current, int32_t * aud_dec_time, int32_t* vid_dec_time);

/**********************************************************
说明：
本函数用于获取VOD核心模块当前的状态信息。
定义：
int32_t STB_VOD_Info_Buffersize(&iShowBufferSizeTmp, NULL);
参数：
Status:返回VOD核心模块的状态

*********************************************************/

int32_t STB_VOD_Info_Buffersize(uint32_t * vsize, uint32_t * asize);

/**********************************************************
说明：
本函数用于获取VOD核心模块当前的状态信息。
定义：
int32_t STB_VOD_Info_Buffersize(&iShowBufferSizeTmp, NULL);
参数：
Status:返回VOD核心模块的状态

*********************************************************/

int32_t STB_VOD_Info_BufferFreesize(uint32_t * vsize, uint32_t * asize);


/**********************************************************
说明：
本函数用于获取VOD核心模块当前的状态信息。
定义：
int32_t  STB_VOD_Info_Status ( VOD_MANAGER_STATUS_t * status );
参数：
Status:返回VOD核心模块的状态

*********************************************************/
VOD_MANAGER_STATUS_t STB_VOD_Info_Status ( void );


/**********************************************************
说明：
获取当前节目音频编码类型。
定义：
VOD_AudioFormat_t STB_VOD_Info_Aud_Type(void)

*********************************************************/
VOD_AudioFormat_t STB_VOD_Info_Aud_Type(void);

/**********************************************************
说明：
获取当前节目的视频编码格式。
定义：
int32_t STB_VOD_Info_Vid_Type(DRV_VideoFormat_t * type);

*********************************************************/
VOD_VideoFormat_t STB_VOD_Info_Vid_Type(void);

/**********************************************************
说明：
获取音频频率。
定义：
int32_t STB_VOD_Info_Aud_Freq( uint32_t * freq);

*********************************************************/
int32_t STB_VOD_Info_Aud_Freq( uint32_t * freq);

/**********************************************************
说明：
获取音频的声道信息。
定义：
int32_t STB_VOD_Info_Aud_Channel(uint32_t * left, uint32_t* right);
参数：
Left：如果左声道返回1，否则返回0
Right:如果右声道返回1，否则返回0
如果是立体声，二者都返回1
返回：
错误代码

*********************************************************/
int32_t STB_VOD_Info_Aud_Channel(uint32_t * left, uint32_t* right);

/**********************************************************
说明：
获得视频解码器的配置参数。
定义：
int32_t STB_VOD_Info_Vid_Config(
uint8_t * configstr, 
uint32_t* configlen, 
uint32_t configmaxlen );
参数：
Configstr:返回字符串的保存地址
Configlen:返回的音频配置字符串长度
Configmaxlen:能够接收的字符串最大长度
返回：
错误代码

*********************************************************/
int32_t STB_VOD_Info_Vid_Config(
										uint8_t * configstr, 
										uint32_t* configlen, 
										uint32_t configmaxlen );

/**********************************************************
说明：
获取音频侦的侦率
定义：
int32_t STB_VOD_Info_Aud_FrameRate( uint32_t* rate );
参数：
Rate:返回音频侦率

*********************************************************/
int32_t STB_VOD_Info_Aud_FrameRate( uint32_t* rate );


/**********************************************************
说明：
获取视频侦的侦率
定义：
int32_t STB_VOD_Info_Vid_FrameRate ( uint32_t* rate );
参数：
Rate:返回视频侦率

*********************************************************/
int32_t STB_VOD_Info_Vid_FrameRate ( uint32_t* rate );

/**********************************************************
说明：
获取在VOD的数据队列中保存的视频RTP的数量
定义：
int32_t STB_VOD_Info_Vid_PacketNum( uint32_t* num);
参数：
Num:返回视频RTP的数量
返回：
错误代码

*********************************************************/
int32_t STB_VOD_Info_Vid_PacketNum( uint32_t* num);

/**********************************************************
说明：
获取在VOD的数据队列中保存的视频RTP的数量
定义：
int32_t STB_VOD_Info_Aud_PacketNum ( uint32_t* num);
参数：
Num:返回音频RTP的数量
返回：
错误代码

*********************************************************/
int32_t STB_VOD_Info_Aud_PacketNum ( uint32_t* num);

/**********************************************************
说明：
返回当前影片使用的range类型是npt、clock、pts
注：npt为相对时间    clock为绝对时间  pts为pts形式的时间
定义：
ErrorCode_t STB_VOD_Info_RangeType ( VOD_RANGETYPE_t* rangetype);
参数：
Timemode:返回影片使用的是相对时间还是绝对时间
返回：
错误代码
*********************************************************/
//int32_t STB_VOD_Info_RangeType ( VOD_RANGE_TYPE_t * rtype );

/**********************************************************
说明：
返回当前影片使用的range类型是npt、clock、pts
定义：
int32_t STB_VOD_Set_Debug_Level ( int32_t debuglevel )
参数：
debuglevel: 调试级别
返回：
错误代码
*********************************************************/
int32_t STB_VOD_Set_Debug_Level ( int32_t debuglevel );

#endif



