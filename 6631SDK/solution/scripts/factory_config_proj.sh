#!/bin/bash
##############################################################################
# Blacker, liujh@nationalchip.com
# Copyright(C) 2013-2020, HangZhou NationalChip Science and Technology Corp.
# Generate Config.in file for projects config
##############################################################################

BUILD_DIR=$(pwd)
config_proj_path=${BUILD_DIR}/projects
config_proj_conf=${BUILD_DIR}/system/configin/Config.in.proj
config_frontend_board_conf=${BUILD_DIR}/system/configin/Config.in.frontend_config

##############################################################################
# Output Family Config
##############################################################################
echo -e "\033[032mGenerate Projects Config for each family\033[0m"
config_family_list=$(ls -l ${config_proj_path} | awk '/^d/ {print $NF}')
echo "Project Family List:" $config_family_list

touch $config_proj_conf
echo "" > $config_proj_conf

if [[ $config_family_list != "" ]];then
	echo "choice" >> $config_proj_conf
	echo "    prompt \"Vendor Name\"" >> $config_proj_conf
	echo "    help" >> $config_proj_conf
	echo "      Select Project Family" >> $config_proj_conf
	echo "" >> $config_proj_conf
fi

# output project family config
for each_family in $config_family_list
do
	family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
    chip_os=$(ls -l ${config_proj_path}/${each_family} | awk '/^d/ {print $NF}' | cut -f -2 -d "_"| sort -u)
	echo "config BR2_FAMILY_${family_name_cap}" >> $config_proj_conf
	echo "  bool \"${family_name_cap}\"" >> $config_proj_conf
    str=""
    for dep in $chip_os
    do
        chip=$(echo $dep | cut -f 1 -d "_" | tr [:lower:] [:upper:])
        os=$(echo $dep | cut -f 2 -d "_" | tr [:lower:] [:upper:])
        str=$str$(echo "(BR2_CHIP_${chip} && BR2_OS_${os}) || ") >> $config_proj_conf
    done
    str=${str%||*}
    echo "    depends on $str" >> $config_proj_conf
	echo "" >> $config_proj_conf
done

if [[ $config_family_list != "" ]];then
	echo "" >> $config_proj_conf
	echo "endchoice" >> $config_proj_conf
	echo "" >> $config_proj_conf
	echo "config BR2_FAMILY_NAME" >> $config_proj_conf
	echo "    string" >> $config_proj_conf
fi

# output project family name
for each_family in $config_family_list
do
	family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
	echo "    default \"${each_family}\" if BR2_FAMILY_${family_name_cap}" >> $config_proj_conf
done
echo "" >> $config_proj_conf

