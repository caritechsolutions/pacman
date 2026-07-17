#!/usr/bin/env python2
# -*- coding: utf-8 -*-

"parse netappsmenu-config.json"

import json
import sys
from collections import OrderedDict
from collections import Iterable
import subprocess
import  re
flag_str = "Netapps"

mainmenu_flag_str1 = "member"
mainmenu_flag_str2 = "APPUI_ENTRY_NETAPPS"
netapps_json = sys.argv[1]+"/netappsmenu-config.json"
mainmenu_json = sys.argv[1]+"/mainmenu-config.json"
mainmenu_json_bak = sys.argv[1]+"/mainmenu-config.json_bak"

t2_mainmenu_netapp_cfile = sys.argv[2]+"/app/app_t2_netapp_ex.c"
t2_mainmenu_netapp_hfile = sys.argv[2]+"/app/include/app_common_2nd_menu.h"
netapps_allconfig = sys.argv[2]+"/output/allconfig.json"
netapp_event_init_cfile = sys.argv[2]+"/app/netapps/app_netapps_common_ex.c"
mainmenu_ini_bak = sys.argv[2]+"/projects/main_menu_temp.ini"

no_protect_app_id_list = {} #example "APPUI_ENTRY_WEATHER":"weather"

def get_by_shell(cmd):
    #print(cmd)
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if '' == err:
        return out.strip()
    else:
        return ''


def get_netapps_info():
    js = {}
    print("parse the netappsmenu-config.json")
    try:
        with open(netapps_json, "rb") as f:
            js = json.load(f, object_pairs_hook=OrderedDict)
    except ValueError as e:
        print("can't parse the netappsmenu-config.json")
        js[flag_str] = None
    except IOError as e:
        print("can't found the netappsmenu-config.json")
        js[flag_str] = None

    return js[flag_str]

def create_netapp_event_init(f, json_dir):
    f.write("void app_netapps_event_init_ex(void)\n{\n")
    if isinstance(json_dir,Iterable):
        with open(netapps_allconfig, "rb") as f2:
            js = json.load(f2, object_pairs_hook=OrderedDict)

        for k in json_dir:
            for i in js:
                app_id = None
                event_name = None
                defined_name = None
                exec_name = None
                menu_title = None
                ui_entry = None
                attr = None
                for j in js[i]:
                    if j.decode("utf-8") == "APP_ID":
                        app_id = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MACRO_DEFINE":
                        defined_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_EXEC":
                        exec_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_TITLE_STR":
                        menu_title = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_EVENT_NAME":
                        event_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_ATTR":
                        attr = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "APPUI_ENTRY":
                        ui_entry = js[i][j.decode("utf-8")]

                if (ui_entry != None)&(defined_name != None):
                    if k == ui_entry:
                        if(event_name != None)&(event_name != "NULL"):
                            f.write("\t#ifdef "+defined_name+"\n")
                            f.write("\textern app_netapp_event_class "+ str(event_name) +";\n")
                            f.write("\tapp_netapp_event_register(&"+str(event_name)+");\n")
                            f.write("\t#endif\n")
    f.write("}\n")

