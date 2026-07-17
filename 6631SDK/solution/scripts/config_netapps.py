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
reload(sys)
sys.setdefaultencoding('utf-8')

SOLUTION_PATH = sys.argv[1]
PROJECTS_PATH = sys.argv[2]
THEME_NAME = sys.argv[3]
ARCH_NAME = sys.argv[4]
OS_NAME = sys.argv[5]
OUTPUT_NETAPPSMENU_CONFIG = PROJECTS_PATH+ '/netappsmenu-config.json'
NETAPPS_PATH = SOLUTION_PATH + '/app/netapps/'
OUTPUT_TEMP_FILE = SOLUTION_PATH + '/output/allconfig.json'
THEME_DST_DIR = SOLUTION_PATH + '/output/theme/'
OUTPUT_APPUI_FILE = SOLUTION_PATH + '/app/netapps/netappsmenu-config.json'
OUTPUT_MAINMENU_INI_TEMP = SOLUTION_PATH + '/projects/main_menu_temp.ini'
OUTPUT_MAKEFILE_NETAPPS = SOLUTION_PATH + '/app/Makefile_netapps'
PATH_MAINMENU_INI = SOLUTION_PATH + '/projects/main_menu.ini'
PATH_SOLUTION_CONFIG = SOLUTION_PATH + '/.config'


allconfig = ''
appmenu = ''
mainmenu_ini_temp = ''
makefile_netapps = ''
load_dict_all = {}
load_dict_apps = {}
appslist = []



def get_by_shell(cmd):
    #print(cmd)
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if '' == err:
        return out.strip()
    else:
        return ''



def check_support(pathname):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    #print("fulldirct:",fulldirct)
    if os.path.isdir(fulldirct):
        ConfigDirct = os.path.join(fulldirct, 'config.json')
        if os.path.isfile(ConfigDirct):
            return ConfigDirct
    return ''


def get_netapp_config(pathname):
    file_name = check_support(pathname)
    if '' != file_name:
        if check_switch(pathname):
            cp_image(pathname)
            cp_title_image(pathname)
            cp_language(pathname)
            cp_widget(pathname)
        make_one_app_makefile(pathname)
        try:
            with open(file_name, 'r') as fj:
                parsed_json = json.load(fj, object_pairs_hook=OrderedDict)
        except ValueError as e:
            print("can't parse the config.json in ",file_name)
        else:
            make_config_json(pathname,parsed_json)
            make_appui_entry(parsed_json)
            make_mainmenu_ini_temp(parsed_json)




def remove_ouput_file():
    if os.path.isfile(OUTPUT_TEMP_FILE):
        os.remove(OUTPUT_TEMP_FILE)
    if os.path.isfile(OUTPUT_APPUI_FILE):
        os.remove(OUTPUT_APPUI_FILE)
    if os.path.isfile(OUTPUT_MAKEFILE_NETAPPS):
        os.remove(OUTPUT_MAKEFILE_NETAPPS)

def open_output_file():
    global allconfig
    global appmenu
    global mainmenu_ini_temp
    global makefile_netapps
    allconfig = open(OUTPUT_TEMP_FILE, 'w+')
    appmenu = open(OUTPUT_APPUI_FILE, 'w+')
    mainmenu_ini_temp = open(OUTPUT_MAINMENU_INI_TEMP, 'w+')
    makefile_netapps = open(OUTPUT_MAKEFILE_NETAPPS, 'w+')
    make_netapps_makefile_header()
    with open(PATH_MAINMENU_INI, 'r') as fj:
        mainmenu_ini_temp.write(fj.read())


def close_output_file():
    global allconfig
    global appmenu
    global mainmenu_ini_temp
    global makefile_netapps
    allconfig.close()
    appmenu.close()
    mainmenu_ini_temp.close()
    makefile_netapps.close()

def make_config_json(appname,jsonsting):
    global allconfig
    global load_dict_all
    load_dict_all[appname]=jsonsting
    #print(appname,load_dict_all[appname])

def make_appui_entry(jsonsting):
    global appmenu
    global load_dict_apps
    global appslist
    appslist.append(jsonsting['APPUI_ENTRY'])
    load_dict_apps['Netapps']= appslist
    #print(jsonsting['APPUI_ENTRY'])

