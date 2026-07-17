#!/bin/bash
if [ -e ${GXSRC_PATH}/.config ]; then
		. ${GXSRC_PATH}/.config
fi

CROSS_COMPILE=$1

if [ -z "$CROSS_COMPILE" ]; then
    echo -e "\033[1;31mplease check cross compile setting about CROSS_COMPILE!\033[0m"
    exit 1
fi

BUILDROOT_PATH=$GXSRC_PATH/../buildroot

#### main menu obj
MAIN_MENU_OBJ=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "APP_MAIN_MENU_OBJ_SUPPORT" | grep "1" |wc -l)
if [ $MAIN_MENU_OBJ -eq 1 ]; then
	echo "main menu obj support"
	if [ -e ${GXSRC_PATH}/dll/mainmenu/out ]; then
		if [ -e ${GXSRC_PATH}/dll/mainmenu/out/gxdll_mainmenu.o ]; then
			cp ${GXSRC_PATH}/dll/mainmenu/out/gxdll_mainmenu.o ${GXSRC_PATH}/output/theme/
		else
			echo "main menu obj file is not exist"
		fi
	else
		echo "main menu obj out dir is not exist"
	fi
fi

#############################################################
# make eCOS image bin
#############################################################
copy_files() {
	local current_dir="$1"
	local target_dir="$2"

	if [ -f "$current_dir" ]; then
		# 如果是普通文件，复制到目标目录
		cp "$current_dir" "$target_dir"
	elif [ -L "$current_dir" ]; then
		# 如果是符号链接，检查目标是否存在
		target="$(readlink -f '$current_dir')"
		if [ -e "$target" ]; then
			cp "$target" "$target_dir"
		else
			echo "Skipping broken symlink: $item"
		fi
	fi
}