def t2_create_internet_entry_init(f, json_dir,netapps_max_count):
    app_id = None
    defined_name = None
    exec_name = None
    menu_title = None
    ui_entry = None
    attr = None
    check_func = None

    f.write("void _internet_entry_init(void)\n{\n")
    f.write("#if NETAPPS_SUPPORT\n")
    f.write("\tint32_t netapp_enable = 0;\n")
    f.write("#endif\n")
    f.write("\tg_Common2ndEntryCount = 0;\n")
    f.write("\tmemset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));\n")

    if isinstance(json_dir,Iterable):
        with open(netapps_allconfig, "rb") as f2:
            js = json.load(f2, object_pairs_hook=OrderedDict)

        m=0
        for k in json_dir:
            if(m>=netapps_max_count):
                print("Error:Pls Check app_common_2nd_menu.h,netapps count bigger than COMMON_2ND_MAX_COUNT!!!")
                break
            m=m+1
            is_protect_app = False
            for i in js:
                app_id = None
                defined_name = None
                exec_name = None
                menu_title = None
                ui_entry = None
                attr = None
                check_func = None
                for j in js[i]:
                    if j.decode("utf-8") == "APP_ID":
                        app_id = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MACRO_DEFINE":
                        defined_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_EXEC":
                        exec_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_TITLE_STR":
                        menu_title = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "APPUI_ENTRY":
                        ui_entry = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_ATTR":
                        attr = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_CHECK_FUNC":
                        check_func = js[i][j.decode("utf-8")]
                if (app_id != None)&(defined_name != None)&(exec_name != None)&(ui_entry != None):
                    if k == ui_entry:
                        is_protect_app = True
                        f.write("#ifdef "+defined_name+"\n")
                        if (check_func != None)&(check_func != "NULL"):
                            f.write("\textern int "+check_func+"();\n")
                            f.write("\tif("+check_func+"())\n\t{\n")
                        f.write("\tif(app_get_netapps_default_xml(\""+app_id+"\", &netapp_enable) == GXCORE_SUCCESS)\n\t{\n")
                        f.write("\t\tif(netapp_enable == 1)\n\t\t{\n")
                        f.write("\t\t\textern void "+exec_name+"(void);\n")
                        f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].title = \""+menu_title+"\";\n")
                        f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].create = "+exec_name+";\n")
                        if attr != None:
                            f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].attr = "+attr+";\n")
                        f.write("\t\t\tg_Common2ndEntryCount++;\n")
                        f.write("\t\t}\n")
                        f.write("\t}\n")
                        if (check_func != None)&(check_func != "NULL"):
                            f.write("\t}\n")
                        f.write("#endif\n")
                        break

            if is_protect_app == False: #try to found it in main_menu_temp.ini
                cmd = "grep -w " + '"{\\s*' + k + '" ' + mainmenu_ini_bak
                entry = get_by_shell(cmd)
                if '' == entry:
                    continue
                else:
                    m = entry[1:-1].split(',')
                    app_id = no_protect_app_id_list[k]
                    attr = m[1].strip().strip('"')
                    menu_title = m[2].strip().strip('"')
                    defined_name = m[3].strip().strip('"')
                    exec_name = m[4].strip().strip('"')
                    check_func = m[5].strip().strip('"')
                    f.write(defined_name+"\n")
                    if (check_func != None)&(check_func != "NULL"):
                        f.write("\textern int "+check_func+"();\n")
                        f.write("\tif("+check_func+"())\n\t{\n")
                    f.write("\tif(app_get_netapps_default_xml(\""+app_id+"\", &netapp_enable) == GXCORE_SUCCESS)\n\t{\n")
                    f.write("\t\tif(netapp_enable == 1)\n\t\t{\n")
                    f.write("\t\t\textern void "+exec_name+"(void);\n")
                    f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].title = \""+menu_title+"\";\n")
                    f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].create = "+exec_name+";\n")
                    if attr != None:
                        f.write("\t\t\tg_Common2ndEntry[g_Common2ndEntryCount].attr = "+attr+";\n")
                    f.write("\t\t\tg_Common2ndEntryCount++;\n")
                    f.write("\t\t}\n")
                    f.write("\t}\n")
                    if (check_func != None)&(check_func != "NULL"):
                        f.write("\t}\n")
                    f.write("#endif\n")


    f.write("\treturn;\n")
    f.write("}\n")

def t2_create_internet_entry_count_get(f,json_dir,netapps_max_count):
    app_id = None
    defined_name = None
    check_func = None

    f.write("int _internet_entry_count_get(void)\n{\n")
    f.write("\tstatic int ret = -1;\n")
    f.write("#if NETAPPS_SUPPORT\n")
    f.write("\tint netapp_enable = 0;\n\n")
    f.write("#endif\n")
    f.write("\tif(ret >= 0)\n")
    f.write("\t\treturn ret;\n")
    f.write("\tret = 0;\n")

    if isinstance(json_dir,Iterable):
        with open(netapps_allconfig, "rb") as f2:
            js = json.load(f2, object_pairs_hook=OrderedDict)
        m=0
        for k in json_dir:
            if(m>=netapps_max_count):
                print("Error:Pls Check app_common_2nd_menu.h,netapps count bigger than COMMON_2ND_MAX_COUNT!!!")
                break
            m=m+1
            is_protect_app = False
            for i in js:
                app_id = None
                defined_name = None
                check_func = None
                for j in js[i]:
                    if j.decode("utf-8") == "APP_ID":
                        app_id = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MACRO_DEFINE":
                        defined_name = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "APPUI_ENTRY":
                        ui_entry = js[i][j.decode("utf-8")]
                    if j.decode("utf-8") == "MENU_CHECK_FUNC":
                        check_func = js[i][j.decode("utf-8")]
                if (app_id != None)&(defined_name != None)&(ui_entry != None):
                    if k == ui_entry:
                        is_protect_app = True
                        f.write("#ifdef "+defined_name+"\n")
                        if (check_func != "NULL")&(check_func != None):
                            f.write("\textern int "+check_func+"();\n")
                            f.write("\tif("+check_func+"())\n\t{\n")
                        f.write("\tif(app_get_netapps_default_xml(\""+app_id+"\", &netapp_enable) == GXCORE_SUCCESS)\n\t{\n")
                        f.write("\t\tif(netapp_enable == 1)\n\t\t{\n")
                        f.write("\t\t\tret++;\n")
                        f.write("\t\t}\n")
                        f.write("\t}\n")
                        if (check_func != "NULL")&(check_func != None):
                            f.write("\t}\n")
                        f.write("#endif\n")
                        break

            if is_protect_app == False: #try to found it in main_menu_temp.ini
                cmd = "grep -w " + '"{\\s*' + k + '" ' + mainmenu_ini_bak
                entry = get_by_shell(cmd)
                if '' == entry:
                    continue
                else:
                    m = entry[1:-1].split(',')
                    app_id = no_protect_app_id_list[k]
                    defined_name = m[3].strip().strip('"')
                    check_func = m[5].strip().strip('"')
                    f.write(defined_name+"\n")
                    if (check_func != "NULL")&(check_func != None):
                        f.write("\textern int "+check_func+"();\n")
                        f.write("\tif("+check_func+"())\n\t{\n")
                    f.write("\tif(app_get_netapps_default_xml(\""+app_id+"\", &netapp_enable) == GXCORE_SUCCESS)\n\t{\n")
                    f.write("\t\tif(netapp_enable == 1)\n\t\t{\n")
                    f.write("\t\t\tret++;\n")
                    f.write("\t\t}\n")
                    f.write("\t}\n")
                    if (check_func != "NULL")&(check_func != None):
                        f.write("\t}\n")
                    f.write("#endif\n")

    f.write("\treturn ret;\n")
    f.write("}\n")


