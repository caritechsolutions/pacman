#!/usr/bin/env python2
# -*- coding:utf-8 -*-

import json
import sys,os
import subprocess

reload(sys)
sys.setdefaultencoding('utf-8')

CONFIG = sys.argv[1]
OUTVAR = sys.argv[2] + '/app/app_main_menu_ex.c'
MACROS = sys.argv[2] + '/app/include/app_config.h'
MULTIL = sys.argv[2] + '/app/include/app_str_id.h'
ENTRIES = sys.argv[2] + '/projects/main_menu_temp.ini'

EXTERN = ''
SUBTITLE = ''
ID = ''
ID_Pos = 0
ID_Array = ''
Sub_Entry_ID = ''
OBJ_SUPPORT = ''
OBJ_Func_Create = 'gxapp_exapi_submenu_create'
OBJ_Func_Check  = 'gxapp_exapi_submenu_check'
MainMenuSetting = ''

with open(CONFIG, 'r') as json_file:
    json_data = json_file.read()
# 解析 JSON 数据
data = json.loads(json_data)

# 提取 style 字符串
style = data['style']

def get_by_shell(cmd):
    #print(cmd)
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if '' == err:
        return out.strip()
    else:
        return ''


def check_support(macro):
    if '' == macro:
        return 1
    else:
        ret = 0
        key = macro.split(' ')[1]
        if '#ifdef' in macro:
            cmd = "grep -w " + '"\\s*#define\\s*' + key + '" ' + MACROS + " | awk -F ' ' '{print $2}'"
            ret = len(get_by_shell(cmd))
        elif '#if' in macro.strip(''):
            cmd = "grep -w " + '"' + key + '" ' + MACROS + " | awk -F ' ' '{print $3}'"
            ret = int(get_by_shell(cmd))
        if ret > 0:
            return 1
        else:
            return 0


def get_multi_language(name):
    if "STR_ID_" not in name:
        cmd = 'grep "\\"' + name + '\\"" ' + MULTIL + " | awk -F ' ' '{print $2}'"
        return get_by_shell(cmd)
    else:
        return name


def get_one_entry(key):
    cmd = "grep -w " + '"{\\s*' + key + '" ' + ENTRIES
    entry = get_by_shell(cmd)
    if '' == entry:
        return '', '', '', '', ''
    else:
        m = entry[1:-1].split(',')
        attr = m[1]
        name = m[2]
        deps = m[3]
        create = m[4]
        check = m[5]
        return attr.strip().strip('"'), name.strip().strip('"'), deps.strip().strip('"'), create.strip(), check.strip()


def get_one_setting(key):
    cmd = "grep -w " + '"{\\s*' + key + '" ' + ENTRIES
    entry = get_by_shell(cmd)
    if '' == entry:
        return '', '', '', '', '', ''
    else:
        m = entry[1:-1].split(',')
        name = m[1]
        deps = m[2]
        keypress = m[3]
        content = m[4]
        check = m[5]
        return name.strip().strip('"'), deps.strip().strip('"'), keypress.strip(), content.strip(), check.strip()


def check_create_func(name):
    cmd = 'find app -name "*.c" | xargs grep "^\\s*void\\s*' + name + '(void)"'
    if '' == get_by_shell(cmd):
        return 0
    else:
        return 1


def check_check_func(name):
    cmd = 'find app -name "*.c" | xargs grep "^\\s*int\\s*' + name + '(void)"'
    #print(cmd)
    if '' == get_by_shell(cmd):
        return 0
    else:
        return 1


def check_keypress_func(name):
    cmd = 'find app -name "*.c" | xargs grep "^\\s*int\\s*' + name + '(\\s*int"'
    if '' == get_by_shell(cmd):
        return 0
    else:
        return 1

