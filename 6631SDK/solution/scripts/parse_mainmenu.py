#!/usr/bin/env python2
# -*- coding:utf-8 -*-

import json
import subprocess
import sys,os

reload(sys)
sys.setdefaultencoding('utf-8')

if len(sys.argv) != 3:
    print("Usage: python run_script.py <main_menu_json_path> <mainmenu_config_json_path>")
    sys.exit(1)

main_menu_json_path = sys.argv[1]
main_menu_out_path = sys.argv[2]
clssic_pythom_flle_path = sys.argv[2] + '/scripts/parse_calssic_mainmenu.py'
grid_pythom_flle_path = sys.argv[2] + '/scripts/parse_grid_mainmenu.py'

with open(main_menu_json_path, 'r') as json_file:
    json_data = json_file.read()

data = json.loads(json_data)

style = data['style']

if style == "classic":
    command_classic = [
        clssic_pythom_flle_path,
        main_menu_json_path,
        main_menu_out_path
    ]

    try:
        subprocess.check_call(command_classic)
    except subprocess.CalledProcessError as e:
        print("An error occurred while executing the command: {}".format(e))
elif style == 'grid':
    command_grid = [
        grid_pythom_flle_path,
        main_menu_json_path,
        main_menu_out_path
    ]

    try:
        subprocess.check_call(command_grid)
    except subprocess.CalledProcessError as e:
        print("An error occurred while executing the command: {}".format(e))
else:
    print("Unknown style: {}".format(style))