def modify_mainmenu_json(json_dir):
    with open(mainmenu_json, "rb") as f:
        js = json.load(f, object_pairs_hook=OrderedDict)

    style = js['style']
    if style == "classic":
        for i in js[mainmenu_flag_str1]:
            count = 0
            for j in js[mainmenu_flag_str1][i]:
                if j.decode("utf-8") == mainmenu_flag_str2:
                    netapp_dic = js[mainmenu_flag_str1][i]
                    netapp_dic.pop(count)
                    for k in json_dir:
                        netapp_dic.insert(count, k)
                        count = count+1
                    with open(mainmenu_json_bak, "wb+") as f:
                        json.dump(js, f, indent=4)
                    s2_t2 = 1
 #               return s2_t2
                count = count+1

    s2_t2 = 0                                       #can't find mainmenu_flag_str2, it is t2 menu
    with open(t2_mainmenu_netapp_cfile, "w+") as f:
        f.write("//This .c is generated by python script tool.\n")
        f.write("//Please do not edit it by yourself.\n")
        f.write("//sources: netappsmenu-config.json, allconfig.json\n")
        f.write("//process: app_t2_netapp_ex.c\n\n")

        f.write('#include "app.h"\n')
        f.write('#include "app_main_menu.h"\n')
        f.write('#include "app_common_2nd_menu.h"\n\n')
        f.write("extern status_t app_get_netapps_default_xml(char * name, int32_t *result);\n\n")
        netapps_max_count = 0

        with open(t2_mainmenu_netapp_hfile, "r") as file:
            contents = file.read()
            if contents.find("COMMON_2ND_MAX_COUNT") != -1:
                findLine=re.findall(r"COMMON_2ND_MAX_COUNT (.*?)\n", contents)[0]
                if(findLine):
                    pair = re.compile(r'[(](.*?)[)]', re.S)
                    findCount=re.findall(pair, findLine)[0]
                    if(findCount):
                        netapps_max_count=int(findCount)
                    else:
                        netapps_max_count=50
                else:
                    netapps_max_count=50
            else:
                netapps_max_count=50


        t2_create_internet_entry_init(f, json_dir,netapps_max_count)
        f.write("\n")
        t2_create_internet_entry_count_get(f, json_dir,netapps_max_count)


    return s2_t2
def netapp_event_init_cfile_check(json_dir):
    with open(mainmenu_json, "rb") as f:
        js = json.load(f, object_pairs_hook=OrderedDict)
    with open(netapp_event_init_cfile, "w+") as f2:
        f2.write("//This .c is generated by python  script tool.\n")
        f2.write("//Please do not edit it by yourself.\n")
        f2.write("//sources: netappsmenu-config.json, allconfig.json\n")
        f2.write("//process: app_netapps_common_ex.c\n\n")
        f2.write('#include "app.h"\n')
        f2.write('#include "app_main_menu.h"\n')
        f2.write('#include "app_netapps_init.h"\n')
        create_netapp_event_init(f2, json_dir)


json_dir = get_netapps_info()
modify_mainmenu_json(json_dir)
netapp_event_init_cfile_check(json_dir)