if [ $OS = "ecos" ];then

	build_image_temp_dir=${GXSRC_PATH}/output/image/bin_ecos
	CHIP_3211=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep GX3211 |wc -l)
    config_family_name=$(echo ${BR2_FAMILY_NAME} | sed 's/\"//g')
    config_proj_name=$(echo ${BR2_PROJ_NAME} | sed 's/\"//g')
    config_proj_path=${GXSRC_PATH}/projects/${config_family_name}/${config_proj_name}
    tool_boot_path=${GXSRC_PATH}/../platform/gxloader
    FLASH_TYPE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "NAND_FLASH_SUPPORT" | grep "1" | wc -l)

	# delete temp file
	#rm -rf $build_image_temp_dir
	#
	mkdir -p $build_image_temp_dir

    #copy flash.conf
    echo "copy flash.conf"
    if [ ! -e ${build_image_temp_dir}/flash.conf ]; then
        echo -e "    -- copy from \033[032m${config_proj_path}/flash/flash.conf\033[0m"
        cp ${config_proj_path}/flash/flash.conf ${build_image_temp_dir}/flash.conf
    fi

    # generate loader
    source ${GXSRC_PATH}/tools/genflash/loader_generate.sh

   	#copy files
	cp ${GXSRC_PATH}/output/out.elf ${build_image_temp_dir}

	cp -rf ${GXSRC_PATH}/system/ecos_template/flash/file_img/datafs_ecos ${build_image_temp_dir}
	cp -rf ${GXSRC_PATH}/system/ecos_template/flash/file_img/rootfs_ecos ${build_image_temp_dir}

	mkdir -p ${build_image_temp_dir}/rootfs_ecos/dvb
	mkdir -p ${build_image_temp_dir}/rootfs_ecos/home/theme_data

	# choose the right theme
	rm -rf ${build_image_temp_dir}/rootfs_ecos/dvb/theme
	HD=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep HD |wc -l)
	if [ $HD -eq 1 ]; then
	#cp -r ${GXSRC_PATH}/output/theme/HD ${build_image_temp_dir}/rootfs_ecos/dvb/theme;
	cp -r ${GXSRC_PATH}/output/theme/ ${build_image_temp_dir}/rootfs_ecos/dvb/theme;
	else
	#cp -r ${GXSRC_PATH}/output/theme/SD ${build_image_temp_dir}/rootfs_ecos/dvb/theme;
	cp -r ${GXSRC_PATH}/output/theme/ ${build_image_temp_dir}/rootfs_ecos/dvb/theme;
	fi;

	if [ -e ${config_proj_path}/firmware/gxlowpower.fw ]; then
        #echo -e "------\033[032m${config_proj_path}/--------------\033[0m"
        mkdir -p ${build_image_temp_dir}/rootfs_ecos/lib/firmware
        cp -rf ${config_proj_path}/firmware/gxlowpower.fw ${build_image_temp_dir}/rootfs_ecos/lib/firmware/gxlowpower.fw
	fi

	CHIP_SCORPIO_TAURUS=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep SCORPIO_TAURUS |wc -l)
	MINIMPEG_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "AUDIO_DESCRIPTOR_SUPPORT" | grep "1" | wc -l)
	SPEEDPLAY_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "PLAYBACK_SPEED_SUPPORT" | grep "1" | wc -l)
	FLAC_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "FLAC_SUPPORT" | grep "1" | wc -l)
	OGG_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "OGG_SUPPORT" | grep "1" | wc -l)
	if [ $CHIP_SCORPIO_TAURUS -eq 1 ]; then
		if [ -d ${GXLIB_PATH}/lib/avfw/taurus ]; then
			mkdir -p ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio
			mkdir -p ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video
			copy_files ${GXLIB_PATH}/lib/avfw/taurus/video/hevc.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video/hevc.bin
			copy_files ${GXLIB_PATH}/lib/avfw/taurus/video/avs.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video/avs.bin
			copy_files ${GXLIB_PATH}/lib/avfw/taurus/video/avc.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video/avc.bin
			copy_files ${GXLIB_PATH}/lib/avfw/taurus/video/mpeg2.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video/mpeg2.bin
			copy_files ${GXLIB_PATH}/lib/avfw/taurus/video/mpeg4.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/video/mpeg4.bin
			if [ $MINIMPEG_ENABLE -eq 1 ] || [ $SPEEDPLAY_ENABLE -eq 1 ]; then
				copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/mpeg.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/mpeg.bin
			else
				copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/mpeg.mini.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/mpeg.mini.bin
			fi
			if [ $FLAC_ENABLE -eq 1 ] && [ $OGG_ENABLE -eq 1 ]; then
				copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/fov.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/fov.bin
			else
				if [ $FLAC_ENABLE -eq 1 ]; then
					copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/flac.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/flac.bin
				fi
				if [ $OGG_ENABLE -eq 1 ]; then
					copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/opus.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/opus.bin
					copy_files ${GXLIB_PATH}/lib/avfw/taurus/audio/vorbis.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/taurus/audio/vorbis.bin
				fi
 			fi
		else
			echo "    -- cannot find ${GXLIB_PATH}/lib/avfw/taurus"
		fi
		if [ -d ${GXLIB_PATH}/lib/avfw/scorpio ]; then
			mkdir -p ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio
			if [ $MINIMPEG_ENABLE -eq 1 ]; then
				copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/mpeg.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/mpeg.bin
			else
				copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/mpeg.mini.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/mpeg.mini.bin
			fi
			if [ $FLAC_ENABLE -eq 1 ] && [ $OGG_ENABLE -eq 1 ]; then
				copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/fov.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/fov.bin
			else
				if [ $FLAC_ENABLE -eq 1 ]; then
					copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/flac.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/flac.bin
				fi
				if [ $OGG_ENABLE -eq 1 ]; then
					copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/opus.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/opus.bin
					copy_files ${GXLIB_PATH}/lib/avfw/scorpio/audio/vorbis.bin ${build_image_temp_dir}/rootfs_ecos/lib/firmware/avfw/scorpio/audio/vorbis.bin
				fi
			fi
		else
			echo "    -- cannot find ${GXLIB_PATH}/lib/avfw/scorpio"
		fi
	fi;

	DDA_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "DOLBY_SUPPORT" | grep "1" | wc -l)
	if [ $DDA_ENABLE -eq 1 ]; then
            if [ -e ${config_proj_path}/firmware/dolby.fw ]; then
                cp -rf ${config_proj_path}/firmware/dolby.fw ${build_image_temp_dir}/rootfs_ecos/usr/sbin
            else
                echo "    -- cannot find ${config_proj_path}/firmware/dolby.fw"
	    fi
	fi;

	APPLETS_BUILTIN_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "APPLETS_BUILTIN_SUPPORT" | grep "1" | wc -l)
	if [ $APPLETS_BUILTIN_ENABLE -eq 1 ]; then
		rm ${build_image_temp_dir}/rootfs_ecos/dvb/applets -rf
		if [ -e ${config_proj_path}/applets/applet_list.json ];then
			cp -rf ${config_proj_path}/applets ${build_image_temp_dir}/rootfs_ecos/dvb/
		fi
	else
	    rm ${build_image_temp_dir}/rootfs_ecos/dvb/applets -rf
	fi;

	GAME_OFF=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "GAME_ENABLE" | grep "0" | wc -l)
	if [ $GAME_OFF -eq 1 ]; then
	rm ${build_image_temp_dir}/rootfs_ecos/dvb/theme/image/game -rf;
	fi;
	#delete useless widget xml
	rm ${build_image_temp_dir}/rootfs_ecos/dvb/theme/widget/win_movie_view_speed_*.xml -rf;

	mkdir -p ${build_image_temp_dir}/ecos

    #delete git file
    find ${build_image_temp_dir} -type f -name ".gitkeep" | xargs rm -rf

	#delete svn file
	rm ${build_image_temp_dir}/rootfs_ecos/dvb/theme/img_index -rf
	find ${build_image_temp_dir} -type d -name ".svn" | xargs rm -rf

	#delete Thumbs.db
	find ${build_image_temp_dir} -name "Thumbs.db" | xargs rm -rf

	#delete useless language xml
	LANG_DIR=${build_image_temp_dir}/rootfs_ecos/dvb/theme/language
	LANG=$(cat ${LANG_DIR}/language | grep '#' | tr -d "#")
	LANG_FILE=`find ${LANG_DIR} -name "*.xml"`
	for i in $LANG_FILE ; do
		FILE_NAME=$(basename $i .xml)
		EXIST=0
		for j in $LANG; do
		    if [ $FILE_NAME = $j ] || [ $FILE_NAME = "i18n" ]; then
				EXIST=1
			fi;
		done
		if [ $EXIST -eq 0 ]; then
			rm $i -f
		fi
	done
	rm ${LANG_DIR}/language -rf

	#delete script
	find ${build_image_temp_dir} -name "*.sh" | xargs rm -rf
	find ${build_image_temp_dir} -name "*.xls" | xargs rm -rf
	find ${build_image_temp_dir} -name "*.py" | xargs rm -rf

	#create ecos_romfs
	if [ $ARCH = "arm" ];then
	arm-none-eabi-objcopy ${build_image_temp_dir}/out.elf -S -g -O binary ${build_image_temp_dir}/ecos.bin
	else
	csky-elf-objcopy ${build_image_temp_dir}/out.elf -S -g -O binary ${build_image_temp_dir}/ecos.bin
	fi;

	#gzip ${build_image_temp_dir}/ecos.bin
	${GXSRC_PATH}/tools/genflash/lzma -z -k --lzma1=mf=bt2,lp=1,lc=3 ${build_image_temp_dir}/ecos.bin
	#mv -f ${build_image_temp_dir}/ecos.bin.gz ${build_image_temp_dir}/ecos
	mv -f ${build_image_temp_dir}/ecos.bin.lzma ${build_image_temp_dir}/ecos/ecos.bin.lzma
	genromfs -f ${build_image_temp_dir}/ecos_romfs.img -d ${build_image_temp_dir}/ecos/

	# write fstab
	#CONF=${GXSRC_PATH}/system/ecos_template/flash/flash.conf
	CONF=${GXSRC_PATH}/output/image/bin_ecos/flash.conf
	BOOT=`cat $CONF | grep "#" -v | grep BOOT -n | awk -F ':' '{print $1}'`
	DATA=`cat $CONF | grep "#" -v | grep DATA -n | awk -F ':' '{print $1}'`

	if [[ $BR2_MOD_NAND_YAFS = y ]];then
		printf "/dev/flash/0/%d  /home/gx  yaffs2\n" $((DATA-BOOT)) >${build_image_temp_dir}/rootfs_ecos/etc/fstab
	else
		printf "/dev/flash/0/%d  /home/gx  minifs\n" $((DATA-BOOT)) >${build_image_temp_dir}/rootfs_ecos/etc/fstab
	fi

	CAS_TYPE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "CASCAM_DVTCAS_SUPPORT" | wc -l)
	if [ $CAS_TYPE -eq 1 ]; then
		CAS=`cat $CONF | grep "#" -v | grep CAS -w -n | awk -F ':' '{print $1}'`
		if [ ! $CAS ]; then
			echo -e "\033[031merror!!!!flashmap do not have CAS partition\033[0m"
		else
			printf "/dev/flash/0/%d  /home/cas  minifs\n" $((CAS-BOOT)) >>${build_image_temp_dir}/rootfs_ecos/etc/fstab
			mkdir -p ${build_image_temp_dir}/rootfs_ecos/home/cas
		fi;
		CASBACK=`cat $CONF | grep "#" -v | grep CASBACK -n | awk -F ':' '{print $1}'`
		if [ ! $CASBACK ]; then
			echo -e "\033[031merror!!!!flashmap do not have CASBACK partition\033[0m"
		else
			printf "/dev/flash/0/%d  /home/casback  minifs\n" $((CASBACK-BOOT)) >>${build_image_temp_dir}/rootfs_ecos/etc/fstab
			mkdir -p ${build_image_temp_dir}/rootfs_ecos/home/casback
		fi;
	fi;
    printf "NONE  /mnt  ramfs\n" >>${build_image_temp_dir}/rootfs_ecos/etc/fstab

    FS_TYPE=$(cat ${GXSRC_PATH}/app/bsp.cpp| grep "SQUASHFS_SUPPER" | grep "1" | wc -l)

    SKIN_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "SKIN_SUPPORT" | grep "1" | wc -l)
    if [ $SKIN_ENABLE -eq 1 ]; then
        if [ $FS_TYPE -eq 1 ]; then
            ${GXSRC_PATH}/tools/genflash/mksquashfs ${build_image_temp_dir}/rootfs_ecos/dvb/  ${build_image_temp_dir}/theme0_squashfs.img -b 32K -noappend -no-progress -all-root > /dev/null
        else
            ${GXSRC_PATH}/tools/genflash/mkfs.cramfs ${build_image_temp_dir}/rootfs_ecos/dvb/ ${build_image_temp_dir}/theme0_cramfs.img
        fi;

        #### dvb_skin for create new skin base theme
        mkdir -p ${build_image_temp_dir}/dvb_skin
        cp ${build_image_temp_dir}/rootfs_ecos/dvb/theme ${build_image_temp_dir}/dvb_skin -rf

        mkdir -p ${build_image_temp_dir}/rootfs_ecos/dvb_temp
        rm -rf ${build_image_temp_dir}/rootfs_ecos/dvb/theme

        cp -r ${GXSRC_PATH}/app/skin/recover/theme/ ${build_image_temp_dir}/rootfs_ecos/dvb/theme;
    fi;

    if [ $FS_TYPE -eq 1 ]; then
        ${GXSRC_PATH}/tools/genflash/mksquashfs ${build_image_temp_dir}/rootfs_ecos ${build_image_temp_dir}/root_fs.img -b 32K -noappend -no-progress -all-root > /dev/null
    else
        #create cramfs, fstab changed need build it again
        ${GXSRC_PATH}/tools/genflash/mkfs.cramfs ${build_image_temp_dir}/rootfs_ecos ${build_image_temp_dir}/root_cramfs.img
    fi;

	cp ${GXSRC_PATH}/system/ecos_template/flash/minifs.img ${build_image_temp_dir}

	cp ${GXSRC_PATH}/tools/genflash/genflash ${build_image_temp_dir}

	cd ${build_image_temp_dir}
	chmod +x ./genflash
	./genflash mkflash flash.conf download.bin unchanged_fs
    if [ $FLASH_TYPE -eq 1 ]; then
        boot_file=$BR2_CHIP_NAME-$BR2_BOARD_NAME-spinand.boot
    else
        boot_file=$BR2_CHIP_NAME-$BR2_BOARD_NAME-sflash.boot
    fi
    if [ -e  ${GXSRC_PATH}/output/image/boot ]; then
        unlink ${GXSRC_PATH}/output/image/boot
    fi
    if [ -e  ${GXSRC_PATH}/output/image/$boot_file ]; then
        unlink ${GXSRC_PATH}/output/image/$boot_file
    fi
    FILE_COUNT=$(find "${GXSRC_PATH}/tools/download" -maxdepth 1 -type f -name "*${BR2_CHIP_NAME}-${BR2_BOARD_NAME}*" | wc -l)
    BOOT_TOOL_FILE_PATH=`find ${GXSRC_PATH}/tools/download -name "${BR2_CHIP_NAME}-${BR2_BOARD_NAME}*"`
    BOOT_TOOL_FILE_NAME=$(basename "$BOOT_TOOL_FILE_PATH")
    if [ -e download.bin ]; then
        echo -e "\033[032m[FlashImg]: Make flash image success.\033[0m"
        mkdir -p ../../../output/image
        mv download.bin ../../../output/image/download_ecos.bin
        if [ -e  ${GXSRC_PATH}/tools/download/boot ]; then
            ln -s ${GXSRC_PATH}/tools/download/boot ${GXSRC_PATH}/output/image/boot
        fi
        if [ -e  ${GXSRC_PATH}/tools/download/$boot_file ]; then
            ln -s ${GXSRC_PATH}/tools/download/$boot_file ${GXSRC_PATH}/output/image/$boot_file
        fi
        if [ "$BR2_CHIP_NAME" != "scorpio-taurus" ]; then
            if [ ${FILE_COUNT} -eq 1 ]; then
                if [ -e ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME} ]; then
                    unlink ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME}
                fi
                ln -s ${BOOT_TOOL_FILE_PATH} ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME}
            else
                cd ${tool_boot_path}
                ./build ${BR2_CHIP_NAME} ${BR2_BOARD_NAME} boot
                cd -
                cp ${tool_boot_path}/output/*.boot ${GXSRC_PATH}/output/image/${BR2_CHIP_NAME}-${BR2_BOARD_NAME}-flash.boot
            fi
        fi
    else
        echo -e "\033[031m[FlashImg]: Make flash image failed.\033[0m"
        exit 1
    fi
fi
#############################################################
# make Linux image bin
#############################################################
if [ $OS = "linux" ];then
	# 1. look up solution/output/image/rootfs directory, then generate rootfs.bin/cfg.bin
	# 2. look up buildroot/output directory, then copy loader-sflash.bin/uImage/rootfs.tar.gz
	# 3. look up solution/output/image directory, then copy flash.conf/loader-sflash.bin/uImage/rootfs.tar.gz
	# 4. look up project/flash directory, then copy flash.conf/loader-sflash.bin/uImage
	# 5. look up project/flash directory, generate from rootfs.tar.gz or just copy rootfs.bin/cfg.bin
	buildroot_output_path=${BUILDROOT_PATH}/output
	flashimg_build_path=${GXSRC_PATH}/output/image/bin_linux
	tool_boot_path=${GXSRC_PATH}/../platform/gxloader
	FLASH_TYPE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "NAND_FLASH_SUPPORT" | grep "1" | wc -l)
	if [ -e ${GXSRC_PATH} ]; then
        if [ ! -d ${flashimg_build_path} ]; then
            mkdir -p ${flashimg_build_path}
        fi
	else
		echo "[FlashImg]: cannot find ${GXSRC_PATH}"
		exit 1
	fi

	solution_build_path=$(pwd)
	config_family_name=$(echo ${BR2_FAMILY_NAME} | sed 's/\"//g')
	config_proj_name=$(echo ${BR2_PROJ_NAME} | sed 's/\"//g')
	config_proj_path=${solution_build_path}/projects/${config_family_name}/${config_proj_name}

	echo "Make Linux flash image"
	echo "Family:"${config_family_name}
	echo "ProjName:"${config_proj_name}

	# copy prject private configure files
	if [ -e $config_proj_path/flash/logo.bin ];then
		cp $config_proj_path/flash/logo.bin $flashimg_build_path
	fi

	# link needed command
	if [ ! -e ${flashimg_build_path}/mkfs.jffs2 ];then
		ln -s ${GXSRC_PATH}/tools/genflash/mkfs.jffs2 ${flashimg_build_path}/mkfs.jffs2
	fi
	if [ ! -e ${flashimg_build_path}/mksquashfs ];then
		ln -s ${GXSRC_PATH}/tools/genflash/mksquashfs ${flashimg_build_path}/mksquashfs
	fi
	if [ ! -e ${flashimg_build_path}/genflash ];then
		ln -s ${GXSRC_PATH}/tools/genflash/genflash ${flashimg_build_path}/genflash
	fi
	if [ ! -e ${flashimg_build_path}/mkfs.ubifs ];then
		ln -s ${GXSRC_PATH}/tools/genflash/mkfs.ubifs ${flashimg_build_path}/mkfs.ubifs
	fi
	if [ ! -e ${flashimg_build_path}/ubinize ];then
		ln -s ${GXSRC_PATH}/tools/genflash/ubinize ${flashimg_build_path}/ubinize
	fi

	# prepare for flash.conf
	if [ ! -e ${flashimg_build_path}/flash.conf ]; then
        echo "[FlashImg]: copy flash.conf"
        if [ -e ${GXSRC_PATH}/output/image/flash.conf ];then
            echo -e "    -- copy from \033[032m${GXSRC_PATH}/output/image/flash.conf\033[0m"
            cp ${GXSRC_PATH}/output/image/flash.conf ${flashimg_build_path}/flash.conf
        elif [ -e ${config_proj_path}/flash/flash.conf ]; then
            echo -e "    -- copy from \033[032m${config_proj_path}/flash/flash.conf\033[0m"
            cp ${config_proj_path}/flash/flash.conf   ${flashimg_build_path}/flash.conf
        else
            echo "    -- cannot find flash.conf"
            exit 1
        fi
    fi

	# prepare for loader
#	echo "[FlashImg]: copy loader"
#	rm -f ${flashimg_build_path}/loader-sflash.bin
#	if [ -e ${GXSRC_PATH}/output/image/loader-sflash.bin ]; then
#		echo -e "    -- copy from \033[032m${GXSRC_PATH}/output/image/loader-sflash.bin\033[0m"
#		cp ${GXSRC_PATH}/output/image/loader-sflash.bin ${flashimg_build_path}/loader-sflash.bin
#	elif [ -e ${config_proj_path}/flash/loader-sflash.bin ]; then
#		echo -e "    -- copy from \033[032m${config_proj_path}/flash/loader-sflash.bin\033[0m"
#		cp ${config_proj_path}/flash/loader-sflash.bin  ${flashimg_build_path}/loader-sflash.bin
#	else
#		echo "    -- cannot find loader-sflash.bin"
#		exit 1
#	fi

    # generate loader
    source ${GXSRC_PATH}/tools/genflash/loader_generate.sh

	# prepare for uImage
	echo "[FlashImg]: copy uImage"
	rm -f ${flashimg_build_path}/uImage
	if [ -e ${GXSRC_PATH}/output/image/uImage ]; then
		echo -e "    -- copy from \033[032m${GXSRC_PATH}/output/image/uImage\033[0m"
		cp ${GXSRC_PATH}/output/image/uImage ${flashimg_build_path}/uImage
	elif [ -e ${buildroot_output_path}/uImage ]; then
		dtb=$(cat ${flashimg_build_path}/flash.conf | grep "^dtb_file" | awk  -F ' ' '{print $2}')
		echo -e "    -- dtb = \033[33m$dtb\033[0m"
		echo -e "    -- copy from \033[032m${buildroot_output_path}/uImage\033[0m"
		cp ${buildroot_output_path}/uImage ${flashimg_build_path}/uImage
		echo -e "    -- copy from \033[032m${buildroot_output_path}/$dtb\033[0m"
		cp ${buildroot_output_path}/$dtb ${flashimg_build_path}/
	elif [ -e ${config_proj_path}/flash/uImage ]; then
		echo -e "    -- copy from \033[032m${config_proj_path}/flash/uImage\033[0m"
		cp ${config_proj_path}/flash/uImage  ${flashimg_build_path}/uImage
	else
		echo "    -- cannot find uImage"
		exit 1
	fi

	SKIN_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "SKIN_SUPPORT" | grep "1" | wc -l)

	# prepare for rootfs.bin
	echo "[FlashImg]: make rootfs.bin"
    rm -rf ${flashimg_build_path}/root
	if [ -e ${GXSRC_PATH}/output/image/rootfs ]; then
		echo -e "    -- generate from \033[032m${GXSRC_PATH}/output/image/rootfs\033[0m"
		fakeroot cp -R ${GXSRC_PATH}/output/image/rootfs ${flashimg_build_path}/root/ -ar
		rm -rf ${flashimg_build_path}/root/dvb/*
		rm -rf ${flashimg_build_path}/root/home/gx/*
		if [ $SKIN_ENABLE -eq 1 ]; then
			mkdir -p ${flashimg_build_path}/root/dvb_temp
		fi;

        if [ -e ${config_proj_path}/default_data.db ];then
            cp ${config_proj_path}/default_data.db ${flashimg_build_path}/root/home/root -f
        fi

        DDA_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "DOLBY_SUPPORT" | grep "1" | wc -l)
        if [ $DDA_ENABLE -eq 1 ]; then
            if [ -e ${config_proj_path}/firmware/dolby.fw ]; then
                cp -rf ${config_proj_path}/firmware/dolby.fw ${flashimg_build_path}/root/usr/sbin
            else
                echo "    -- cannot find ${config_proj_path}/firmware/dolby.fw"
            fi
        fi;

        find ${flashimg_build_path}/root/ -type d -name ".svn" | xargs rm -rf
		cd ${flashimg_build_path}
                ./mksquashfs ./root ./rootfs.bin -noappend -no-duplicates > /dev/null
	elif [ -e ${GXSRC_PATH}/output/image/rootfs.tar.gz ]; then
		echo -e "    -- generate from \033[032m${GXSRC_PATH}/output/image/rootfs.tar.gz\033[0m"
		mkdir -p ${flashimg_build_path}/root
		tar xf ${GXSRC_PATH}/output/image/rootfs.tar.gz -C ${flashimg_build_path}/root

		rm -rf ${flashimg_build_path}/root/dvb/*
		rm -rf ${flashimg_build_path}/root/home/gx/*
		if [ $SKIN_ENABLE -eq 1 ]; then
			mkdir -p ${flashimg_build_path}/root/dvb_temp
		fi;

        if [ -e ${config_proj_path}/default_data.db ];then
            cp ${config_proj_path}/default_data.db ${flashimg_build_path}/root/home/root -f
        fi

        DDA_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "DOLBY_SUPPORT" | grep "1" | wc -l)
        if [ $DDA_ENABLE -eq 1 ]; then
            if [ -e ${config_proj_path}/firmware/dolby.fw ]; then
                cp -rf ${config_proj_path}/firmware/dolby.fw ${flashimg_build_path}/root/usr/sbin
            else
                echo "    -- cannot find ${config_proj_path}/firmware/dolby.fw"
            fi
        fi;

		find ${flashimg_build_path}/root/ -type d -name ".svn" | xargs rm -rf
		cd ${flashimg_build_path}
        ./mksquashfs ./root ./rootfs.bin -noappend -no-duplicates > /dev/null
	elif [ -e ${buildroot_output_path}/rootfs.tar.gz ]; then
		echo -e "    -- generate from \033[032m${buildroot_output_path}/rootfs.tar.gz\033[0m"
		mkdir -p ${flashimg_build_path}/root
		tar xf ${buildroot_output_path}/rootfs.tar.gz -C ${flashimg_build_path}/root

        #find ${flashimg_build_path}/root -name "gdbserver" | xargs rm 2> /dev/null
        #rm ${flashimg_build_path}/root/lib/ld-* 2> /dev/null
        #rm ${flashimg_build_path}/root/lib/lib* 2> /dev/null

		rm -rf ${flashimg_build_path}/root/dvb/*
		rm -rf ${flashimg_build_path}/root/home/gx/*
		if [ $SKIN_ENABLE -eq 1 ]; then
			mkdir -p ${flashimg_build_path}/root/dvb_temp
		fi;

        if [ -e ${config_proj_path}/default_data.db ];then
            cp ${config_proj_path}/default_data.db ${flashimg_build_path}/root/home/root -f
        fi

        DDA_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "DOLBY_SUPPORT" | grep "1" | wc -l)
        if [ $DDA_ENABLE -eq 1 ]; then
            if [ -e ${config_proj_path}/firmware/dolby.fw ]; then
                cp -rf ${config_proj_path}/firmware/dolby.fw ${flashimg_build_path}/root/usr/sbin
            else
                echo "    -- cannot find ${config_proj_path}/firmware/dolby.fw"
            fi
        fi;

		find ${flashimg_build_path}/root/ -type d -name ".svn" | xargs rm -rf
		cd ${flashimg_build_path}
		if [[ $BR2_MOD_NAND_UBIFS = y ]];then
		    ./mkfs.ubifs -r ./root -m 2048 -e 126976 -c 1024 -o ./rootfs.img
		    ./ubinize -o ./rootfs.bin -m 2048 -p 128KiB -s 2048 -O 2048 ${GXSRC_PATH}/tools/ubifs/ubi.cfg
		else
            ./mksquashfs ./root ./rootfs.bin -noappend -no-duplicates > /dev/null
		fi
	elif [ -e ${config_proj_path}/flash/rootfs.tar.gz ]; then
		echo -e "    -- generate from \033[032m${config_proj_path}/flash/rootfs.tar.gz\033[0m"
		mkdir -p ${flashimg_build_path}/root
		tar xf ${config_proj_path}/flash/rootfs.tar.gz -C ${flashimg_build_path}/root

		rm -rf ${flashimg_build_path}/root/dvb/*
		rm -rf ${flashimg_build_path}/root/home/gx/*
		if [ $SKIN_ENABLE -eq 1 ]; then
			mkdir -p ${flashimg_build_path}/root/dvb_temp
		fi;

        if [ -e ${config_proj_path}/default_data.db ];then
            cp ${config_proj_path}/default_data.db ${flashimg_build_path}/root/home/root -f
        fi

        DDA_ENABLE=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "DOLBY_SUPPORT" | grep "1" | wc -l)
        if [ $DDA_ENABLE -eq 1 ]; then
            if [ -e ${config_proj_path}/firmware/dolby.fw ]; then
                cp -rf ${config_proj_path}/firmware/dolby.fw ${flashimg_build_path}/root/usr/sbin
            else
                echo "    -- cannot find ${config_proj_path}/firmware/dolby.fw"
            fi
        fi;

		find ${flashimg_build_path}/root/ -type d -name ".svn" | xargs rm -rf
		cd ${flashimg_build_path}
                ./mksquashfs ./root ./rootfs.bin -noappend -no-duplicates > /dev/null
	elif [ -e ${config_proj_path}/flash/rootfs.bin ]; then
		echo -e "    -- copy from \033[032m${config_proj_path}/flash/rootfs.bin\033[0m"
		cp ${config_proj_path}/flash/rootfs.bin  ${flashimg_build_path}/rootfs.bin
	else
		echo "    -- cannot find rootfs"
		exit 1
	fi


	# prepare for app.bin
	echo "[FlashImg]: make app.bin"
	rm -rf ${flashimg_build_path}/user
	mkdir -p ${flashimg_build_path}/user
	if [ -e ${GXSRC_PATH}/output/image/rootfs/dvb/out.elf ]; then
		echo -e "    -- generate from \033[032m${GXSRC_PATH}/output/image/rootfs\033[0m"
		cp -ar ${GXSRC_PATH}/output/image/rootfs/dvb/* ${flashimg_build_path}/user
	elif [ -e ${GXSRC_PATH}/output/out.elf ]; then
		echo -e "    -- generate from \033[032m${GXSRC_PATH}/output/out.elf\033[0m"
		cp ${GXSRC_PATH}/output/out.elf ${flashimg_build_path}/user/out.elf -f
		cp  -ar ${GXSRC_PATH}/output/theme ${flashimg_build_path}/user/theme
	else
		echo "    -- cannot find out.elf"
		exit 1
	fi
	# copy project keymap
	if [ -e $config_proj_path/keymap.xml ];then
		mkdir -p $flashimg_build_path/user/theme
		cp $config_proj_path/keymap.xml $flashimg_build_path/user/theme
	fi

	#delete Thumbs.db
	find ${flashimg_build_path} -name "Thumbs.db" | xargs rm -rf

	#delete useless language xml
	LANG_DIR=${flashimg_build_path}/user/theme/language
	LANG=$(cat ${LANG_DIR}/language | grep '#' | tr -d "#")
	LANG_FILE=`find ${LANG_DIR} -name "*.xml"`
	for i in $LANG_FILE ; do
		FILE_NAME=$(basename $i .xml)
		EXIST=0
		for j in $LANG; do
		    if [ $FILE_NAME = $j ] || [ $FILE_NAME = "i18n" ]; then
				EXIST=1
			fi;
		done
		if [ $EXIST -eq 0 ]; then
			rm $i -f
		fi
	done
	rm ${LANG_DIR}/language -rf

	#delete script
	find ${flashimg_build_path}/user -name "*.sh" | xargs rm -rf
	find ${flashimg_build_path}/user -name "*.xls" | xargs rm -rf
	find ${flashimg_build_path}/user -name "*.py" | xargs rm -rf


	find ${flashimg_build_path}/user -type d -name ".svn"|xargs rm -rf
	if [ -e ${flashimg_build_path}/user/out.elf ];then
        ${CROSS_COMPILE}strip ${flashimg_build_path}/user/out.elf
		chmod +x ${flashimg_build_path}/user/out.elf
	fi
	cd ${flashimg_build_path}
	if [[ $BR2_MOD_NAND_UBIFS = y ]];then
	    ./mkfs.ubifs -r user -m 2048 -e 126976 -c 1024 -o user.img
	    ./ubinize -o app.bin -m 2048 -p 128KiB -s 2048 -O 2048 ${GXSRC_PATH}/tools/ubifs/ubi_user.cfg
	else
        if [ $SKIN_ENABLE -eq 1 ]; then

            mkdir ${flashimg_build_path}/temp
            mv user/out.elf temp/out.elf
            #./mksquashfs user/theme  theme0_squashfs.img -b 32K -noappend -no-progress -all-root > /dev/null
            ./mksquashfs user  theme0_squashfs.img -noappend -no-duplicates > /dev/null

            mv temp/out.elf user/out.elf
            rm -rf temp
            rm -rf user/theme
            cp -r ${GXSRC_PATH}/app/skin/recover/theme/ user/theme;
        fi;
        ./mksquashfs user app.bin -noappend -no-duplicates > /dev/null
	fi
	# generate image
	echo "[FlashImg]: make flash image"
	cd ${flashimg_build_path}
	./genflash mkflash ./flash.conf flashrom.bin
	mkdir -p ${GXSRC_PATH}/output/image

    if [ $FLASH_TYPE -eq 1 ]; then
        boot_file=$BR2_CHIP_NAME-$BR2_BOARD_NAME-spinand.boot
    else
        boot_file=$BR2_CHIP_NAME-$BR2_BOARD_NAME-sflash.boot
    fi
    if [ -e  ${GXSRC_PATH}/output/image/boot ]; then
        unlink ${GXSRC_PATH}/output/image/boot
    fi
    if [ -e  ${GXSRC_PATH}/output/image/$boot_file ]; then
        unlink ${GXSRC_PATH}/output/image/$boot_file
    fi

    FILE_COUNT=$(find "${GXSRC_PATH}/tools/download" -maxdepth 1 -type f -name "*${BR2_CHIP_NAME}-${BR2_BOARD_NAME}*" | wc -l)
    BOOT_TOOL_FILE_PATH=`find ${GXSRC_PATH}/tools/download -name "${BR2_CHIP_NAME}-${BR2_BOARD_NAME}*"`
    BOOT_TOOL_FILE_NAME=$(basename "$BOOT_TOOL_FILE_PATH")
    if [ -e flashrom.bin ]; then
        echo -e "\033[032m[FlashImg]: Make flash image success.\033[0m"
        mv flashrom.bin ${GXSRC_PATH}/output/image/download_linux.bin
        if [ -e  ${GXSRC_PATH}/tools/download/boot ]; then
            ln -s ${GXSRC_PATH}/tools/download/boot ${GXSRC_PATH}/output/image/boot
        fi
        if [ -e  ${GXSRC_PATH}/tools/download/$boot_file ]; then
            ln -s ${GXSRC_PATH}/tools/download/$boot_file ${GXSRC_PATH}/output/image/$boot_file
        fi
        if [ "$BR2_CHIP_NAME" != "scorpio-taurus" ]; then
            if [ ${FILE_COUNT} -eq 1 ]; then
                if [ -e ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME} ]; then
                    unlink ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME}
                fi
                ln -s ${BOOT_TOOL_FILE_PATH} ${GXSRC_PATH}/output/image/${BOOT_TOOL_FILE_NAME}
            else
                cd ${tool_boot_path}
                ./build ${BR2_CHIP_NAME} ${BR2_BOARD_NAME} boot
                cd -
                cp ${tool_boot_path}/output/*.boot ${GXSRC_PATH}/output/image/${BR2_CHIP_NAME}-${BR2_BOARD_NAME}-flash.boot
            fi
        fi
    else
        echo -e "\033[031m[FlashImg]: Make flash image failed.\033[0m"
        exit 1
    fi
fi