def get_entry_item_config(item):
    global SUBTITLE, ID, ID_Pos, EXTERN, Sub_Entry_ID, MainMenuSetting
    attr, name, deps, create, check = get_one_entry(item)
    if '' == name:
        print('\033[1;31m' + 'Please check [%s] in enum AppUiEntriesID' % item + '\033[0m')
        return 1

    if 1 == check_support(deps):
        if name != '':
            multilang = get_multi_language(name) == '' and ('"' + name + '"') or get_multi_language(name)
            SUBTITLE = SUBTITLE + '\t' + multilang + ',\n'
            ID = ID + '\t' + item.strip().replace('APPUI_ENTRY_', 'MM_') + ',\n'

        if create != "NULL":
            if 0 == check_create_func(create):
                print('\033[1;31m' + 'Please check ui create func [%s] for [%s] in main_menu.ini' % (
                        create, name) + '\033[0m')
                sys.exit(1)
            if OBJ_SUPPORT != '':
                create = OBJ_Func_Create
            EXTERN = EXTERN + 'extern void ' + create + '(void);\n'

        if check != "NULL":
            if 0 == check_check_func(check):
                print('\033[1;31m' + 'Please check ui check func [%s] for [%s] in main_menu.ini' % (
                    check, name) + '\033[0m')
                sys.exit(1)
            if OBJ_SUPPORT != '':
                check = OBJ_Func_Check
            EXTERN = EXTERN + 'extern int ' + check + '(void);\n'

        if OBJ_SUPPORT != '':
            Sub_Entry_ID = item.strip()
        else:
            Sub_Entry_ID = '0'
        MainMenuSetting = MainMenuSetting + '\t{' + attr + ', ' + create + ', ' + check + ', ' + Sub_Entry_ID + '},\n'
        return 0
    else:
         print('\033[1;31m' + 'Please check ui check func [%s] for [%s] in main_menu.ini' % (
                    check, name) + '\033[0m')
         return 1

def get_main_menu_ex_file_config():
    global OBJ_SUPPORT
    cmd = '(cat ' + MACROS + '| grep "APP_MAIN_MENU_OBJ_SUPPORT" | grep "1" | wc -l)'
    ret = int(get_by_shell(cmd))
    if ret > 0:
       print("main menu obj support")
       OBJ_SUPPORT = 'Obj Support'
    else:
       print("main menu obj not support")