def make_mainmenu_ini_temp(jsonsting):
    global mainmenu_ini_temp
    if check_str(jsonsting['APPUI_ENTRY']):
        one_line = '{'+jsonsting['APPUI_ENTRY']+', '+jsonsting['MENU_ATTR']+', "'+jsonsting['MENU_TITLE_STR']+'","#ifdef '+jsonsting['MACRO_DEFINE']+'", '+jsonsting['MENU_EXEC']+', '+jsonsting['MENU_CHECK_FUNC']+'}\n'
        #print(one_line)
        mainmenu_ini_temp.write(one_line)


def check_str(entrystring):
    cmd = "grep -w " + '"{\\s*' + entrystring + '" ' + PATH_MAINMENU_INI
    entry = get_by_shell(cmd)
    if '' == entry:
        return 1
    else:
        return 0

def cp_netappsmenu_json():
    if os.path.isfile(OUTPUT_NETAPPSMENU_CONFIG):
        return
    if os.path.isfile(OUTPUT_APPUI_FILE):
        with open(OUTPUT_NETAPPSMENU_CONFIG, 'w+') as netappsmenu_config:
            with open(OUTPUT_APPUI_FILE, 'r') as fj:
                netappsmenu_config.write(fj.read())

def cp_image(pathname):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    menupath = fulldirct + '/menu/' + THEME_NAME + '/image/'
    if os.path.isdir(menupath):
        themepath = THEME_DST_DIR +'image/netapps/'
        if not os.path.exists(themepath):
            os.makedirs(themepath)
        menupath = menupath +'*'
        cmd = "cp -R " + menupath + ' '+ themepath
        get_by_shell(cmd)

def cp_title_image(pathname):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    menupath = fulldirct + '/image/'
    if os.path.isdir(menupath):
        themepath = THEME_DST_DIR +'image/netapps/'
        if not os.path.exists(themepath):
            os.makedirs(themepath)
        menupath = menupath +'*'
        cmd = "cp -R " + menupath + ' '+ themepath
        get_by_shell(cmd)

def cp_widget(pathname):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    menupath = fulldirct + '/menu/' + THEME_NAME + '/widget/'
    if os.path.isdir(menupath):
        themepath = THEME_DST_DIR +'widget/netapps/'
        if not os.path.exists(themepath):
            os.makedirs(themepath)
        menupath = menupath +'*'
        cmd = "cp -R " + menupath + ' '+ themepath
        get_by_shell(cmd)

def cp_language(pathname):
    fulldirct = os.path.join(NETAPPS_PATH, pathname)
    menupath = fulldirct + '/str/'
    lan_path = THEME_DST_DIR+'language/language'
    if not os.path.isfile(lan_path):
        return
    if not os.path.isdir(menupath):
        return
    with open(lan_path, 'r') as f:
        list1 = f.readlines()
        for i in range(0, len(list1)):
            if '#' in list1[i] :
                list1[i] = list1[i].rstrip('\n')
                list1[i] = list1[i].lstrip('#')
            else:
                no = i
        del list1[no]
        for i in range(0, len(list1)):
            xmlpath1 = menupath + list1[i] + '.xml'
            xmlpath2 = THEME_DST_DIR +'language/' + list1[i] + '.xml'
            if not os.path.isfile(xmlpath1):
                continue
            if  not os.path.isfile(xmlpath2):
                continue
            with open(xmlpath2, 'r+') as f2:
                content = f2.read()
                pos = content.rfind("</tran>")
                pos = pos +len("</tran>\n")
                if pos < 0:
                    continue
                position = f2.seek(pos,0)
                with open(xmlpath1, 'r') as f1:
                    for line in f1:
                        if "<tran id=" in line:
                            str_id = str_wrap_by("<tran",">",line)
                            if ''  ==  str_id:
                                continue
                            check_str_id(xmlpath2,str_id,f2,line)
                    f2.write("    </language>\n")

def str_wrap_by(start_str, end, oneline):
    start = oneline.find(start_str)
    if start >= 0:
        start += len(start_str)
        end = oneline.find(end, start)
        if end >= 0:
            return oneline[start:end].strip()
    return ''

def check_str_id(xmlpath2,str_id,f2,line):
    cmd = 'grep \"'+str_id+ '\" ' + xmlpath2
    tran_id = get_by_shell(cmd)
    if '' == tran_id:
        f2.write(line)

