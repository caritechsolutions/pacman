#!/bin/bash

echo -n "Input the player project path: "
read -e PLAYER_PATH

echo $PLAYER_PATH

STB_PATH=$GXSRC_PATH
echo $STB_PATH

DEL_FILES="app_pmp_root.c app_pmp_top.c  .gdbinit app.c app_sysset.c  gdbconfig main.c Makefile memwatch.c signal_connect.c include/app.h libnet"


#svn update

#copy image
mkdir -p ${STB_PATH}/theme/image/player
cp ${PLAYER_PATH}/theme2/image/*.bmp ${STB_PATH}/theme/image/player/.
cd ${STB_PATH}/theme/image && sh create_image.sh

#copy app code
mkdir -p ${STB_PATH}/app/player
cp -r ${PLAYER_PATH}/app/* ${STB_PATH}/app/player

#del svn 
find  ${STB_PATH}/app/player -type d -name ".svn" | xargs rm -rf
#del specify file
for i in $DEL_FILES; do
rm -rf ${STB_PATH}/app/player/${i}
done

# back from media
sed -i -e "s/\/\/cant exit media_centre/case APPK_BACK:\n\
			{\
			int32_t init_value;\n\
			GUI_SetInterface(\"logic_clut\", \"IconArrowGreen2.bmp\");\n\
			GxBus_ConfigGetInt(\"osd>trans_level\", \&init_value, 255);\n\
			GUI_SetInterface(\"osd_alpha_global\", \&init_value);\n\
			GUI_EndDialog(\"win_media_centre\");\n\
            if (pmpset_get_int(PMPSET_VOLUME)==PMPSET_MUTE_ON){ init_value=1; GUI_SetInterface(\"av>mute\", \&init_value);}\n\
            else { init_value=0; GUI_SetInterface(\"av>mute\", \&init_value);}\n\
            app_epg_init(g_AppPlayOps.normal_play.ts_src);\n\
           	}\nbreak;/g" $STB_PATH/app/player/app_media_centre.c

# entre media
sed -i -e "s/\#ifdef PMP_ATTACH_DVB/\
            {\
			int32_t init_value;\n\
            app_send_msg_exec(GXMSG_EPG_RELEASE, NULL);\n\
            pmpset_init();\n\
            GxBus_ConfigGetInt(\"av>mute\", \&init_value, 0);\n\
            if (init_value == 0)  {pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);}\n\
            else {pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);}\n\
            }\n\#ifdef PMP_ATTACH_DVB/g" $STB_PATH/app/player/app_media_centre.c

# extern function
sed -i -e "s/\#include \"app.h\"/\
\#include \"app.h\"\n\
\#include \"app_module.h\"\n\
\#include \"app_send_msg.h\"\n\n\
extern void app_epg_init(uint32_t ts_src);/g" $STB_PATH/app/player/app_media_centre.c


