#!/usr/bin/env python2
#-*- coding:utf-8 -*-

################################################################################
# Copyright © 2019-2029 NationalChip Corporation, All Rights Reserved
# This script is used to analyze frontend config in frontend_config dir,
# and generate macro define to app_frontend_board_config.h.
# This script is based on Python 2.7.12 or 3.5.2, and there is no dependency.
################################################################################
import json
from collections import OrderedDict
import subprocess
import sys, os, re
import shutil
reload(sys)
sys.setdefaultencoding('utf-8')

SOLUTION_PATH = sys.argv[1]
PROJECT_CONFIG_PATH = SOLUTION_PATH + "/.config"
CONFIG_PATH = SOLUTION_PATH + "/system/configin"
NETAPPS_PATH = SOLUTION_PATH + '/app/netapps/'
OUTPUT_TEMP_FILE = NETAPPS_PATH + 'allconfig.json'
OUTPUT_TEMP_PATH = ''
configin_file = CONFIG_PATH+"/Config.in.netapps"
project_config_json = "netappsmenu-config.json"

allconfig = ''
load_dict_all = {}
config_in_netapps= {}

def get_by_shell(cmd):
    #print(cmd)
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if '' == err:
        return out.strip()
    else:
        return ''

def check_support(pathname):
    #print(NETAPPS_PATH)
    #for i in os.listdir(NETAPPS_PATH):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    if os.path.isdir(fulldirct): #入参需要是绝对路径
        ConfigDirct = os.path.join(fulldirct, 'config.json')
        if os.path.isfile(ConfigDirct): #入参需要是绝对路径
            return ConfigDirct
    return ''


def get_netapp_config(pathname):
    file_name = check_support(pathname)
    if '' != file_name:
        try:
            with open(file_name, 'r') as fj:
                parsed_json = json.load(fj, object_pairs_hook=OrderedDict)
        except ValueError as e:
            print("can't parse the config.json in ",file_name)
        else:
                make_config_json(pathname,parsed_json)

def remove_ouput_file():
    if os.path.isfile(OUTPUT_TEMP_FILE):
        os.remove(OUTPUT_TEMP_FILE)

def remove_configin_file():
    if os.path.isfile(configin_file):
        os.remove(configin_file)

def make_config_json(appname,jsonsting):
    global allconfig
    global load_dict_all
    load_dict_all[appname]=jsonsting

def do_all_netapps():
    global allconfig
    global load_dict_all
    with open(OUTPUT_TEMP_FILE, 'w+') as allconfig:
        for i in os.listdir(NETAPPS_PATH):
            if('protect' == i):
                fulldirct = NETAPPS_PATH + 'protect'
                for j in os.listdir(fulldirct):
                    pathname = i + '/' + j
                    get_netapp_config(pathname)
            else:
                get_netapp_config(i)
        json.dump(load_dict_all,allconfig, indent=4)

def get_project_json_path():
    project_json = ""
    family_name = None
    proj_name = None
    FAMILY_CONFIG_PERFIX = "BR2_FAMILY_NAME="
    PROJECT_CONFIG_PERFIX = "BR2_PROJ_NAME="

    if os.path.isfile(PROJECT_CONFIG_PATH):
        with open(PROJECT_CONFIG_PATH) as f:
            for config in f.readlines():
                if FAMILY_CONFIG_PERFIX in config:
                    family_name = config[len(FAMILY_CONFIG_PERFIX)+1:-2]    # 读取的内容带双引号和换行，需要去掉
                if PROJECT_CONFIG_PERFIX in config:
                    proj_name = config[len(PROJECT_CONFIG_PERFIX)+1:-2]     # 读取的内容带双引号和换行，需要去掉
            if (family_name != None)&(proj_name != None):
                project_json = SOLUTION_PATH+"/projects"+"/"+family_name+"/"+proj_name+"/"+project_config_json
                print(project_json)
            else:
                project_json = ""

    else:
        print("can't found .config file")
        project_json = ""

    return project_json

def add_configin_info(menuconfig_id, menuconfig_name, default_value=None, depends_on=None):
    config_in_netapps_content=[]
    config_in_netapps_content.append("config "+menuconfig_id+"\n")
    config_in_netapps_content.append("\tbool \""+menuconfig_name+"\"\n")
    if default_value != None:
        config_in_netapps_content.append("\tdefault "+default_value+"\n")
    if depends_on != None:
        config_in_netapps_content.append("\tdepends on "+depends_on+"\n")
    config_in_netapps_content.append("\n")
    config_in_netapps[menuconfig_id]=config_in_netapps_content


