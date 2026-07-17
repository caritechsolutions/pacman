#!/usr/bin/env python3
import os

#./build <chip> <board> <.config>

build_all_board = 1
change_config = 1

chip_list = [
"gx3201",
"gx3113C",
"gx3211",
"gx6605s",
]

board_dict = {
        "gx3201_list":[
            "generic",
            ],

        "gx3113C_list":[
            "generic",
            ],

        "gx3211_list":[
            "generic",
            ],

        "gx6605s_list":[
            "generic",
            ],
}

config_list = [
"boot",
"debug",
"release",
]

secure_verify = [ "s/^ENABLE_SECURE_VERIFY = ./ENABLE_SECURE_VERIFY = y/g", ]

spiflash = [
"s/^ENABLE_SPIFLASH = ./ENABLE_SPIFLASH = y/g",
"s/^ENABLE_SPINAND = ./ENABLE_SPINAND = n/g",
"s/^ENABLE_NANDFLASH = ./ENABLE_NANDFLASH = n/g",
]

spinand = [
"s/^ENABLE_SPIFLASH = ./ENABLE_SPIFLASH = n/g",
"s/^ENABLE_SPINAND = ./ENABLE_SPINAND = y/g",
"s/^ENABLE_NANDFLASH = ./ENABLE_NANDFLASH = n/g",
]

nandflash = [
"s/^ENABLE_SPIFLASH = ./ENABLE_SPIFLASH = n/g",
"s/^ENABLE_SPINAND = ./ENABLE_SPINAND = n/g",
"s/^ENABLE_NANDFLASH = ./ENABLE_NANDFLASH = y/g",
]

sed_config_dict = {
        'public_list':[
            [ "s/^ENABLE_LOGO = ./ENABLE_LOGO = y/g",],
            ],

        'gx3201_list':[
            spiflash,
            spinand,
            nandflash,
            ],

        'gx3113C_list':[
            secure_verify,
            ],

        'gx3211_list':[
            spiflash,
            spinand,
            nandflash,
            secure_verify,
            ],

        'gx6605s_list':[
            spiflash,
            spinand,
            nandflash,
            secure_verify,
            ],
}

def put_error_log(cmd,sed_list):
    print("build error!")
    print("last cmd :")
    print("\t{0}".format(cmd))
    for s in sed_list:
        print('\tsed -i "{0}" .config'.format(s))

    print("\tmake clean;make\n")

def iferror(config):
    if config == "boot":
        file_type = "boot"
    else:
        file_type = "bin"
    status = os.system("rm output/*.{0}".format(file_type))
    if status != 0:
        return True
    else:
        return False

def make(config,sed_list):
    for s in sed_list:
        os.system('sed -i "{0}" .config'.format(s))

    os.system("make clean;make")
    if iferror(config) == True:
        return False

    return True


def build(chip,board,config):
    cmd = './build {0} {1} {2}'.format(chip,board,config)
    print("run cmd: "+cmd)
    os.system(cmd)
    if iferror(config) == True:
        put_error_log(cmd,[])
        return False

    if change_config == 1:
        os.system('cp .config tmp.config')

        for sub_list in sed_config_dict['public_list']:
            os.system('cp tmp.config .config')
            if make(config,sub_list) == False:
                put_error_log(cmd,sub_list)
                os.system('rm tmp.config')
                return False

        for sub_list in sed_config_dict['{0}_list'.format(chip)]:
            os.system('cp tmp.config .config')
            if make(config,sub_list) == False:
                put_error_log(cmd,sub_list)
                os.system('rm tmp.config')
                return False

        os.system('rm tmp.config')

    return True

def get_board_list(chip):
    board_list = []
    output = os.popen("ls conf/{0}/".format(chip))
    for s in output.readlines():
        s = s.strip()
        board_list.append(s)
    return board_list

def main():
    if build_all_board == 1:
        for chip in chip_list:
            board_dict['{0}_list'.format(chip)] = get_board_list(chip)

    for chip in chip_list:
        for board in board_dict['{0}_list'.format(chip)]:
            for config in config_list:
                status = build(chip,board,config)
                if status == False:
                    return False
    print("build OK!")
    return True

main()

