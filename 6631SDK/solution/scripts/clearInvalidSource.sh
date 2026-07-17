#!/bin/bash

path=$(cd `dirname $0`;pwd)
tar_path=$path/../output/theme
solution_path=$path/..

declare -A wnd_dict
wnd_dict["IPTV_SUPPORT"]="wnd_iptv_browser.xml,wnd_iptv_info.xml,wnd_iptv_list.xml,wnd_iptv_number.xml,wnd_iptv_play.xml,wnd_iptv.xml"
wnd_dict["GAME_ENABLE"]="wnd_tetris.xml,wnd_snake.xml"
wnd_dict["SAT_FINDER_SUPPORT"]="wnd_satfinder.xml"
wnd_dict["MOD_3G_SUPPORT"]="wnd_3g.xml"
wnd_dict["NETAPPS_SUPPORT"]="wnd_apps_netvideo_play.xml,wnd_apps_netvideo.xml"
wnd_dict["DLNADMR_SUPPORT"]="wnd_dlna_dmr.xml"
wnd_dict["DLNADMP_SUPPORT"]="wnd_dlna_dmp.xml"
wnd_dict["DLNADMS_SUPPORT"]="wnd_dlna_dms.xml"
wnd_dict["FACTORY_TEST_SUPPORT"]="wnd_factory_test.xml"
wnd_dict["CVBS_TEST_SUPPORT"]="wnd_cvbs_test.xml"
wnd_dict["TKGS_SUPPORT"]="wnd_tkgs_upgrade_process.xml,wnd_visiblelocation_popup.xml,wnd_visible_location.xml,wnd_preferred_list.xml,wnd_hidden_location.xml"
wnd_dict["KEY_SCANCODE_SUPPORT"]="wnd_key_scancode.xml"
wnd_dict["MAP_SUPPORT"]="wnd_map.xml"
wnd_dict["MERGE_DB_SUPPORT"]="wnd_merge_db.xml"
wnd_dict["NET_UPGRADE_SUPPORT"]="wnd_netupg_process.xml,wnd_net_upgrade.xml"
wnd_dict["NETWORK_SUPPORT"]="wnd_network.xml,wnd_plugin_play_info.xml"
wnd_dict["OTA_SUPPORT"]="wnd_ota_upgrade.xml"
wnd_dict["RECALL_LIST_SUPPORT"]="wnd_recall_list.xml"
wnd_dict["DVB_CLOUD_SUPPORT"]="wnd_satellite_online_list.xml,wnd_sat_source_select.xml,wnd_tp_source_select.xml"
wnd_dict["DEMOD_DTMB"]="wnd_search_setting_dtmb.xml"
wnd_dict["DEMOD_DVB_C"]="wnd_search_setting_frequency.xml,wnd_search_setting_dvbc.xml"
wnd_dict["DEMOD_DVB_T"]="wnd_search_setting_dvbt.xml"
wnd_dict["UNICABLE_SUPPORT"]="wnd_unicable_set_popup.xml"
wnd_dict["WIFI_SUPPORT"]="wnd_wifi.xml"
#wnd_dict["MEDIA_FILE_EDIT_SUPPORT"]="win_file_edit.xml"

function clear_invalid_translate(){
    count=0
    echo "#include\"app_config.h\"" > word.c
    echo "#include\"app_str_id.h\"" >> word.c
    grep -r "STR_ID" $solution_path/app/include/app_str_id.h | cut -f1 -d"\""|cut -f2 -d" " >>word.c
    csky-elf-gcc -E -I $solution_path/app/include/ word.c -o word_out
    grep -r "^STR_ID" word_out > useless_id
    echo "">>useless_id
    lang_xml=`grep -r "#" $tar_path/language/language| cut -f2 -d"#"`
    while read line
    do
        word=`grep -r "$line " $solution_path/app/include/app_str_id.h |cut -f2 -d"\""`
        if [[ $word =~ "/" ]];then
            continue
        fi
        let count=count+1
        for l in $lang_xml
        do
            sed "/$word/d" -i $tar_path/language/$l.xml
        done
    done<useless_id
    echo "Del $count unuse translate word"
    rm word.c
    rm word_out
    rm useless_id
}

function clear_invalid_wnd_xml(){
    for i in ${!wnd_dict[*]}
    do
        v=`grep -r "$i " $solution_path/app/include/app_config.h |cut -f3 -d" "`
        if [[ $v -eq "0" ]];then
            wnd=${wnd_dict[$i]}
            len=`echo $wnd | awk -F"," '{print NF}'`
            for((i=1;i<=$len;i++))
            do
                wnd_xml=`echo $wnd | cut -f $i -d","`
                echo "Del invalid wnd xml $wnd_xml"
                rm $tar_path/widget/$wnd_xml
            done
        fi
    done
}

function clear_invalid_img(){
    while read line
    do
        if [[ $line =~ "id=" ]];then
            image_id=`echo $line | cut -f2 -d "\""`
            image_path=`echo $line |cut -f2 -d">" |cut -f1 -d"<" `
            grep -rq "$image_id" $tar_path/widget $tar_path/style.xml $solution_path/app/
            if [ $? -ne 0 ];then
                echo "Del invalid img $image_path"
                rm $tar_path/image/$image_path
            fi
        fi
    done<$tar_path/image/image.xml
}

clear_invalid_translate
clear_invalid_wnd_xml
clear_invalid_img