##############################################################################
# Output Projects config
##############################################################################
declare -a board_list
declare -a tuner_list
for each_family in $config_family_list
do
    family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
    config_board_list=$(ls -l ${config_proj_path}/${each_family} | awk '/^d/ {print $NF}')
    echo -e "Family: \033[033m${family_name_cap}\033[0m"
    echo "Project list :"
    echo " "$config_board_list"  " | sed 's/ /\n\t/g'

    unset board_list
    index=0
    for each_proj in $config_board_list
    do
       prj=`echo "$each_proj" | cut -f 1-3 -d '_'`
       flag=`echo ${board_list[@]} | grep -w "${prj}_"`
       if [ ! -n "$flag" ];then
           board_list[$index]=${prj}
           index=$((index+1))
       fi
    done
    if [[ $board_list != "" ]];then
        echo "if BR2_FAMILY_${family_name_cap}" >> $config_proj_conf
        echo "" >> $config_proj_conf

        echo "choice" >> $config_proj_conf
        echo "    prompt \"Board Name\"" >> $config_proj_conf
        echo "" >> $config_proj_conf


        # output project config item
        for each_proj in ${board_list[@]}
        do
            #echo ${each_proj}
            proj_name_cap=$(echo $each_proj | tr [:lower:] [:upper:])
            chip_name=$(echo $proj_name_cap | awk -F "_" '{print $1}')
            os_name=$(echo $proj_name_cap | awk -F "_" '{print $2}')
            board_name=$(echo $proj_name_cap | awk -F "_" '{print $3}')
            tuner_type=$(echo $proj_name_cap | awk -F "_" '{print $4}')

            echo "config BR2_PROJ_${family_name_cap}_${chip_name}_${os_name}_${board_name/\-/}" >> $config_proj_conf
            echo "    bool \"${board_name}\"" >> $config_proj_conf
            echo "    depends on BR2_CHIP_${chip_name} && BR2_OS_${os_name}" >> $config_proj_conf
            echo "    help" >> $config_proj_conf
            echo "      ${board_name} ${chip_name} ${os_name}" >> $config_proj_conf
            echo "" >> $config_proj_conf
        done

        echo "endchoice" >> $config_proj_conf
        echo "" >> $config_proj_conf


        echo "config BR2_BOARD_NAME" >> $config_proj_conf
        echo "    string" >> $config_proj_conf
        for list in $config_board_list
        do
            family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
            chip_name_cap=$(echo $list | awk -F "_" '{print $1}' | tr [:lower:] [:upper:])
            os_name_cap=$(echo $list | awk -F "_" '{print $2}' | tr [:lower:] [:upper:])
            board_name=$(echo $list | awk -F "_" '{print $3}')
            board_name_cap=$(echo $list | awk -F "_" '{print $3}' | tr [:lower:] [:upper:])
            echo "    default \"${board_name}\" if BR2_PROJ_${family_name_cap}_${chip_name_cap}_${os_name_cap}_${board_name_cap/\-/}" >> $config_proj_conf
       done
        echo "" >> $config_proj_conf
        echo "endif" >> $config_proj_conf
        echo "" >> $config_proj_conf
    fi

##########
#for tuner type
##########
    unset tuner_list
    index=0
    for each_proj in $config_board_list
    do
       prj=`echo "$each_proj" | cut -f 1-4 -d '_'`
       flag=`echo ${board_list[@]} | grep -w ${prj}`
       if [ ! -n "$flag" ];then
           board_list[$index]=${prj}
           index=$((index+1))
       fi
    done
    for each_tuner in ${config_board_list}
    do
        flag=`echo ${tuner_list[@]} | grep -w ${each_tuner}`
        if [ ! -n "$flag" ];then
            tuner_list[$index]=${each_tuner}
            index=$((index+1))
        fi
    done

    #echo ${tuner_list[@]}
    if [[ $board_list != "" ]];then
        echo "if BR2_FAMILY_${family_name_cap}" >> $config_proj_conf
        echo "" >> $config_proj_conf

        echo "choice" >> $config_proj_conf
        echo "    prompt \"Tuner type\"" >> $config_proj_conf
        echo "" >> $config_proj_conf
        for each_proj in ${tuner_list[@]}
        do
            proj_name_cap=$(echo $each_proj | tr [:lower:] [:upper:])
            chip_name=$(echo $proj_name_cap | awk -F "_" '{print $1}')
            os_name=$(echo $proj_name_cap | awk -F "_" '{print $2}')
            board_name=$(echo $proj_name_cap | awk -F "_" '{print $3}')
            tuner_type=$(echo $proj_name_cap | awk -F "_" '{print $4}')

            echo "config BR2_PROJ_${family_name_cap}_${chip_name}_${os_name}_${board_name/\-/}_${tuner_type}" >> $config_proj_conf
            echo "    bool \"${tuner_type}\"" >> $config_proj_conf
            echo "    depends on BR2_CHIP_${chip_name} && BR2_OS_${os_name} && BR2_PROJ_${family_name_cap}_${chip_name}_${os_name}_${board_name/\-/}" >> $config_proj_conf
            echo "    help" >> $config_proj_conf
            echo "      ${chip_name} ${os_name} ${board_name} ${tuner_type}" >> $config_proj_conf
            echo "" >> $config_proj_conf
        done
        echo "endchoice" >> $config_proj_conf
        echo "" >> $config_proj_conf

       echo "config BR2_TUNER_TYPE" >> $config_proj_conf
       echo "    string" >> $config_proj_conf
       for list in $config_board_list
       do
            family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
            chip_name_cap=$(echo $list | awk -F "_" '{print $1}' | tr [:lower:] [:upper:])
            os_name_cap=$(echo $list | awk -F "_" '{print $2}' | tr [:lower:] [:upper:])
            board_name_cap=$(echo $list | awk -F "_" '{print $3}' | tr [:lower:] [:upper:])
            tuner_type=$(echo $list | awk -F "_" '{print $4}')
            tuner_type_cap=$(echo $list | awk -F "_" '{print $4}'| tr [:lower:] [:upper:])
            echo "    default \"${tuner_type}\" if BR2_PROJ_${family_name_cap}_${chip_name_cap}_${os_name_cap}_${board_name_cap/\-/}_${tuner_type_cap}" >> $config_proj_conf
       done

        echo "endif" >> $config_proj_conf
        echo "" >> $config_proj_conf
    fi
