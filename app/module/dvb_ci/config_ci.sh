#!/bin/bash

cmd=$1
config_dir=${GXSRC_PATH}/app/module/dvb_ci
app_lib_path=${GXSRC_PATH}/lib/${ARCH}-${OS}
theme_path=${GXSRC_PATH}/output/theme
config_bin_dst_dir=${GXSRC_PATH}/output/image/bin_${OS}
conf_family_name=${BR2_FAMILY_NAME}
conf_proj_name=${BR2_PROJ_NAME}
flash_config_dir=${GXSRC_PATH}/projects/${conf_family_name}/${conf_proj_name}/flash
if [[ ${cmd} = "create" ]]; then
    if [ -e $config_dir/lib ];then
        content=`ls $config_dir/lib/`
        if [ -z "$content" ];then
            echo "no file to copy"
        else
	   cp $config_dir/lib/${ARCH}_${OS}/*.a  $app_lib_path
	   cp $config_dir/lib/${ARCH}_${OS}/*.bin  $flash_config_dir
	   echo "-------Project Dir: " $flash_config_dir
        fi
    fi
    if [[ ${OS} = "ecos" ]]; then
        if [ -e $config_dir/ci_widget ];then
            mkdir -p $theme_path/widget/dvb_ci
            cp $config_dir/ci_widget/*.xml $theme_path/widget/dvb_ci
        fi
    fi 
    if [[ ${OS} = "linux" ]]; then
        if [ -e $config_dir/linux_widget ];then
            mkdir -p $theme_path/widget/dvb_ci
            cp $config_dir/linux_widget/*.xml $theme_path/widget/dvb_ci
        fi
    fi

fi
if [[ ${cmd} = "clean" ]]; then
    if [ -e $theme_path/widget/dvb_ci ];then
        rm -f $theme_path/widget/dvb_ci
    fi
    rm -f $app_lib_path/libcim*.a

fi



