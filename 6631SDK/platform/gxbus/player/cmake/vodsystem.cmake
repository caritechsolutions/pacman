IF (CONFIG_VOD_SYSTEM)
	SET(SRC_VODSYSTEM ${SRC_VODSYSTEM}
		vodsystem/vod_api/gxplayer_vod.c
		vodsystem/vod_source/vod_trans/ts/vod_trans_ts_parse.c
		vodsystem/vod_source/vod_trans/ts/vod_trans_ts.c
		vodsystem/vod_source/vod_trans/pes/vod_trans_pes_parse.c
		vodsystem/vod_source/vod_trans/pes/vod_trans_pes.c
		vodsystem/vod_source/vod_trans/hwts/vod_trans_hwts.c
		vodsystem/vod_source/vod_trans/vod_trans.c
		vodsystem/vod_source/vod_recv/coship/udp_receiver.c
		vodsystem/vod_source/vod_recv/coship/tcp_receiver.c
		vodsystem/vod_source/vod_recv/coship/receiver.c
		vodsystem/vod_source/vod_recv/wfd/wfd_udp_receiver.c
		vodsystem/vod_source/vod_recv/wfd/wfd_tcp_receiver.c
		vodsystem/vod_source/vod_recv/wfd/wfd_receiver.c
		vodsystem/vod_source/vod_recv/vod_recv_api.c
		vodsystem/vod_source/vod_protocol/coship/rtsp_response.c
		vodsystem/vod_source/vod_protocol/coship/rtsp_command.c
		vodsystem/vod_source/vod_protocol/coship/rtsp_client.c
		vodsystem/vod_source/vod_protocol/wfd/wfd_response.c
		vodsystem/vod_source/vod_protocol/wfd/wfd_command.c
		vodsystem/vod_source/vod_protocol/wfd/wfd_client.c
		vodsystem/vod_source/vod_manager/vod_machines/vod_machine_coship_living.c
		vodsystem/vod_source/vod_manager/vod_machines/vod_machine_coship.c
		vodsystem/vod_source/vod_manager/vod_machines/vod_machine_wfd.c
		vodsystem/vod_source/vod_manager/vod_manager_util.c
		vodsystem/vod_source/vod_manager/vod_manager_main.c
		vodsystem/vod_source/vod_decode/vod_decode_api.c
		vodsystem/vod_source/vod_control_api/vod_control_api.c
		vodsystem/vod_source/vod_comm_func/vod_common_msg.c
		vodsystem/vod_source/vod_comm_func/vod_common_func.c
		vodsystem/vod_source/vod_comm_func/vod_common_xxxx.c
		vodsystem/vod_porting/vod_porting_sys.c
		vodsystem/vod_porting/vod_porting_socket.c
		vodsystem/vod_porting/vod_porting_notify.c
		vodsystem/vod_porting/vod_porting_dvb_player.c
		vodsystem/vod_porting/vod_porting_decode_3201.c
		)
ENDIF (CONFIG_VOD_SYSTEM)

