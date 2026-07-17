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
import sys, os
reload(sys)
sys.setdefaultencoding('utf-8')

SOLUTION_PATH = sys.argv[1]
PATH_NETAPPS_ALLCONFIG = SOLUTION_PATH + '/output/allconfig.json'
PATH_SOLUTION_CONFIG = SOLUTION_PATH + '/.config'
OUT_APP_CONFIG_H = sys.argv[1] + '/app/include/app_config.h'


f1 = ''

def get_by_shell(cmd):
    #print(cmd)
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if '' == err:
        return out.strip()
    else:
        return ''


def check_switch(menuconfig_id):
    if os.path.isfile(PATH_SOLUTION_CONFIG):
        cmd = 'grep -r \"'+menuconfig_id+ '=\" ' + PATH_SOLUTION_CONFIG
        br2 = get_by_shell(cmd)
        if '' != br2 and '#' not in br2 and '=y' in br2:
            return 1
    return 0

def get_one_subdefines(one_app):
    global f1
    if 'SUB_DEFINES' in one_app:
        sub_defines = one_app['SUB_DEFINES']
        for one in sub_defines:
            sub_macro_define = one['MACRO_DEFINE']
            sub_menuconfig_id = one['MENUCONFIG_ID']
            if check_switch(sub_menuconfig_id):
                line = '#define ' + sub_macro_define + '\n'
                #print(line)
                f1.write(line)

def get_netapps_macro_define():
    global f1
    with open(OUT_APP_CONFIG_H, 'r+') as f1:
        f1.read()
        try:
            with open(PATH_NETAPPS_ALLCONFIG, "rb") as f2:
                allconfig = json.load(f2, object_pairs_hook=OrderedDict)
        except ValueError as e:
            print("can't parse the allconfig.json in ",PATH_NETAPPS_ALLCONFIG)
        else:
            for i in allconfig:
                macro_define = allconfig[i]['MACRO_DEFINE']
                menuconfig_id = allconfig[i]['MENUCONFIG_ID']
                if check_switch(menuconfig_id):
                    line = '#define ' + macro_define + '\n'
                    f1.write(line)
                    get_one_subdefines(allconfig[i])

get_netapps_macro_define()