def parse_config(parsed_json):
    ret = 0
    menuconfig_id = parsed_json['MENUCONFIG_ID']
    cmd = 'grep -r \"'+menuconfig_id+ '\" ' + PATH_SOLUTION_CONFIG
    br2 = get_by_shell(cmd)
    if '' != br2 and '#' not in br2 and '=y' in br2:
        ret = 1
    if '' == br2:
        ret = 0
        cmd = 'grep -r \"BR2_MOD_APP_NETAPPS\" ' + PATH_SOLUTION_CONFIG
        br3 = get_by_shell(cmd)
        if '' != br3 and '#' not in br3 and '=y' in br3:
            if 'DEFAULT_CLOSE' in parsed_json:
                default_close = parsed_json['DEFAULT_CLOSE']
                if 'yes' == default_close:
                    ret = 0
                if 'no' == default_close:
                    modify_config_switch(menuconfig_id)
                    ret =  1
            else:
                modify_config_switch(menuconfig_id)
                ret = 1
            if 1 == ret:
               modify_subdefines(parsed_json)
    return ret

def check_switch(appname):
    fulldirct = NETAPPS_PATH + appname +'/config.json'
    if os.path.isfile(PATH_SOLUTION_CONFIG) and os.path.isfile(fulldirct):
        try:
            with open(fulldirct, 'r') as fj:
                parsed_json = json.load(fj, object_pairs_hook=OrderedDict)
        except ValueError as e:
            print("can't parse the config.json in ",fulldirct)
            return 0
        else:
            return parse_config(parsed_json)
    return 0

def modify_subdefines(parsed_json):
    if 'SUB_DEFINES' in parsed_json:
        sub_defines = parsed_json['SUB_DEFINES']
        for one in sub_defines:
            sub_menuconfig_id = one['MENUCONFIG_ID']
            cmd = 'grep -r \"'+sub_menuconfig_id+ '\" ' + PATH_SOLUTION_CONFIG
            br2 = get_by_shell(cmd)
            if '' == br2:
                modify_config_switch(sub_menuconfig_id)

def modify_config_switch(menuconfig_id):
    with open(PATH_SOLUTION_CONFIG, 'r+') as f1:
        f1.read()
        line = menuconfig_id + '=y\n'
        #print(line)
        f1.write(line)

def make_netapps_makefile_header():
    global makefile_netapps
    makefile_netapps.write("#This makefile is generated by python script tool. \n")
    makefile_netapps.write("#Please do not edit it by yourself.\n")
    makefile_netapps.write("#Ref: scripts/config_netapps.py. \n\n")


def make_one_app_makefile(appname):
    global makefile_netapps
    fulldirct = NETAPPS_PATH + appname +'/lib/' + ARCH_NAME + '-' + OS_NAME +'/'
    if not os.path.isdir(fulldirct):
        return
    lib = 'LDFLAGS += -L$(GXSRC_PATH)/app/netapps/' + appname +'/lib/$(ARCH)-$(OS)\n'
    include = 'CFLAGS += -I$(GXSRC_PATH)/app/netapps/' + appname +'/include\n'
    makefile_netapps.write(lib)
    makefile_netapps.write(include)
    for i in os.listdir(fulldirct):
        #print(i)
        if '.a' in i:
            i = i.replace('.a','',1)
            if 'lib' in i:
                i = i.replace('lib','',1)
                libname = 'LDFLAGS += -l' + i + '\n'
                #print(libname)
                makefile_netapps.write(libname)


def do_all_netapps():
    global allconfig
    global load_dict_all
    global appmenu
    global load_dict_apps
    for i in os.listdir(NETAPPS_PATH):
        if('protect' == i):
            fulldirct = NETAPPS_PATH + 'protect'
            if not os.path.isdir(fulldirct):
                continue
            for j in os.listdir(fulldirct):
                pathname = i + '/' + j
                get_netapp_config(pathname)
        else:
            get_netapp_config(i)
    json.dump(load_dict_all,allconfig, indent=4)
    json.dump(load_dict_apps,appmenu, indent=4)


remove_ouput_file()
open_output_file()
do_all_netapps()
close_output_file()
cp_netappsmenu_json()
