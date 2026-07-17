#!/usr/bin/env python2
# -*- coding:utf-8 -*-

import json
from collections import OrderedDict
import subprocess
import sys,os

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
MAINTITLE = ''
Default_SubMenu = 0
ClassificationItemSize = 0
Sub_Entry_ID = ''
OBJ_SUPPORT = ''
OBJ_Func_Create = 'gxapp_exapi_submenu_create'
OBJ_Func_Check  = 'gxapp_exapi_submenu_check'
OBJ_Func_Keypress = 'gxapp_exapi_submenu_keypress'
OBJ_Func_Content = 'gxapp_exapi_submenu_content'
MainMenuSetting = ''
MainMenuOperationLadder = ''
f = ''


with open(CONFIG, 'r') as fj:
    parsed_json = json.load(fj, object_pairs_hook=OrderedDict)


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


def get_default_focus_widget(key):
    cmd = 'grep -w ' + '"{\\s*' + key + '" ' + ENTRIES
    widget = get_by_shell(cmd)
    if '' == widget:
        return 0
    else:
        m = widget[1:-1].split(',')
        return m[1].strip().strip('"')


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


def check_content_func(name):
    cmd = 'find app -name "*.c" | xargs grep "^\\s*char\\s*\\*\\s*' + name + '(void)"'
    if '' == get_by_shell(cmd):
        return 0
    else:
        return 1


def get_setting_item_config(item):
    global ID, ID_Pos, SUBTITLE, ClassificationItemSize, Sub_Entry_ID, MainMenuSetting, EXTERN
    name, deps, keypress, content, check = get_one_setting(item)

    if '' == name:
        print('\033[1;31m' + 'Please check [%s] in enum AppUiEntriesID' % name + '\033[0m')
        sys.exit(1)

    if 1 == check_support(deps):
        if name != '':
            multilang = get_multi_language(name) == '' and ('"' + name + '"') or get_multi_language(name)
            SUBTITLE = SUBTITLE + '\t' + multilang + ',\n'
            ID = ID + '\t' + item.strip().replace('APPUI_SETTING_', 'MM_') + ',\n'
            ID_Pos = ID_Pos + 1
            ClassificationItemSize = ClassificationItemSize + 1

        if keypress != "NULL":
            if 0 == check_keypress_func(keypress):
                print('\033[1;31m' + 'Please check ui setting keypress func [%s] for [%s] in main_menu.ini' % (
                    keypress, name) + '\033[0m')
                sys.exit(1)
            else:
                if OBJ_SUPPORT != '':
                    keypress = OBJ_Func_Keypress
                EXTERN = EXTERN + 'extern int ' + keypress + '(int key);\n'

        if content != "NULL":
            if 0 == check_content_func(content):
                print('\033[1;31m' + 'Please check ui setting content func [%s] for [%s] in main_menu.ini' % (
                    content, name) + '\033[0m')
                sys.exit(1)
            else:
                if OBJ_SUPPORT != '':
                    content = OBJ_Func_Content
                EXTERN = EXTERN + 'extern char *' + content + '(void);\n'

        if check != "NULL":
            if 0 == check_check_func(check):
                print('\033[1;31m' + 'Please check ui setting check func [%s] for [%s] in main_menu.ini' % (
                    check, name) + '\033[0m')
                sys.exit(1)
            else:
                if OBJ_SUPPORT != '':
                    check = OBJ_Func_Check
                EXTERN = EXTERN + 'extern int ' + check + '(void);\n'

        if OBJ_SUPPORT != '':
            Sub_Entry_ID = item.strip()
        else:
            Sub_Entry_ID = '0'
        MainMenuSetting = MainMenuSetting + '\t{MAIN_MENU_SETTING,\t.u.item_s = {' + keypress + ', ' + content + ', ' + check + ', '+Sub_Entry_ID + '}},\n'