def add_configin_into_file():
    sort_info=sorted(config_in_netapps.items(),key=lambda x:x[0])
    with open(CONFIG_PATH+"/Config.in.netapps", "w+") as f:
        for i in sort_info:
            for j in i[1]:
                f.write(j)


def create_netapps_configin(netapps_json, allconfig_json):
    if os.path.isfile(netapps_json):
        try:
            with open(netapps_json, "rb") as f:
                js_project = json.load(f, object_pairs_hook=OrderedDict)
        except ValueError as e:
            print("can't parse the config.json in ",netapps_json)
            js_project = None
    else:
        print("can't found netappsmenu-config.json in project folder")
        js_project = None
    try:
        with open(allconfig_json, "rb") as f:
            js_netapp = json.load(f, object_pairs_hook=OrderedDict)
    except ValueError as e:
            print("can't parse the config.json in ",allconfig_json)
            js_netapp = None


    if js_netapp == None:
        return
    if js_project != None:
        for k in js_project["Netapps"]:
            for i in js_netapp:
                menuconfig_name = None
                menuconfig_id = None
                ui_entry = None
                sub_define = None
                default_value = "y"

                for j in js_netapp[i]:
                    if j.decode("utf-8") == "MENUCONFIG_NAME":
                        menuconfig_name = js_netapp[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENUCONFIG_ID":
                        menuconfig_id = js_netapp[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "APPUI_ENTRY":
                        ui_entry = js_netapp[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "SUB_DEFINES":
                        sub_define = js_netapp[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "DEFAULT_CLOSE":
                        default_value = js_netapp[i][j.decode("utf-8")]
                        if 'yes' == default_value:
                            default_value = "n"
                        elif 'no' == default_value:
                            default_value = "y"
                        else:
                            default_value = "y"

                if (ui_entry == k)&(menuconfig_name != None)&(menuconfig_id != None):
                    add_configin_info(menuconfig_id, menuconfig_name, default_value)
                    if (sub_define != None):
                        for h in sub_define:
                            sub_menuconfig_name = None
                            sub_menuconfig_id = None
                            for g in h:
                                if g.decode("utf-8") == "MENUCONFIG_NAME":
                                    sub_menuconfig_name = h[g.decode("utf-8")]
                                if g.decode("utf-8") == "MENUCONFIG_ID":
                                    sub_menuconfig_id = h[g.decode("utf-8")]

                                if (sub_menuconfig_name != None)&(sub_menuconfig_id != None):
                                    add_configin_info(sub_menuconfig_id, sub_menuconfig_name, default_value, menuconfig_id)


    for i in js_netapp:
        menuconfig_name = None
        menuconfig_id = None
        ui_entry = None
        sub_define = None
        default_value = "y"
        flag = True
        for j in js_netapp[i]:
            if j.decode("utf-8") == "MENUCONFIG_NAME":
                menuconfig_name = js_netapp[i][j.decode("utf-8")]
            if j.decode("utf-8") == "MENUCONFIG_ID":
                menuconfig_id = js_netapp[i][j.decode("utf-8")]
            if j.decode("utf-8") == "APPUI_ENTRY":
                ui_entry = js_netapp[i][j.decode("utf-8")]
            if j.decode("utf-8") == "SUB_DEFINES":
                sub_define = js_netapp[i][j.decode("utf-8")]
            if j.decode("utf-8") == "DEFAULT_CLOSE":
                default_value = js_netapp[i][j.decode("utf-8")]
                if 'yes' == default_value:
                    default_value = "n"
                elif 'no' == default_value:
                    default_value = "y"
                else:
                    default_value = "y"

        if js_project != None:
            for k in js_project["Netapps"]:
                if ui_entry == k:
                    flag = False
        else:
            flag = True
        if (flag == True)&(menuconfig_name != None)&(menuconfig_id != None):
            add_configin_info(menuconfig_id, menuconfig_name, default_value)
            if (sub_define != None):
                sub_menuconfig_name = None
                sub_menuconfig_id = None
                for h in sub_define:
                    sub_menuconfig_name = None
                    sub_menuconfig_id = None
                    for g in h:
                        if g.decode("utf-8") == "MENUCONFIG_NAME":
                            sub_menuconfig_name = h[g.decode("utf-8")]
                        if g.decode("utf-8") == "MENUCONFIG_ID":
                            sub_menuconfig_id = h[g.decode("utf-8")]

                        if (sub_menuconfig_name != None)&(sub_menuconfig_id != None):
                            add_configin_info(sub_menuconfig_id, sub_menuconfig_name, default_value, menuconfig_id)

remove_configin_file()
remove_ouput_file()
do_all_netapps()
create_netapps_configin(get_project_json_path(), OUTPUT_TEMP_FILE)
remove_ouput_file()
add_configin_into_file()
