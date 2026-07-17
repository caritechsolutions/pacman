#!/bin/bash
#####################################################################################################
## CASCAM_SUPPORT

#####################################################################################################
if [[ $BR2_MOD_CASCAM = y ]];then
	echo "#define CASCAM_SUPPORT 1">>$config_file_h

	if [[ $BR2_MOD_CASCAM_CDCAS = y ]];then
		echo "#define CASCAM_CDCAS_SUPPORT">>$config_file_h
		cas_name=cdcas
	fi
	if [[ $BR2_MOD_CASCAM_ABVCAS = y ]];then
		echo "#define CASCAM_ABVCAS_SUPPORT">>$config_file_h
		cas_name=abvcas
	fi
	if [[ $BR2_MOD_CASCAM_TRCAS = y ]];then
		echo "#define CASCAM_TENGRUICAS_SUPPORT">>$config_file_h
		cas_name=trcas
	fi
	if [[ $BR2_MOD_CASCAM_DVTCAS = y ]];then
		echo "#define CASCAM_DVTCAS_SUPPORT">>$config_file_h
		cas_name=dvtcas
	fi
	if [[ $BR2_MOD_CASCAM_DESAICAS = y ]];then
		echo "#define CASCAM_DESAICAS_SUPPORT">>$config_file_h
		cas_name=desaicas
	fi
	if [[ $BR2_MOD_CASCAM_CONAXCAS = y ]];then
		echo "#define CASCAM_CONAXCAS_SUPPORT">>$config_file_h
		cas_name=conaxcas
	fi
	if [[ $BR2_MOD_CASCAM_CRYPTOGUARDCAS = y ]];then
		echo "#define CASCAM_CGCAS_SUPPORT">>$config_file_h
		cas_name=cryptoguardcas
	fi
	if [[ $BR2_MOD_CASCAM_GOSCAS = y ]];then
		echo "#define CASCAM_GOSCAS_SUPPORT">>$config_file_h
		cas_name=goscas
	fi
	if [[ $BR2_MOD_CASCAM_IRDETOCAS = y ]];then
		echo "#define CASCAM_IRDETOCAS_SUPPORT">>$config_file_h
		cas_name=irdetocas
	fi
	if [[ $BR2_MOD_CASCAM_VMXCAS = y ]];then
		echo "#define CASCAM_VMXCAS_SUPPORT">>$config_file_h
		cas_name=vmxcas
	fi
	if [[ $BR2_MOD_CASCAM_TELECASTCAS = y ]];then
		echo "#define CASCAM_TELECASTCAS_SUPPORT">>$config_file_h
		cas_name=telecastcas
	fi
	if [[ $BR2_MOD_CASCAM_CTICAS = y ]];then
		echo "#define CASCAM_CTICAS_SUPPORT">>$config_file_h
		cas_name=cticas
	fi
	if [[ $BR2_MOD_CASCAM_NAGRACAS = y ]];then
		echo "#define CASCAM_NAGRACAS_SUPPORT">>$config_file_h
		cas_name=nagracas
	fi
	echo "cas_name : ${cas_name}"

	if [ -e ${config_build_dir}/app/ca/export/${ARCH}-${OS}/firmware/${cas_name}/gxsecure.fw ]; then
		if [ $OS = "ecos" ];then
			build_image_temp_dir=${GXSRC_PATH}/output/image/bin_ecos
		fi
		mkdir -p ${build_image_temp_dir}/rootfs_ecos/lib/firmware
		cp -rf ${config_build_dir}/app/ca/export/${ARCH}-${OS}/firmware/${cas_name}/gxsecure.fw ${build_image_temp_dir}/rootfs_ecos/lib/firmware
	else
		echo -e "\033[031m------------------------------------------------------\033[0m"
		echo -e "\033[031mWARNING : Don't find the gxsecure.fw!\033[0m"
		echo -e "\033[031m------------------------------------------------------\033[0m"
	fi

	if [ -e ${GXLIB_PATH}/include/app/ca ];then
		rm ${GXLIB_PATH}/include/app/ca -rf
	fi
	mkdir -p ${GXLIB_PATH}/include/app/ca
	cp  ${config_build_dir}/app/ca/export/${ARCH}-${OS}/libcascam/${cas_name}/libgxcascam.a ${config_build_dir}/lib/${ARCH}-${OS} -rf
	cp  ${config_build_dir}/app/ca/export/${ARCH}-${OS}/cascam/* ${GXLIB_PATH}/include/app/ca -rf
	cp  ${config_build_dir}/app/ca/include/*.h ${GXLIB_PATH}/include/app/ca -rf
	cp  ${config_build_dir}/app/ca/include/common/* ${GXLIB_PATH}/include/app/ca -rf
	cp  ${config_build_dir}/app/ca/include/porting/* ${GXLIB_PATH}/include/app/ca -rf
	cp  ${config_build_dir}/app/ca/export/${ARCH}-${OS}/cas/${cas_name}/*.a ${config_build_dir}/lib/${ARCH}-${OS} -rf
	cp  ${config_build_dir}/app/ca/include/cas/${cas_name}/* ${GXLIB_PATH}/include/app/ca -rf

	if [[ $BR2_THEME_classic_blue  = y ]];then
		if [ -e ${config_build_dir}/app/ca/cas/${cas_name}/1280X720/widget ];then
			mkdir -p ${config_theme_dst_dir}/widget/ca/${cas_name}
			mkdir -p ${config_theme_dst_dir}/widget/ca/common_menu
			mkdir -p ${config_theme_dst_dir}/image/ca/
			cp ${config_build_dir}/app/ca/cas/${cas_name}/1280X720/widget/* ${config_theme_dst_dir}/widget/ca/${cas_name} -rf
				cp ${config_build_dir}/app/ca/common_menu/1280X720/widget/* ${config_theme_dst_dir}/widget/ca/common_menu -rf
			cp ${config_build_dir}/app/ca/cas/${cas_name}/1280X720/image/* ${config_theme_dst_dir}/image/ca/ -rf
		else
			echo -e "\033[031m------------------------------------------------------\033[0m"
			echo -e "\033[031mWARNING : Don't find the ${cas_name} widget (1280*720)!\033[0m"
			echo -e "\033[031mWARNING : Use the ${cas_name} widget (1024*576)!\033[0m"
			echo -e "\033[031m------------------------------------------------------\033[0m"
			if [ -e ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/widget ];then
				mkdir -p ${config_theme_dst_dir}/widget/ca/${cas_name}
				mkdir -p ${config_theme_dst_dir}/widget/ca/common_menu
				mkdir -p ${config_theme_dst_dir}/image/ca/
				cp ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/widget/* ${config_theme_dst_dir}/widget/ca/${cas_name} -rf
				cp ${config_build_dir}/app/ca/common_menu/1024X576/widget/* ${config_theme_dst_dir}/widget/ca/common_menu -rf
				cp ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/image/* ${config_theme_dst_dir}/image/ca/ -rf
			else
				echo -e "\033[031m------------------------------------------------------\033[0m"
				echo -e "\033[031mWARNING : Don't find the ${cas_name} widget (1024*576)!\033[0m"
				echo -e "\033[031m------------------------------------------------------\033[0m"
			fi
		fi
	else
		if [ -e ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/widget ];then
			mkdir -p ${config_theme_dst_dir}/widget/ca/${cas_name}
			mkdir -p ${config_theme_dst_dir}/widget/ca/common_menu
			mkdir -p ${config_theme_dst_dir}/image/ca/
			cp ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/widget/* ${config_theme_dst_dir}/widget/ca/${cas_name} -rf
				cp ${config_build_dir}/app/ca/common_menu/1024X576/widget/* ${config_theme_dst_dir}/widget/ca/common_menu -rf
			cp ${config_build_dir}/app/ca/cas/${cas_name}/1024X576/image/* ${config_theme_dst_dir}/image/ca/ -rf
		else
			echo -e "\033[031m------------------------------------------------------\033[0m"
			echo -e "\033[031mWARNING : Don't find the ${cas_name} widget (1024*576)!\033[0m"
			echo -e "\033[031m------------------------------------------------------\033[0m"
		fi
	fi

else
	#echo -e "\033[031m------------------------------------------------------\033[0m"
	#echo -e "\033[031m Are you sure you donot need CASCAM supported?\033[0m"
	#echo -e "\033[031m------------------------------------------------------\033[0m"
	echo "#define CASCAM_SUPPORT 0">>$config_file_h
	if [ -e ${GXLIB_PATH}/include/app/ca ];then
		rm ${GXLIB_PATH}/include/app/ca -rf
	fi
	if [ -e ${config_build_dir}/lib/${ARCH}-${OS}/libgxcascam.a ];then
		rm ${config_build_dir}/lib/${ARCH}-${OS}/libgxcascam.a
	fi
	if [ -e ${config_build_dir}/lib/${ARCH}-${OS}/libcd.a ];then
		rm ${config_build_dir}/lib/${ARCH}-${OS}/libcd.a
	fi
	if [ -e ${config_build_dir}/lib/${ARCH}-${OS}/libabv.a ];then
		rm ${config_build_dir}/lib/${ARCH}-${OS}/libabv.a
	fi
fi

echo "">>$config_file_h


if [ $BR2_MOD_CASCAM_VMXCAS = y ];then
	value=${GXSRC_PATH}/app/ca/include/cas/vmxcas/app_vmxcas_config.h
	mode=`cat $value | grep "VMXCAS_ADV_PLUS" | awk '{print $3}'`
	if [ $mode = 1 ];then
	makefile_path=${GXSRC_PATH}/app/Makefile
	strA="LDFLAGS += -lvmxsec_ree -lgxtac_ree -lteec -lgxcascam -lxdevapi -l_vmx_ree_GX6631_NCP_VMXSEC_VCNScHRLpIOOc_NCP_CANOPUS_20230227-230420154446 -lvmxsec_ree -lgxtac_ree -lxdevapi -lteec -ltee-supplicant -lvmxtac_test_ree_TAC_NCP_CANOPUS"
	result=$(cat $makefile_path | grep "$strA")
	if [ "$result" != "" ];then
		echo "ecos vmx lib already -l"
	else
		sed -i '/-lgxsecure/a  LDFLAGS += -lvmxsec_ree -lgxtac_ree -lteec -lgxcascam -lxdevapi -l_vmx_ree_GX6631_NCP_VMXSEC_VCNScHRLpIOOc_NCP_CANOPUS_20230227-230420154446 -lvmxsec_ree -lgxtac_ree -lxdevapi -lteec -ltee-supplicant -lvmxtac_test_ree_TAC_NCP_CANOPUS' $makefile_path
	fi
	strB="LDFLAGS += -l_vmx_ree_GX6631_NCP_VMXSEC_VCNScHRLpIOOc_NCP_CANOPUS_20230227-230420154446 -lvmxsec_ree -lgxtac_ree -lxdevapi -lteec -lvmxtac_test_ree_TAC_NCP_CANOPUS"
	result=$(cat $makefile_path | grep "$strB")
	if [ "$result" != "" ];then
		echo "ecos vmx lib already -l"
	else
		num=`grep -rn "lqrencode" ${makefile_path} | grep -v "lgxsecure" | awk -F ':' '{print $1}'`
		echo "$num"
		sed -i "$num aLDFLAGS += -l_vmx_ree_GX6631_NCP_VMXSEC_VCNScHRLpIOOc_NCP_CANOPUS_20230227-230420154446 -lvmxsec_ree -lgxtac_ree -lxdevapi -lteec -lvmxtac_test_ree_TAC_NCP_CANOPUS" $makefile_path
	fi
	fi
fi