def get_entry_item_config(item):
    global SUBTITLE, ID, ID_Pos, EXTERN, ClassificationItemSize, Sub_Entry_ID, MainMenuSetting
    attr, name, deps, create, check = get_one_entry(item)
    if '' == name:
        print('\033[1;31m' + 'Please check [%s] in enum AppUiEntriesID' % item + '\033[0m')
        sys.exit(1)

    if 1 == check_support(deps):
        if name != '':
            multilang = get_multi_language(name) == '' and ('"' + name + '"') or get_multi_language(name)
            SUBTITLE = SUBTITLE + '\t' + multilang + ',\n'
            ID = ID + '\t' + item.strip().replace('APPUI_ENTRY_', 'MM_') + ',\n'
            ID_Pos = ID_Pos + 1
            ClassificationItemSize = ClassificationItemSize + 1

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
        MainMenuSetting = MainMenuSetting + '\t{MAIN_MENU_ENTRY, \t.u.item_e = {' + attr + ', ' + create + ', ' + check + ', '+Sub_Entry_ID+'}},\n'

def get_class_strings(value):
    global SUBTITLE, EXTERN, ID, ID_Array, ClassificationItemSize
    ClassificationItemSize = 0

    for e in value:
        if e.find("APPUI_SETTING") != -1:
            get_setting_item_config(e)
        elif e.find("APPUI_ENTRY") != -1:
            get_entry_item_config(e)
        else:
            print('\033[1;31m' + '%s is invalid please check in main_menu.ini' % e + '\033[0m')
            sys.exit(1)

    ID_Array = ID_Array + '%d' % ClassificationItemSize + ', '
    ID = ID + '\n'
    SUBTITLE = SUBTITLE + '\n'


def get_main_menu_item_offset():
    if 'mainmenu_item' in parsed_json:
        if parsed_json['mainmenu_item'] != '':
            offset = parsed_json['mainmenu_item']['offset']
            icon_interval = parsed_json['mainmenu_item']['interval']
            level = parsed_json['mainmenu_item']['level']
            visual = parsed_json['mainmenu_item']['visual']
            arrow = parsed_json['mainmenu_item']['arrow']
            return offset, icon_interval, level, visual, arrow
    return '', '', '', '', ''


def get_main_menu_setting():
    global MAINTITLE, ID, ID_Pos, SUBTITLE, Default_SubMenu

    for i, key in enumerate(parsed_json['member']):
        # print(key)
        if key == parsed_json['default']:
            Default_SubMenu = i

        MAINTITLE = MAINTITLE + '\t' + (
                    get_multi_language(key) == '' and ('"' + key + '"') or get_multi_language(key)) + ',\n'
        ID = ID + '\t// ' + key + ' Index: %d' % ID_Pos + '\n'
        SUBTITLE = SUBTITLE + '\t// ' + key + ' Index: %d' % ID_Pos + '\n'
        get_class_strings(parsed_json['member'][key])


def get_ladder(ladder):
    global MainMenuOperationLadder

    if 'focus widget' not in ladder:
        print('\033[1;31m[Operation ladder] doesn\'t exist focus widget\033[0m\n')
        sys.exit(1)

    focus_widget = get_default_focus_widget(ladder['focus widget'])
    if '' == focus_widget:
        print('\033[1;31m' + '[%s] doesn\'t exist corresponding focus widget' % ladder['default focus'] + '\033[0m\n')
        sys.exit(1)
    else:
        MainMenuOperationLadder = MainMenuOperationLadder + '\t{.focus_widget = ' + '"' + focus_widget + '", .next_key_num = '

    if 'enter next key' in ladder:
        key = ladder['enter next key']
        if isinstance(key, list) and 0 != len(key) and len(key) < 5:
            MainMenuOperationLadder = MainMenuOperationLadder + str(len(key)) + ', .next_ladder_key = {'
            for e in key:
                MainMenuOperationLadder = MainMenuOperationLadder + e + ', '
        else:
            print('\033[1;31m[Operation ladder] format is error\033[0m\n')
            sys.exit(1)
        MainMenuOperationLadder = MainMenuOperationLadder[0:-2]
    else:
        MainMenuOperationLadder = MainMenuOperationLadder + '1, .next_ladder_key = {STBK_OK'

    if 'enter back key' in ladder:
        key = ladder['enter back key']
        if isinstance(key, list) and 0 != len(key) and len(key) < 5:
            MainMenuOperationLadder = MainMenuOperationLadder + '}, .back_key_num = ' + str(len(key)) + ', .back_ladder_key = { '
            for e in key:
                MainMenuOperationLadder = MainMenuOperationLadder + e + ', '
        else:
            print('\033[1;31m[Operation ladder] format is error\033[0m\n')
            sys.exit(1)
        MainMenuOperationLadder = MainMenuOperationLadder[0:-2]
    else:
        MainMenuOperationLadder = MainMenuOperationLadder + '}, .back_key_num = 2, back_ladder_key = {STBK_EXIT, STBK_MENU'

    MainMenuOperationLadder = MainMenuOperationLadder + '}},\n'

    if 'next ladder' in ladder:
        get_ladder(ladder['next ladder'])
    else:
        return


