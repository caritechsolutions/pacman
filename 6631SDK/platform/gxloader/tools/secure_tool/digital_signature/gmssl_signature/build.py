#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import subprocess
import platform
from version import version

def generate_version_file():
    version_file_str = """
# UTF-8
#
# For more details about fixed file info 'ffi' see:
# http://msdn.microsoft.com/en-us/library/ms646997.aspx
VSVersionInfo(
  ffi=FixedFileInfo(
    # filevers and prodvers should be always a tuple with four items: (1, 2, 3, 4)
    # Set not needed items to zero 0.
    filevers=(3, 0, 0, 0),
    prodvers=(3, 0, 0, 0),
    # Contains a bitmask that specifies the valid bits 'flags'r
    mask=0x3f,
    # Contains a bitmask that specifies the Boolean attributes of the file.
    flags=0x0,
    # The operating system for which this file was designed.
    # 0x4 - NT and there is no need to change it.
    OS=0x4,
    # The general type of file.
    # 0x1 - the file is an application.
    fileType=0x1,
    # The function of the file.
    # 0x0 - the function is not defined for this fileType
    subtype=0x0,
    # Creation date and time stamp.
    date=(0, 0)
    ),
  kids=[
    StringFileInfo(
      [
      StringTable(
        u'040904e4',
        [StringStruct(u'CompanyName', u'杭州杭州国芯微股份有限公司'),
        StringStruct(u'FileDescription', u'sirius or canopus国密签名工具'),
        StringStruct(u'FileVersion', u'%s'),
        StringStruct(u'InternalName', u'www.nationalchip.com'),
        StringStruct(u'LegalCopyright', u'Copyright (C) 2001-2023 nationalchip All Rights Reserved'),
        StringStruct(u'OriginalFilename', u'signature.exe'),
        StringStruct(u'ProductName', u'sirius or canopus国密签名工具'),
        StringStruct(u'ProductVersion', u'%s')])
      ]),
    VarFileInfo([VarStruct(u'Translation', [1033, 1252])])
  ]
)""" % (version,version)
    with open('version.txt', 'w', encoding='utf-8') as f:
        f.write(version_file_str)

def run_cmd(full_cmd):
    print(full_cmd)
    subprocess.call(full_cmd, shell=True)

dir_name = 'signature_' + version
generate_version_file()
os = platform.system()


if os == 'Windows':
    run_cmd("pyinstaller -w --clean -F             \
                        --icon ui/logo.ico         \
                        -n signature               \
                        --add-data=\"bin;bin\"     \
                        --add-data=\"ui;ui\"       \
                        --additional-hooks-dir=.   \
                        --version-file=version.txt \
                        gui.py")

    dir_name += "-windows"
    run_cmd("del /F /S /Q " + dir_name)
    run_cmd("mkdir " + dir_name)
    run_cmd("mkdir " + dir_name + "\\key")
    run_cmd("mkdir " + dir_name + "\\config")
    run_cmd("mkdir " + dir_name + "\\common_config")
    run_cmd("mkdir " + dir_name + "\\config")
    run_cmd("copy dist " + dir_name)
    run_cmd("copy key " + dir_name + "\\key")
    run_cmd("copy bin\\help.txt " + dir_name)
    run_cmd("copy config " + dir_name+ "\\config")
    run_cmd("copy common_config " + dir_name + "\\common_config")
    run_cmd("del /F /S /Q __pycache__ build dist program.spec signature.spec version.txt")
    run_cmd("del /F /S /Q " + dir_name + "\\config\\config.json")
    run_cmd("rd /S /Q __pycache__ build dist")

elif os == 'Linux':
    print('Linux platform')
    run_cmd("pyinstaller --add-data=\"ui:ui\" --clean  -F -w            \
                      --icon=ui/logo.ico        \
                      -n signature              \
                      --add-data=\"bin:bin\"    \
                      --add-data=\"ui:ui\"      \
                      --additional-hooks-dir=.  \
                      gui.py")

    dir_name += "-ubuntu"
    run_cmd("rm -rf " + dir_name)
    run_cmd("mkdir " + dir_name)
    run_cmd("cp -r dist/* " + dir_name)
    run_cmd("cp -r key " + dir_name)
    run_cmd("cp bin/help.txt " + dir_name)
    run_cmd("cp -r config " + dir_name)
    run_cmd("cp -r common_config " + dir_name)
    run_cmd("rm -rf __pycache__ build dist program.spec signature.spec " + dir_name + "/config/config.json")
    run_cmd("rm -rf __pycache__ build dist otp_manager.spec version.txt")
    run_cmd("tar czf %s.tar.gz %s"%(dir_name, dir_name))
else:
    print(os+' platform not support now')
    pass