done
############
#for proj name
###########
echo "config BR2_PROJ_NAME" >> $config_proj_conf
echo "    string" >> $config_proj_conf
for each_family in $config_family_list
do
    config_board_list=$(ls -l ${config_proj_path}/${each_family} | awk '/^d/ {print $NF}')
    for list in $config_board_list
    do
        list_cap=$(echo $list | tr [:lower:] [:upper:])
        family_name_cap=$(echo $each_family | tr [:lower:] [:upper:])
        chip_name=$(echo $list_cap | awk -F "_" '{print $1}')
        os_name=$(echo $list_cap | awk -F "_" '{print $2}')
        board_name=$(echo $list_cap | awk -F "_" '{print $3}')
        tuner_type=$(echo $list_cap | awk -F "_" '{print $4}')
        echo "    default \"${list}\" if BR2_CHIP_${chip_name} && BR2_OS_${os_name} && BR2_PROJ_${family_name_cap}_${chip_name}_${os_name}_${board_name/\-/} && BR2_PROJ_${family_name_cap}_${chip_name}_${os_name}_${board_name/\-/}_${tuner_type}" >> $config_proj_conf
    done
done
echo "" >> $config_proj_conf

##############################################################################
# Gerate Frontend Kconfig
##############################################################################
config_frontend_list=$(ls -l ${BUILD_DIR}/frontend_config | awk '/^-/ {print $NF}')
echo ""
echo "frontend list:"
echo "#Do not edit this file! Auto generated by factory_config_proj.sh" > $config_frontend_board_conf
echo "choice" >> $config_frontend_board_conf
echo "    prompt \"Frontend Board Config SELECT\"" >> $config_frontend_board_conf
echo "    help" >> $config_frontend_board_conf
echo "    select a default frontend config to match your board" >> $config_frontend_board_conf
echo "" >> $config_frontend_board_conf

for each_frontend in $config_frontend_list
do
    frontend_name_cap=$(echo $each_frontend | tr [:lower:] [:upper:])
    frontend_chip_name=$(echo $frontend_name_cap | awk -F "_" '{print $1}')
    frontend_mode=$(echo $frontend_name_cap | awk -F "_" '{print $2}')
    if [[ $frontend_name_cap = "README.TXT" ]];then
        continue
    fi

    echo "        "$each_frontend
    echo "config BR2_FE_CONFIG_"$frontend_name_cap >> $config_frontend_board_conf
    echo "    bool \""$frontend_name_cap"\"" >> $config_frontend_board_conf
    echo "    depends on BR2_CHIP_"$frontend_chip_name >> $config_frontend_board_conf
    echo "" >> $config_frontend_board_conf

done
echo ""
echo "endchoice" >> $config_frontend_board_conf