def get_main_menu_operation_ladder():
    global MainMenuOperationLadder

    if "operation ladder" not in parsed_json:
        MainMenuOperationLadder = '\t{.focus_widget = "wnd_main_menu_item_listview", .next_key_num = 1, ' \
                                  '.next_ladder_key = {STBK_OK}, .back_key_num = 2, .back_ladder_key = {STBK_MENU, ' \
                                  'STBK_EXIT}}, '
        return
    get_ladder(parsed_json['operation ladder'])


def get_main_menu_source_file_head():
    global f
    f = open(OUTVAR, 'w+')

    f.write("//This .c is generated by python script tool.\n")
    f.write("//Please do not edit it by yourself.\n")
    f.write("//sources: main_menu.ini mainmenu-config.json\n")
    f.write("//process: scripts/parse_mainmenu.py\n\n")

    f.write('#include "app.h"\n')
    f.write('#include "app_main_menu.h"\n\n')
    f.write('#include "app_main_menu_exapi.h"\n')
    f.write('#include "app_main_menu_style.h"\n\n')


def get_extern_function_statement():
    global f
    str_sub_func_create = 'void ' + OBJ_Func_Create+'(void)\n'
    str_sub_func_check  = 'int ' + OBJ_Func_Check+'(void)\n'
    str_sub_func_keypress  = 'int ' + OBJ_Func_Keypress+'(int key)\n'
    str_sub_func_content  = 'char* ' + OBJ_Func_Content+'(void)\n'
    if OBJ_SUPPORT != '':
        f.write(str_sub_func_create)
        f.write('{\n')
        f.write('  int id = gxapp_exapi_get_entry_uiid();\n')
        f.write('  gxapp_exapi_mainmenu_sub_create(id);\n')
        f.write('}\n\n')
        f.write(str_sub_func_check)
        f.write('{\n')
        f.write('  int id = gxapp_exapi_get_proc_uiid();\n')
        f.write('  return gxapp_exapi_mainmenu_sub_check(id);\n')
        f.write('}\n\n')
        f.write(str_sub_func_keypress)
        f.write('{\n')
        f.write('  int id = gxapp_exapi_get_proc_uiid();\n')
        f.write('  return gxapp_exapi_mainmenu_keypress(id,key);\n')
        f.write('}\n\n')
        f.write(str_sub_func_content)
        f.write('{\n')
        f.write('  int id = gxapp_exapi_get_proc_uiid();\n')
        f.write('  return gxapp_exapi_mainmenu_get_content(id);\n')
        f.write('}\n\n')
    else:
        f.write("%s\n" % EXTERN)