get_main_menu_ex_file_config()
# 检查 style 是否为 "grid"
if style == "grid":
    for index, member in enumerate(data['member']):
        entry = member['entry']
        if 1 == get_entry_item_config(entry):
            print("Error: Main menu item Error.")
            sys.exit(1)

    # C 文件内容
    c_file_content = []
    c_file_content.append('#include "app.h"')
    c_file_content.append('#include "app_main_menu.h"')
    c_file_content.append('#include "app_main_menu_exapi.h"')
    c_file_content.append('#include "app_main_menu_style.h"\n\n')

    str_sub_func_create = 'void ' + OBJ_Func_Create+'(void)\n{'
    str_sub_func_check  = 'int ' + OBJ_Func_Check+'(void)\n{'
    if OBJ_SUPPORT != '':
        c_file_content.append(str_sub_func_create)
        #c_file_content.append('{\n')
        c_file_content.append('  int id = gxapp_exapi_get_entry_uiid();\n')
        c_file_content.append('  gxapp_exapi_mainmenu_sub_create(id);\n')
        c_file_content.append('}\n\n')
        c_file_content.append(str_sub_func_check)
        #c_file_content.append('{\n')
        c_file_content.append('  int id = gxapp_exapi_get_proc_uiid();\n')
        c_file_content.append('  return gxapp_exapi_mainmenu_sub_check(id);\n')
        c_file_content.append('}\n\n')
    else:
        c_file_content.append("%s\n" % EXTERN)

    c_file_content.append("\n\n")

    c_file_content.append("typedef enum {")
    c_file_content.append("%s" % ID[:-2])
    c_file_content.append("} MenuEntry;\n\n")

    c_file_content.append("static char*  s_NewMenuSubTitle[] = {")
    c_file_content.append("%s" % SUBTITLE[:-2])
    c_file_content.append("};\n\n")

    c_file_content.append("static ItemEntry s_NewMenuItem[] = {")
    c_file_content.append("%s" % MainMenuSetting[:-2])
    c_file_content.append("};\n\n")



    #entry_map = {member['entry']: index for index, member in enumerate(data['member'])}

    member_count = len(data['member'])
    c_file_content.append("#define MEMBER_COUNT {}\n\n".format(member_count))

    default_value = str(data['default'])

    if isinstance(default_value, str) and default_value.isdigit():
        default_value = int(default_value)
    else:
        default_value = 0xff

    if isinstance(default_value, int) and 0 <= default_value < member_count:
        c_file_content.append("#define DEFAULT_VALUE {}\n\n".format(default_value))
    else:
        c_file_content.append("#define DEFAULT_VALUE 0xFF\n\n")

    # 生成数组
    c_file_content.append("MainmenuStyleItem style_items[] = {")
    for member in data['member']:
        focus_image = member['image']['focus']
        unfocus_image = member['image']['unfcous']
        state = member.get('state', 'show')
        state_value = "true" if state == "show" else "false"
        if 'keypress' in member:
            for key in ['up', 'down', 'left', 'right']:
                value = member['keypress'].get(key)
                if value:
                    number = int(value.split('_')[-1])  # 获取 position_marker_X 中的 X
                    if key == 'up':
                        up_value = number - 1
                    elif key == 'down':
                        down_value = number - 1
                    elif key == 'left':
                        left_value = number - 1
                    elif key == 'right':
                        right_value = number - 1
                else:
                    if key == 'up':
                        up_value = 0xff
                    elif key == 'down':
                        down_value = 0xff
                    elif key == 'left':
                        left_value = 0xff
                    elif key == 'right':
                        right_value = 0xff
        c_file_content.append("    {{\"{}\", \"{}\", {}, {}, {}, {}, {}}},".format(
            focus_image, unfocus_image, state_value,
            up_value, down_value, left_value, right_value
        ))
    c_file_content.append("};\n")

    c_file_content.append('int MainMenuSubSize_SizeGet(void)\n{')
    c_file_content.append('\treturn MEMBER_COUNT;\n}\n\n')
    c_file_content.append('int MainMenuDefaultFocus_Get(void)\n{')
    c_file_content.append('\treturn DEFAULT_VALUE;\n}\n\n')
    c_file_content.append('char* MainMenuSubTitle_Get(int sel)\n{')
    c_file_content.append('\tif( sel > (MEMBER_COUNT-1))')
    c_file_content.append('\t\treturn NULL;')
    c_file_content.append('\treturn s_NewMenuSubTitle[sel];\n}\n\n')

    c_file_content.append('ItemEntry* MainMenuEntryItem_Get(int sel)\n{')
    c_file_content.append('\tif( sel > (MEMBER_COUNT-1))')
    c_file_content.append('\t\treturn NULL;')
    c_file_content.append('\treturn &s_NewMenuItem[sel];\n}\n\n')

    c_file_content.append('MainmenuStyleItem* MainMenuStyleItem_Get(int sel)\n{')
    c_file_content.append('\tif( sel > (MEMBER_COUNT-1))')
    c_file_content.append('\t\treturn NULL;')
    c_file_content.append('\treturn &style_items[sel];\n}\n\n')
    c_file_content.append('void MainMenuStyleItem_Init(void)\n{')
    c_file_content.append('\textern MainmenuStyleOps %sops;' % style)
    c_file_content.append('\tapp_mainmenu_style_register(&%sops);' % style)
    c_file_content.append('\treturn;\n}\n\n')

    # 写入 C 文件
    with open(OUTVAR, "w") as c_file:
        c_file.write("/* Auto-generated file */\n\n")
        c_file.write("\n".join(c_file_content))
else:
    print("Style is not 'grid'. No C file generated.")

