#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import subprocess
import platform
from version import version

def run_cmd(full_cmd):
    print(full_cmd)
    subprocess.call(full_cmd, shell=True)

dir_name = 'GX3215B_signature_cli_' + version

os = platform.system()

if os == 'Windows':
    pass
elif os == 'Linux':
    print('Linux platform')
    run_cmd("pyinstaller  --clean  -F -w        \
                      --icon=ui/logo.ico        \
                      -n gx3215b_signature       \
                      --add-data=\"bin:bin\"    \
                      --additional-hooks-dir=.  \
                      gx3215_signature.py")

    dir_name += "-CentOS"
    run_cmd("rm -rf " + dir_name)
    run_cmd("mkdir " + dir_name)
    run_cmd("cp bin/config.txt " + dir_name)
    run_cmd("cp bin/help.txt " + dir_name)
    run_cmd("chmod +x " + dir_name + "/help.txt")
    run_cmd("cp -r dist/* " + dir_name)
    run_cmd("cp -r config " + dir_name)
    run_cmd("cp -r common_config " + dir_name)
    run_cmd("rm -rf __pycache__ build dist gx3215_signature.spec")
    run_cmd("tar czf %s.tar.gz %s"%(dir_name, dir_name))
else:
    print(os+' platform not support now')
    pass