def get_main_menu_item_config():
    global f
    step = 0;
    offset, icon_interval, level, visual, arrow = get_main_menu_item_offset()
    if offset != '':
        step = icon_interval / level
    else:
        offset, icon_interval, level, visual, arrow, step = (0, 0, 0, 0, 0, 0)

    f.write('int mainmenu_item_offset_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % offset)

    f.write('int mainmenu_item_length_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % icon_interval)

    f.write('int mainmenu_item_move_level_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % level)

    f.write('int mainmenu_item_listview_visual_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % visual)

    f.write('int mainmenu_item_arrow_exist_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % arrow)

    f.write('int mainmenu_item_move_step_get(void)\n{\n')
    f.write('\treturn %d;\n}\n\n' % step)


def get_main_menu_global_var():
    global f
    f.write("static MainMenuStyle s_MainMenuStyle = {\n")

    if "Last" == parsed_json['default']:
        f.write("\t.default_last_sub = true,\n")
    else:
        f.write("\t.default_last_sub = false,\n")

    f.write('\t.default_sub = %d,\n' % Default_SubMenu)

    if 'mainmenu_item' in parsed_json:
        if parsed_json['mainmenu_item'] != '':
            f.write('\t.item_move = true\n')
        else:
            f.write('\t.item_move = false\n')
    else:
        f.write('\t.item_move = false\n')

    f.write("};\n\n")

    if "classification change" in parsed_json:
        if type(parsed_json['classification change']) == list and len(parsed_json['classification change']) == 2:
            f.write('static int s_ClassificationChange[2] = {%s, %s};\n\n'
                    % (parsed_json['classification change'][0], parsed_json['classification change'][1]))
        else:
            f.write('static int s_ClassificationChange[2] = {STBK_LEFT, STBK_RIGHT};\n\n')
    else:
        f.write('static int s_ClassificationChange[2] = {STBK_LEFT, STBK_RIGHT};\n\n')

    f.write('static char *s_MainMenuMainTitle[] =\n{\n')
    f.write("%s\n" % MAINTITLE[:-2])
    f.write('};\n\n')

    f.write('static OperationLadder s_OperationLadder[] =\n{\n')
    f.write("%s\n" % MainMenuOperationLadder[:-2])
    f.write('};\n\n')

    f.write('static unsigned int s_MainMenuSubSize[] = {%s};\n' % ID_Array[:-2])

    f.write("typedef enum\n{\n")
    f.write("%s\n" % ID[:-2])
    f.write("} MainMenuItemID;\n\n")

    f.write('static char *s_MainMenuSubTitle[] =\n{\n')
    f.write("%s\n" % SUBTITLE[:-2])
    f.write('};\n\n')

    f.write("static MainMenuSetting s_MainMenuSetting[] =\n{\n")
    f.write("%s\n" % MainMenuSetting[:-2])
    f.write("};\n\n")


def get_data_function_definition():
    global f
    style = parsed_json['style']
    f.write('MainMenuStyle* MainMenuStyle_DataGet(void)\n{\n')
    f.write('\treturn &s_MainMenuStyle;\n}\n\n')

    f.write('int* ClassificationChange_DataGet(int sel)\n{\n')
    f.write('\treturn &s_ClassificationChange[sel];\n}\n\n')

    f.write('int ClassificationChange_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_ClassificationChange) / sizeof(int);\n}\n\n')

    f.write('char** MainMenuMainTitle_DataGet(int sel)\n{\n')
    f.write('\treturn &s_MainMenuMainTitle[sel];\n}\n\n')

    f.write('int MainMenuMainTitle_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_MainMenuMainTitle) / sizeof(char *);\n}\n\n')


    f.write('OperationLadder* OperationLadder_DataGet(int sel)\n{\n')
    f.write('\treturn &s_OperationLadder[sel];\n}\n\n')

    f.write('int OperationLadder_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_OperationLadder) / sizeof(OperationLadder);\n}\n\n')

    f.write('unsigned int* MainMenuSubSize_DataGet(int sel)\n{\n')
    f.write('\treturn &s_MainMenuSubSize[sel];\n}\n\n')

    f.write('int MainMenuSubSize_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_MainMenuSubSize) / sizeof(unsigned int);\n}\n\n')

    f.write('char** MainMenuSubTitle_DataGet(int sel)\n{\n')
    f.write('\treturn &s_MainMenuSubTitle[sel];\n}\n\n')

    f.write('int MainMenuSubTitle_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_MainMenuSubTitle) / sizeof(char *);\n}\n\n')

    f.write('MainMenuSetting* MainMenuSetting_DataGet(int sel)\n{\n')
    f.write('\treturn &s_MainMenuSetting[sel];\n}\n\n')

    f.write('int MainMenuSetting_SizeGet(void)\n{\n')
    f.write('\treturn sizeof(s_MainMenuSetting) / sizeof(MainMenuSetting);\n}\n\n')

    f.write('void MainMenuStyleItem_Init(void)\n{\n')
    f.write('\textern MainmenuStyleOps %sops;\n' % style)
    f.write('\tapp_mainmenu_style_register(&%sops);\n' % style)
    f.write('\treturn;\n}\n\n')



def get_main_menu_source_file_tail():
    global f
    f.close()
    if os.path.isfile(ENTRIES):
        os.remove(ENTRIES)

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
get_main_menu_setting()
get_main_menu_operation_ladder()

get_main_menu_source_file_head()
get_extern_function_statement()
get_main_menu_item_config()
get_main_menu_global_var()
get_data_function_definition()
get_main_menu_source_file_tail()
