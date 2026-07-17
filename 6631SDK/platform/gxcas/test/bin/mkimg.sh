#!/bin/bash

TEMP_FILE=${GXSRC_PATH}/bin/stb_img

mkdir -p $TEMP_FILE

#copy files
cp -f ${GXSRC_PATH}/app/out.elf ${TEMP_FILE}

cp -rf ${GXSRC_PATH}/bin/flash/file_img/datafs_ecos ${TEMP_FILE}
cp -rf ${GXSRC_PATH}/bin/flash/file_img/rootfs_ecos ${TEMP_FILE}

mkdir -p ${TEMP_FILE}/rootfs_ecos/dvb

## choose the right theme
#HD=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep HD |wc -l)
#if [ $HD -eq 1 ]; then
#cp -rf ${GXSRC_PATH}/theme/HD ${TEMP_FILE}/rootfs_ecos/dvb/theme;
#else
#cp -rf ${GXSRC_PATH}/theme/SD ${TEMP_FILE}/rootfs_ecos/dvb/theme;
#fi;

#GAME_OFF=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "GAME_ENABLE" | grep "0" | wc -l)
#if [ $GAME_OFF -eq 1 ]; then
#rm ${TEMP_FILE}/rootfs_ecos/dvb/theme/image/game -rf;
#fi;

#delete useless widget xml
rm ${TEMP_FILE}/rootfs_ecos/dvb/theme/widget/win_movie_view_speed_*.xml -rf;

mkdir -p ${TEMP_FILE}/ecos

#delete svn file
rm ${TEMP_FILE}/rootfs_ecos/dvb/theme/img_index -rf
find ${TEMP_FILE} -type d -name ".svn" | xargs rm -rf

#delete Thumbs.db
find ${TEMP_FILE} -name "Thumbs.db" | xargs rm -rf

##delete useless language xml
#LANG_DIR=${TEMP_FILE}/rootfs_ecos/dvb/theme/language
#LANG=$(cat ${LANG_DIR}/language | grep '#' | tr -d "#")
#LANG_FILE=`find ${LANG_DIR} -name "*.xml"`
#for i in $LANG_FILE ; do
#	FILE_NAME=$(basename $i .xml)
#	EXIST=0
#	for j in $LANG; do
#	    if [ $FILE_NAME = $j ] || [ $FILE_NAME = "i18n" ]; then
#			EXIST=1
#		fi;
#	done
#	if [ $EXIST -eq 0 ]; then
#		rm $i -f
#	fi
#done

#delete script
find ${TEMP_FILE} -name "*.sh" | xargs rm -rf
find ${TEMP_FILE} -name "*.xls" | xargs rm -rf
find ${TEMP_FILE} -name "*.py" | xargs rm -rf

#create ecos_romfs
if [ $ARCH = "arm" ];then
    arm-eabi-objcopy ${TEMP_FILE}/out.elf -S -g -O binary ${TEMP_FILE}/ecos.bin
else
    csky-elf-objcopy ${TEMP_FILE}/out.elf -S -g -O binary ${TEMP_FILE}/ecos.bin
fi;

gzip ${TEMP_FILE}/ecos.bin
mv -f ${TEMP_FILE}/ecos.bin.gz ${TEMP_FILE}/ecos
genromfs -f ${TEMP_FILE}/ecos_romfs.img -d ${TEMP_FILE}/ecos/

# write fstab
CONF=${GXSRC_PATH}/bin/flash/flash.conf
BOOT=`cat $CONF | grep "#" -v | grep BOOT -n | awk -F ':' '{print $1}'`
DATA=`cat $CONF | grep "#" -v | grep DATA -n | awk -F ':' '{print $1}'`
printf "/dev/flash/0/%d  /home/gx  minifs\n" $((DATA-BOOT)) >${TEMP_FILE}/rootfs_ecos/etc/fstab
printf "NONE  /mnt  ramfs\n" >>${TEMP_FILE}/rootfs_ecos/etc/fstab

#create cramfs, fstab changed need build it again
mkfs.cramfs ${TEMP_FILE}/rootfs_ecos ${TEMP_FILE}/root_cramfs.img

cp -f ${TEMP_FILE}/ecos_romfs.img ${GXSRC_PATH}/bin/flash
cp -f ${TEMP_FILE}/root_cramfs.img ${GXSRC_PATH}/bin/flash

# delete temp file
rm -rf $TEMP_FILE

cd ${GXSRC_PATH}/bin/flash

./genflash mkflash flash.conf download.bin

