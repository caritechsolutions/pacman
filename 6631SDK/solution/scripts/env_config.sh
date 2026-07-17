#!/bin/bash

config_build_dir=$(pwd)
config_list_dir=${config_build_dir}/configs

if [[ ! ${solution} ]] || [[ ! -f ${solution}/.config ]];then
    config_list=$(ls ${config_list_dir})

    echo -e "\033[33;1mPlease Choose one from below for [solution config]\033[0m\n"
    index=0
    for p in $config_list;
    do
        let index++
        array[$index]=$p
        echo -e "\t\033[32;1m[$index] $p\033[0m"
    done
    echo -en "\n\033[33;1mInput config num (1-${#array[*]}): \033[0m"
    read -e NUM
    if [[ $NUM -gt ${#array[*]} || $NUM -le 0 ]];then
        echo -en "\n\033[31;1mInput profile num out of range!! \033[0m\n"
        exit 1
    fi
    CONFIG=${array[$NUM]}
    cp ${config_list_dir}/$CONFIG ${config_build_dir}/.config || exit 1
fi
