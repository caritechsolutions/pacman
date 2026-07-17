#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import subprocess
from optparse import OptionParser
from common import *
#from log import logger
from crc32 import crc32
import struct
from gmtool import GmToolBase
from gxpy.gxsecure.secure_manager.gxsecure_manager import GxSecureManager

class Info(object):
    self_dir = get_self_dir()
    CHIP =  "sirius"
    SECURE_LEVEL = None
    SIGN_ALGO = None
    SIGN_FILE = None
    BOOTLOADER = None
    # generic config
    MAGIC_NUM=4
    CRC_NUM=4
    ND_KEY_NUM=256
    SIGN_NUM=256
    MARK_ID_NUM=4
    LOADER_STAGE1_START = 32  #sirius
    LOADER_STAGE2_START = 0x4000 #sirius
    LOADER_STAGE1_LEN = 0x4000 #sirius
    STAGE1_DATA_NUM = None
    BIN_OR_BOOT = None
    REG_CFG_FILE = None
    COMMON_CONFIG_DIR = "common_config"
    MARKET_ID_FILE = self_dir + os.sep + COMMON_CONFIG_DIR + os.sep + "market_id.txt"

    # config for gx3211/gx6605s
    REG_CFG_FILE_NUM = None
    # config for sirius
    LOADER_STAGE1_DECRYPT_KEY_START=16340  #0x3FD4
    LOADER_STAGE1_DECRYPT_KEY_LENGTH=16
    LOADER_STAGE1_DECRYPT_FLAG_START=16356 #0x3FE4
    LOADER_STAGE1_DECRYPT_FLAG_LENGTH=4

class GmTool(GmToolBase):
    PRIVATE_KEY_FILE=None
    PUBLIC_KEY_FILE=None
    PRIVATE_KEY_BIN_FILE=None
    PUBLIC_KEY_BIN_FILE_X=None
    PUBLIC_KEY_BIN_FILE_Y=None
    PUBLIC_KEY_BIN_FILE_X_START=1
    PUBLIC_KEY_BIN_FILE_Y_START=33
    PUBLIC_KEY_BIN_LENGTH=32
    PUBLIC_KEY_BIN_START_FLAGS="pub"
    PUBLIC_KEY_BIN_END_FLAGS="ASN1 OID"
    PRIVATE_KEY_BIN_START_FLAGS="priv"
    PRIVATE_KEY_BIN_END_FLAGS="pub"

    SM2_IDA_VALUE   = None
    SM2_ENTLA_VALUE = None
    SM2_IDA_RESERVE_VALUE = None

    RESERVE1_NUM=28
    RESERVE2_NUM=32
    RESERVE3_NUM=192
    RESERVE4_NUM=192
    RESERVE5_NUM=20
    RESERVE6_NUM=192
    RESERVE7_NUM=28
    STAGE2_LENGTH_NUM=4
    SM2_IDA_NUM=158
    SM2_ENTLA_NUM=2
    SM2_COORDINATE_NUM=32
    SM2_SIGN_NUM=32
    ENTLA = None
    password="123456"

    def __init__(self, key_dir="key", tmp_dir="tmp"):
        super(GmTool, self).__init__()

        if key_dir.endswith(os.sep):
            self.SM2_KEY_DIR = key_dir[:-1]
        else:
            self.SM2_KEY_DIR = key_dir

        if tmp_dir.endswith(os.sep):
            self.tmpDir = tmp_dir[:-1]
        else:
            self.tmpDir = tmp_dir

        self_dir = get_self_dir()
        self.SM2_CONFIG_DIR  = self_dir + os.sep + "config"
        self.tmpDir = tmp_dir + os.sep
        self.SM2_FIXED_VALUE = self.tmpDir + "fixed_to_sign.bin"

    def generate_private_key(self, name):
        DeleteFile(name)
        CreateParentDir(name)
        self.RunSubProcess([self.gmssl, "genpkey", "-algorithm", "EC", "-pkeyopt", "ec_paramgen_curve:sm2p256v1", "-pkeyopt", "ec_param_enc:named_curve", "-out", name])

        CheckGenFileOK(name)

        if False:
            print("make public x&y bin")
            content = self.RunSubProcess([self.gmssl, "pkey", "-in", name, "-text", "-noout"])
            content = TrimLines(content)
            content = TakePartStr(content, self.PRIVATE_KEY_BIN_START_FLAGS, self.PUBLIC_KEY_BIN_END_FLAGS)
            parts = content.split(self.PRIVATE_KEY_BIN_END_FLAGS, 1)
            pri_content =parts[0]
            pub_content =parts[1]

            pri_content = pri_content.replace(':', ' ')
            pub_content = pub_content.replace(':', ' ')

            print("")
            print("pri: " + pri_content)
            print("pub: " + pub_content)

            SaveStr(name + "_private_x.txt", pri_content)
            SaveStr(name + "_public_x.txt", pub_content)

        #SaveData(name + "_private.bin", hex2byte(pri_content))
        #CheckGenFileOK(name + "_private.bin")

    def generate_private_bin(self, private_key, name):
        CheckFileFail(private_key)
        DeleteFile(name)

        content = self.RunSubProcess([self.gmssl, "pkey", "-in", private_key, "-text", "-noout", \
                "-passin", "pass:" + self.password])
        content = TrimLines(content)
        content = TakePartStr(content, self.PRIVATE_KEY_BIN_START_FLAGS, self.PUBLIC_KEY_BIN_END_FLAGS)
        parts = content.split(self.PRIVATE_KEY_BIN_END_FLAGS, 1)
        pri_content =parts[0]
        pub_content =parts[1]

        pri_content = pri_content.replace(':', ' ')
        pub_content = pub_content.replace(':', ' ')

        #print("pri: " + pri_content)
        #print("pub: " + pub_content)

        SaveData(name, hex2byte(pri_content))
        CheckGenFileOK(name)

    def generate_private_pem(self, pri_bin, name):
        CheckFileFail(pri_bin)
        DeleteFile(name)

        content = ReadPartBytesOfFile(pri_bin)
        SaveStr(self.tmpDir + "private_key_hex.txt", byte2hex(content))

        self.RunSubProcess([self.gmssl, "sm2", "-inform", "text", "-in", self.tmpDir + "private_key_hex.txt", \
                "-out", name, "-passout", "pass:" + self.password])

        CheckGenFileOK(name)
        DeleteFile(self.tmpDir + "private_key_hex.txt")

    def generate_private_pem2(self, pri_bin, name):
        CheckFileFail(pri_bin)
        DeleteFile(name)
        content = ReadPartBytesOfFile(pri_bin)
        pri_key_pem, pub_key_pem = GxSecureManager.gm_bin2pem(content)
        SaveStr(name, pri_key_pem)
        CheckGenFileOK(name)

    def generate_public_bin2(self, private_key, name):
        CheckFileFail(private_key)
        DeleteFile(name)
        with open(private_key, 'r') as f:
            content = f.read().strip()
        pri_key_bin, pub_key_bin = GxSecureManager.gm_pem2bin(content)
        SaveData(name, pub_key_bin)
        CheckGenFileOK(name)

    def generate_public_key(self, private_key, name):
        if not os.path.exists(private_key):
            msg = "%s private key not exist, please generate private key first."%private_key
            raise UsrError(msg)

        DeleteFile(name)
        self.RunSubProcess([self.gmssl, "pkey", "-pubout", "-in", private_key, "-out", name, \
                "-passin", "pass:" + self.password])
        CheckGenFileOK(name)

    def generate_public_bin(self, private_key, name):
        if not os.path.exists(private_key):
            msg = "%s private key not exist, please generate private key first."%private_key
            raise UsrError(msg)

        DeleteFile(name)

        content = self.RunSubProcess([self.gmssl, "pkey", "-in", private_key, "-text_pub", "-noout", \
                "-passin", "pass:" + self.password])
        content = TrimLines(content)
        pub_content = TakePartStr(content, self.PUBLIC_KEY_BIN_START_FLAGS, self.PUBLIC_KEY_BIN_END_FLAGS)
        pub_content = pub_content.replace(':', ' ')

        #print("pub: " + pub_content)
        data = hex2byte(pub_content)
        SaveData(name, data[1:])
        CheckGenFileOK(name)

        if False:
            part = TakePartBytes(data, self.PUBLIC_KEY_BIN_FILE_X_START, self.PUBLIC_KEY_BIN_LENGTH)
            SaveData(self.PUBLIC_KEY_BIN_FILE_X, part)
            CheckGenFileOK(self.PUBLIC_KEY_BIN_FILE_X)

            part = TakePartBytes(data, self.PUBLIC_KEY_BIN_FILE_Y_START, self.PUBLIC_KEY_BIN_LENGTH)
            SaveData(self.PUBLIC_KEY_BIN_FILE_Y, part)
            CheckGenFileOK(self.PUBLIC_KEY_BIN_FILE_Y)


    def sm2(self, pri_key, input):
        CheckFileFail(pri_key)
        CheckFileFail(input)
        TEMP_FILE1 = self.tmpDir + "TEMP_FILE1"
        DeleteFile(TEMP_FILE1)
        self.RunSubProcess([self.gmssl, "pkeyutl", "-sign", "-asn1parse", "-pkeyopt", "ec_scheme:sm2", "-inkey", \
                pri_key, "-in", input, "-out", TEMP_FILE1, "-passin", "pass:" + self.password])
        CheckFileFail(TEMP_FILE1)
        fo = open(TEMP_FILE1, "r")
        fo.readline()
        line_r = fo.readline().split(':')[-1].strip()
        line_s = fo.readline().split(':')[-1].strip()
        fo.close()

        pad_r_len = 64 - len(line_r)
        pad_s_len = 64 - len(line_s)
        for _ in range(pad_r_len):
            line_r = '0' + line_r
        for _ in range(pad_s_len):
            line_s = '0' + line_s

        data_r = hex2byte(line_r)
        data_s = hex2byte(line_s)

        DeleteFile(TEMP_FILE1)
        return data_r, data_s

    def sm3(self, files):
        """
        files is list type
        """
        TEMP_FILE2 = self.tmpDir + "TEMP_FILE2"
        DeleteFile(TEMP_FILE2)
        fw = open(TEMP_FILE2, "wb")

        max_len = 0x100000
        for i in range(len(files)):
            # print(files[i])
            CheckFileFail(files[i])
            fr = open(files[i], "rb")
            while(True):
                part = fr.read(max_len)
                if len(part) == 0:
                    break
                fw.write(part)
            fr.close()

        fw.flush()
        fw.close()

        CheckFileFail(TEMP_FILE2)

        content = self.RunSubProcess([self.gmssl, "sm3", "-r", TEMP_FILE2])
        hash = TrimLines(content)
        hash = hash.split()[0]

        DeleteFile(TEMP_FILE2)
        return hex2byte(hash)

    def encrypt(self, input, output, algo, mode, key, iv=None):
        if algo == 'sm4':
            if mode == 'ecb':
                self.RunSubProcess(\
                        [self.gmssl, "sms4-ecb", "-in", input, '-out', output, '-K', key, '-nopad', '-nosalt'])
            elif mode == 'cbc':
                self.RunSubProcess(\
                        [self.gmssl, "sms4-cbc", "-in", input, '-out', output, '-K', key, '-nopad', '-nosalt', '-iv', iv])
            else:
                print("error algo mode")
        elif algo == 'aes128':
            if mode == 'ecb':
                self.RunSubProcess(\
                        [self.gmssl, "enc", '-aes-128-ecb', '-in', input, '-out', output, '-K', key, '-nopad', '-nosalt'])
            elif mode == 'cbc':
                self.RunSubProcess(\
                        [self.gmssl, "enc", '-aes-128-cbc', '-in', input, '-out', output, '-K', key, '-nopad', '-nosalt', '-iv', iv])
            else:
                print("error algo mode")
        else:
            print("error algo")

    def create_ec_param(self):
        content = ReadFileContent(self.SM2_CONFIG_DIR + os.sep + "IDA.txt")
        ida_data = hex2byte(content)
        self.SM2_IDA_VALUE = ida_data
        ida_bytes_len = len(ida_data)
        ida_reserve_bytes_len = self.SM2_IDA_NUM - ida_bytes_len
        ida_bits_len = ida_bytes_len * 8
        ida_bits_len = "%04x"%ida_bits_len

        self.SM2_ENTLA_VALUE = hex2byte(ida_bits_len)
        all  = self.SM2_ENTLA_VALUE
        all += self.SM2_IDA_VALUE
        all += hex2byte(ReadFileContent(self.SM2_CONFIG_DIR + os.sep + "a.txt"))
        all += hex2byte(ReadFileContent(self.SM2_CONFIG_DIR + os.sep + "b.txt"))
        all += hex2byte(ReadFileContent(self.SM2_CONFIG_DIR + os.sep + "Gx.txt"))
        all += hex2byte(ReadFileContent(self.SM2_CONFIG_DIR + os.sep + "Gy.txt"))

        self.SM2_IDA_RESERVE_VALUE = bytes(ida_reserve_bytes_len)
        SaveData(self.SM2_FIXED_VALUE, all)
        CheckFileFail(self.SM2_FIXED_VALUE)

    def delete_ec_param(self):
        DeleteFile(self.SM2_FIXED_VALUE)

    def sign_for_other_file(self, input, name, key_name="blk1_pri.bin"):
        TEMP_FILE4 = self.tmpDir + "TEMP_FILE4"
        TEMP_FILE5 = self.tmpDir + "TEMP_FILE5"

        DeleteFile(TEMP_FILE4)
        DeleteFile(TEMP_FILE5)

        KEY1_PRI_BIN = self.SM2_KEY_DIR + os.sep + key_name
        PRIVATE_KEY_FILE = self.tmpDir + "blkn_pri.pem"
        KEY_PUBLIC_BIN = self.tmpDir + "blkn_pub.bin"
        key_public_bin_trim = self.tmpDir + "blkn_pub.bin.trim"

        self.generate_private_pem(KEY1_PRI_BIN, PRIVATE_KEY_FILE)

        self.generate_public_bin(PRIVATE_KEY_FILE, KEY_PUBLIC_BIN)
        tmp_buffer = ReadPartBytesOfFile(KEY_PUBLIC_BIN, 0)
        SaveData(key_public_bin_trim, tmp_buffer)

        CheckFileFail(PRIVATE_KEY_FILE)
        CheckFileFail(KEY_PUBLIC_BIN)

        self.create_ec_param()
        SM2_ZA_VALUE = self.sm3([self.SM2_FIXED_VALUE, key_public_bin_trim])
        SaveData(TEMP_FILE4, SM2_ZA_VALUE)
        CheckFileFail(TEMP_FILE4)
        tmp_hash = self.sm3([TEMP_FILE4, input])
        SaveData(TEMP_FILE5, tmp_hash)
        CheckFileFail(TEMP_FILE5)
        data_r, data_s = self.sm2(PRIVATE_KEY_FILE, TEMP_FILE5)
        DeleteFile(PRIVATE_KEY_FILE)

        SaveData(name, data_r + data_s)
        DeleteFile(TEMP_FILE4)
        DeleteFile(TEMP_FILE5)
        DeleteFile(KEY_PUBLIC_BIN)
        DeleteFile(key_public_bin_trim)
        self.delete_ec_param()

        CheckFileFail(name)

    def sign_other_file(self, input, name, key_name="blk1_pri.bin"):

        KEY1_PRI_BIN = self.SM2_KEY_DIR + os.sep + key_name
        PRIVATE_KEY_FILE = self.tmpDir + "blkn_pri.pem"

        self.generate_private_pem2(KEY1_PRI_BIN, PRIVATE_KEY_FILE)
        CheckFileFail(PRIVATE_KEY_FILE)
        with open(PRIVATE_KEY_FILE, 'rb') as f:
            private_rsa_root_key = f.read().decode().strip()

        GxSecureManager.gm_other_sign(input, name, private_rsa_root_key)
        CheckFileFail(name)
        DeleteFile(PRIVATE_KEY_FILE)

    def sign_for_bootloader_file(self, input, secure_level, flash_type='nor'):
        return_data = None

        SM2_ZA_VALUE = self.tmpDir + "za.bin"

        KEY1_PRI_BIN = self.SM2_KEY_DIR + os.sep + "blk0_pri.bin"
        KEY2_PRI_BIN = self.SM2_KEY_DIR + os.sep + "blk1_pri.bin"

        TEMP_FILE10 = self.tmpDir + "TEMP_FILE10"
        TEMP_FILE11 = self.tmpDir + "TEMP_FILE11"
        TEMP_FILE12 = self.tmpDir + "TEMP_FILE12"
        TEMP_FILE13 = self.tmpDir + "TEMP_FILE13"
        TEMP_FILE20 = self.tmpDir + "TEMP_FILE20"
        TEMP_FILE21 = self.tmpDir + "TEMP_FILE21"
        TEMP_FILE22 = self.tmpDir + "TEMP_FILE22"
        TEMP_FILE23 = self.tmpDir + "TEMP_FILE23"
        TEMP_FILE30 = self.tmpDir + "TEMP_FILE30"
        TEMP_FILE31 = self.tmpDir + "TEMP_FILE31"

        PRIVATE_KEY_FILE = self.tmpDir + "blk0_pri.pem"
        PRIVATE_2NDKEY_FILE = self.tmpDir + "blk1_pri.pem"

        first_public_bin = self.tmpDir + "blk0_pub.bin"
        first_public_bin_trim = self.tmpDir + "blk0_pub.bin.trim"
        second_public_bin = self.tmpDir + "blk1_pub.bin"
        second_public_bin_trim = self.tmpDir + "blk1_pub.bin.trim"

        CheckFileFail(input)

        self.create_ec_param()
        content = ReadFileContent(Info.MARKET_ID_FILE)
        market_id = hex2byte(content)
        market_id_num = len(market_id)
        stage1_data_num = Info.LOADER_STAGE1_LEN - Info.MAGIC_NUM - self.RESERVE1_NUM - self.SM2_ENTLA_NUM - \
            self.SM2_IDA_NUM - self.RESERVE2_NUM -self.SM2_COORDINATE_NUM  -self.SM2_COORDINATE_NUM  \
            - market_id_num - self.SM2_SIGN_NUM -self.SM2_SIGN_NUM - self.RESERVE3_NUM -self.SM2_SIGN_NUM \
            -self.SM2_SIGN_NUM -self.RESERVE4_NUM - Info.LOADER_STAGE1_DECRYPT_KEY_LENGTH \
            -Info.LOADER_STAGE1_DECRYPT_FLAG_LENGTH - self.RESERVE5_NUM - Info.CRC_NUM

        if secure_level == 1:
            if Info.CHIP == "sirius":
                self.generate_private_pem(KEY1_PRI_BIN, PRIVATE_KEY_FILE)
                self.generate_public_bin(PRIVATE_KEY_FILE, first_public_bin)
                CheckFileFail(first_public_bin)
                tmp_buffer = ReadPartBytesOfFile(first_public_bin, 0)
                SaveData(first_public_bin_trim, tmp_buffer)

                magic_num = ReadPartBytesOfFile(input, 0, Info.MAGIC_NUM)
                reserve1 = ReadPartBytesOfFile(input, Info.MAGIC_NUM, self.RESERVE1_NUM)
                loader_stage1 = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_START, stage1_data_num)
                loader_stage1_decrypt_key = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_DECRYPT_KEY_START, \
                				Info.LOADER_STAGE1_DECRYPT_KEY_LENGTH)
                loader_stage1_decrypt_flag = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_DECRYPT_FLAG_START, \
                				Info.LOADER_STAGE1_DECRYPT_FLAG_LENGTH)
                KEY2_PUBLIC_X = bytes(self.SM2_COORDINATE_NUM)
                KEY2_PUBLIC_Y = bytes(self.SM2_COORDINATE_NUM)
                KEY2_PUBLIC_SIGN_R = bytes(self.SM2_SIGN_NUM)
                KEY2_PUBLIC_SIGN_S = bytes(self.SM2_SIGN_NUM)
                reserve2 = bytes(self.RESERVE2_NUM)
                reserve3 = bytes(self.RESERVE3_NUM)
                reserve4 = bytes(self.RESERVE4_NUM)
                reserve5 = bytes(self.RESERVE5_NUM)

                tmp_data = self.sm3([self.SM2_FIXED_VALUE, first_public_bin_trim])
                SaveData(SM2_ZA_VALUE, tmp_data)
                CheckFileFail(SM2_ZA_VALUE)

                stage1_plain = bytes()
                stage1_plain += loader_stage1
                stage1_plain += self.SM2_ENTLA_VALUE
                stage1_plain += self.SM2_IDA_VALUE
                stage1_plain += self.SM2_IDA_RESERVE_VALUE
                stage1_plain += reserve2
                stage1_plain += KEY2_PUBLIC_X
                stage1_plain += KEY2_PUBLIC_Y
                stage1_plain += market_id
                stage1_plain += KEY2_PUBLIC_SIGN_R
                stage1_plain += KEY2_PUBLIC_SIGN_S
                stage1_plain += reserve3
                SaveData(TEMP_FILE10, stage1_plain)
                CheckFileFail(TEMP_FILE10)
                stage1_plain_hash = self.sm3([SM2_ZA_VALUE, TEMP_FILE10])
                SaveData(TEMP_FILE11, stage1_plain_hash)
                CheckFileFail(TEMP_FILE11)
                STAGE1_SIGN_R, STAGE1_SIGN_S = self.sm2(PRIVATE_KEY_FILE, TEMP_FILE11)
                DeleteFile(TEMP_FILE10)
                DeleteFile(TEMP_FILE11)
                DeleteFile(SM2_ZA_VALUE)

                output_data  = magic_num
                output_data += reserve1
                output_data += loader_stage1
                output_data += self.SM2_ENTLA_VALUE
                output_data += self.SM2_IDA_VALUE
                output_data += self.SM2_IDA_RESERVE_VALUE
                output_data += reserve2
                output_data += KEY2_PUBLIC_X
                output_data += KEY2_PUBLIC_Y
                output_data += market_id
                output_data += KEY2_PUBLIC_SIGN_R
                output_data += KEY2_PUBLIC_SIGN_S
                output_data += reserve3
                output_data += STAGE1_SIGN_R
                output_data += STAGE1_SIGN_S
                output_data += reserve4
                output_data += loader_stage1_decrypt_key
                output_data += loader_stage1_decrypt_flag
                output_data += reserve5
                crc_value = crc32(output_data[Info.LOADER_STAGE1_START:])
                crc_data = struct.pack('I', crc_value)
                output_data += crc_data

                loader_stage2_start = Info.LOADER_STAGE2_START
                if flash_type == "nand":
                    tmp_data = bytes()
                    for _ in range(5):
                        tmp_data += output_data
                    output_data = tmp_data

                    loader_stage2_start = Info.LOADER_STAGE1_LEN * 5

                reserve7 = ReadPartBytesOfFile(input, loader_stage2_start, self.RESERVE7_NUM)
                LOADER_STAGE2_FACT_START = loader_stage2_start + self.RESERVE7_NUM + self.STAGE2_LENGTH_NUM
                loader_stage2 = ReadPartBytesOfFile(input, LOADER_STAGE2_FACT_START)

                # stage2_bin
                tmp_data = self.sm3([self.SM2_FIXED_VALUE, first_public_bin_trim])
                SaveData(SM2_ZA_VALUE, tmp_data)
                CheckFileFail(SM2_ZA_VALUE)
                stage2_plain = loader_stage2
                SaveData(TEMP_FILE12, stage2_plain)
                CheckFileFail(TEMP_FILE12)
                stage2_plain_hash = self.sm3([SM2_ZA_VALUE, TEMP_FILE12])
                SaveData(TEMP_FILE13, stage2_plain_hash)
                CheckFileFail(TEMP_FILE13)
                STAGE2_SIGN_R, STAGE2_SIGN_S = self.sm2(PRIVATE_KEY_FILE, TEMP_FILE13)

                output_data += reserve7
                output_data += struct.pack('I', len(loader_stage2) + len(STAGE2_SIGN_R) + len(STAGE2_SIGN_S))
                output_data += loader_stage2
                output_data += STAGE2_SIGN_R
                output_data += STAGE2_SIGN_S

                DeleteFile(SM2_ZA_VALUE)
                DeleteFile(TEMP_FILE12)
                DeleteFile(TEMP_FILE13)

                return_data = output_data

                """
                output_path = output_dir + os.sep + TakeBaseName(input) + "-level1" + TakeNameExt(input)
                SaveData(output_path, output_data)
                CheckGenFileOK(output_path)

                if False and Info.BIN_OR_BOOT != "BOOT":
                    output_path = output_dir + os.sep + "market_id.bin"
                    SaveData(output_path, market_id)
                    CheckGenFileOK(output_path)

                    output_path = output_dir + os.sep + "loader_level1_stage1_code.bin"
                    SaveData(output_path, stage1_plain)
                    CheckGenFileOK(output_path)

                    output_path = output_dir + os.sep + "loader_level1_stage2_code.bin"
                    SaveData(output_path, stage2_plain)
                    CheckGenFileOK(output_path)
                """
            else:
                print("NOT support chip %s"%Info.CHIP)

        elif secure_level == 2:
            if Info.CHIP == "sirius":
                self.generate_private_pem(KEY1_PRI_BIN, PRIVATE_KEY_FILE)
                self.generate_private_pem(KEY2_PRI_BIN, PRIVATE_2NDKEY_FILE)

                self.generate_public_bin(PRIVATE_KEY_FILE, first_public_bin)
                CheckFileFail(first_public_bin)
                tmp_buffer = ReadPartBytesOfFile(first_public_bin, 0)
                SaveData(first_public_bin_trim, tmp_buffer)
                CheckFileFail(first_public_bin_trim)

                self.generate_public_bin(PRIVATE_2NDKEY_FILE, second_public_bin)
                CheckFileFail(second_public_bin)
                tmp_buffer = ReadPartBytesOfFile(second_public_bin, 0)
                SaveData(second_public_bin_trim, tmp_buffer)
                CheckFileFail(second_public_bin_trim)

                magic_num = ReadPartBytesOfFile(input, 0, Info.MAGIC_NUM)
                reserve1 = ReadPartBytesOfFile(input, Info.MAGIC_NUM, self.RESERVE1_NUM)
                loader_stage1 = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_START, stage1_data_num)
                loader_stage1_decrypt_key = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_DECRYPT_KEY_START, \
                				Info.LOADER_STAGE1_DECRYPT_KEY_LENGTH)
                loader_stage1_decrypt_flag = ReadPartBytesOfFile(input, Info.LOADER_STAGE1_DECRYPT_FLAG_START, \
                				Info.LOADER_STAGE1_DECRYPT_FLAG_LENGTH)

                reserve2 = bytes(self.RESERVE2_NUM)
                reserve3 = bytes(self.RESERVE3_NUM)
                reserve4 = bytes(self.RESERVE4_NUM)
                reserve5 = bytes(self.RESERVE5_NUM)

                tmp_data = self.sm3([self.SM2_FIXED_VALUE, first_public_bin_trim])
                SaveData(SM2_ZA_VALUE, tmp_data)
                CheckFileFail(SM2_ZA_VALUE)
                key2_plain  = ReadPartBytesOfFile(second_public_bin_trim)
                key2_plain += market_id
                SaveData(TEMP_FILE20, key2_plain)
                CheckFileFail(TEMP_FILE20)
                key2_plain_hash = self.sm3([SM2_ZA_VALUE, TEMP_FILE20])
                SaveData(TEMP_FILE21, key2_plain_hash)
                CheckFileFail(TEMP_FILE21)
                KEY2_PUBLIC_SIGN_R, KEY2_PUBLIC_SIGN_S = self.sm2(PRIVATE_KEY_FILE, TEMP_FILE21)
                DeleteFile(TEMP_FILE20)
                DeleteFile(TEMP_FILE21)
                DeleteFile(SM2_ZA_VALUE)

                tmp_data = self.sm3([self.SM2_FIXED_VALUE, second_public_bin_trim])
                SaveData(SM2_ZA_VALUE, tmp_data)
                CheckFileFail(SM2_ZA_VALUE)
                stage1_plain = loader_stage1
                SaveData(TEMP_FILE22, stage1_plain)
                CheckFileFail(TEMP_FILE22)
                stage1_plain_hash = self.sm3([SM2_ZA_VALUE, TEMP_FILE22])
                SaveData(TEMP_FILE23, stage1_plain_hash)
                CheckFileFail(TEMP_FILE23)
                STAGE1_SIGN_R, STAGE1_SIGN_S = self.sm2(PRIVATE_2NDKEY_FILE, TEMP_FILE23)
                DeleteFile(TEMP_FILE22)
                DeleteFile(TEMP_FILE23)
                DeleteFile(SM2_ZA_VALUE)

                output_data = magic_num
                output_data += reserve1
                output_data += loader_stage1
                output_data += self.SM2_ENTLA_VALUE
                output_data += self.SM2_IDA_VALUE
                output_data += self.SM2_IDA_RESERVE_VALUE
                output_data += reserve2
                output_data += ReadPartBytesOfFile(second_public_bin_trim)
                output_data += market_id
                output_data += KEY2_PUBLIC_SIGN_R
                output_data += KEY2_PUBLIC_SIGN_S
                output_data += reserve3
                output_data += STAGE1_SIGN_R
                output_data += STAGE1_SIGN_S
                output_data += reserve4
                output_data += loader_stage1_decrypt_key
                output_data += loader_stage1_decrypt_flag
                output_data += reserve5
                crc_value = crc32(output_data[Info.LOADER_STAGE1_START:])
                crc_data = struct.pack('I', crc_value)
                output_data += crc_data

                loader_stage2_start = Info.LOADER_STAGE2_START
                if flash_type == "nand":
                    tmp_data = bytes()
                    for _ in range(5):
                        tmp_data += output_data
                    output_data = tmp_data

                    loader_stage2_start = Info.LOADER_STAGE1_LEN * 5

                reserve7 = ReadPartBytesOfFile(input, loader_stage2_start, self.RESERVE7_NUM)
                LOADER_STAGE2_FACT_START = loader_stage2_start  + self.RESERVE7_NUM + self.STAGE2_LENGTH_NUM
                loader_stage2 = ReadPartBytesOfFile(input, LOADER_STAGE2_FACT_START)

                # stage2_bin
                tmp_data = self.sm3([self.SM2_FIXED_VALUE, second_public_bin_trim])
                SaveData(SM2_ZA_VALUE, tmp_data)
                CheckFileFail(SM2_ZA_VALUE)
                stage2_plain = loader_stage2
                SaveData(TEMP_FILE30, stage2_plain)
                CheckFileFail(TEMP_FILE30)
                stage2_plain_hash = self.sm3([SM2_ZA_VALUE, TEMP_FILE30])
                SaveData(TEMP_FILE31, stage2_plain_hash)
                CheckFileFail(TEMP_FILE31)
                STAGE2_SIGN_R, STAGE2_SIGN_S = self.sm2(PRIVATE_2NDKEY_FILE, TEMP_FILE31)
                DeleteFile(SM2_ZA_VALUE)
                DeleteFile(TEMP_FILE30)
                DeleteFile(TEMP_FILE31)

                output_data += reserve7
                output_data += struct.pack('I', len(loader_stage2) + len(STAGE2_SIGN_R) + len(STAGE2_SIGN_S))
                output_data += loader_stage2
                output_data += STAGE2_SIGN_R
                output_data += STAGE2_SIGN_S

                return_data = output_data
                """
                output_path = output_dir + os.sep + TakeBaseName(input) + "-level2" + \
                                TakeNameExt(input)
                SaveData(output_path, output_data)
                CheckGenFileOK(output_path)

                if False and Info.BIN_OR_BOOT != "BOOT":
                    output_path = output_dir + os.sep + "market_id.bin"
                    SaveData(output_path, market_id)
                    CheckGenFileOK(output_path)

                    output_path = output_dir +os.sep + "loader_level2_stage1_code.bin"
                    SaveData(output_path, stage1_plain)
                    CheckGenFileOK(output_path)

                    output_path = output_dir +os.sep + "loader_level2_stage2_code.bin"
                    SaveData(output_path, stage2_plain)
                    CheckGenFileOK(output_path)
                """

            else:
                print("NOT support chip %s"%Info.CHIP)
        else:
            print("NOT support secure level %d"%secure_level)

        self.delete_ec_param()

        DeleteFile(first_public_bin)
        DeleteFile(first_public_bin_trim)
        DeleteFile(second_public_bin)
        DeleteFile(second_public_bin_trim)
        DeleteFile(PRIVATE_KEY_FILE)
        DeleteFile(PRIVATE_2NDKEY_FILE)

        return return_data

    def sign_bootloader_file(self, chip_type, input, flash_type='nand'):
        return_data = None

        KEY1_PRI_BIN = self.SM2_KEY_DIR + os.sep + "blk0_pri.bin"
        KEY2_PRI_BIN = self.SM2_KEY_DIR + os.sep + "blk1_pri.bin"

        TEMP_FILE10 = self.tmpDir + "tmp_sign.bin"

        PRIVATE_KEY_FILE = self.tmpDir + "blk0_pri.pem"
        PRIVATE_2NDKEY_FILE = self.tmpDir + "blk1_pri.pem"

        CheckFileFail(input)
        Info.CHIP = chip_type
        self.generate_private_pem2(KEY1_PRI_BIN, PRIVATE_KEY_FILE)
        self.generate_private_pem2(KEY2_PRI_BIN, PRIVATE_2NDKEY_FILE)
        with open(PRIVATE_KEY_FILE, 'rb') as f:
            private_rsa_root_key = f.read().decode().strip()

        with open(PRIVATE_2NDKEY_FILE, 'rb') as f:
            private_rsa_loader_key = f.read().decode().strip()

        content = ReadFileContent(Info.MARKET_ID_FILE)
        GxSecureManager.gm_loader_sign(chip_type, input, TEMP_FILE10, private_rsa_root_key, private_rsa_loader_key, content)
        CheckFileFail(TEMP_FILE10)

        return_data = ReadPartBytesOfFile(TEMP_FILE10)
        DeleteFile(TEMP_FILE10)
        DeleteFile(PRIVATE_KEY_FILE)
        DeleteFile(PRIVATE_2NDKEY_FILE)

        return return_data

    def encrypt_for_bootloader(self, input, algo, mode, stage1_key, stage2_key, stage1_key_der=None, stage2_key_der=None, iv=None, \
            stage1_enc=True, stage2_enc=True, stage1_enc_der=False, stage2_enc_der=False, output_dir=None, flash_type='nor'):

        ALIGN_VALUE = 16
        if Info.CHIP == "sirius":
            LOADER_STAGE1_ENCRYPT_KEY_START=16340  #0x3FD4
            LOADER_STAGE1_ENCRYPT_KEY_LENGTH=16
            LOADER_STAGE1_ENCRYPT_FLAG=11111111
            LOADER_STAGE1_ENCRYPT_FLAG_START=16356 #0x3FE4
            LOADER_STAGE1_ENCRYPT_FLAG_LENGTH=4
            LOADER_STAGE1_ENCRYPT_DATA_START=32
            LOADER_STAGE1_ENCRYPT_DATA_SIZE=15536

            LOADER_STAGE2_ENCRYPT_KEY_START=16390  #0x4006
            LOADER_STAGE2_ENCRYPT_KEY_LENGTH=16
            LOADER_STAGE2_ENCRYPT_FLAG=11111111
            LOADER_STAGE2_ENCRYPT_FLAG_START=16386 #0x4002
            LOADER_STAGE2_ENCRYPT_FLAG_LENGTH=4
            LOADER_STAGE2_ENCRYPT_DATA_START=16416 #0x4020

            RESERVE1_SIZE=32	# 0x0 ~ 0x20
            RESERVE2_SIZE=772	# 0x3cd0 ~ 0x3fd4
            RESERVE3_SIZE=26	# 0x3fe8 ~ 0x4002
            RESERVE4_SIZE=10	# 0x4016 ~ 0x4020

            reserve1_bytes = ReadPartBytesOfFile(input, 0, RESERVE1_SIZE)
            stage1_data_bytes = ReadPartBytesOfFile(input, LOADER_STAGE1_ENCRYPT_DATA_START, LOADER_STAGE1_ENCRYPT_DATA_SIZE)
            reserve2_bytes = ReadPartBytesOfFile(input, LOADER_STAGE1_ENCRYPT_DATA_START + LOADER_STAGE1_ENCRYPT_DATA_SIZE, \
                    RESERVE2_SIZE)
            loader_stage1_decrypt_key_bytes = ReadPartBytesOfFile(input, LOADER_STAGE1_ENCRYPT_KEY_START, \
                    LOADER_STAGE1_ENCRYPT_KEY_LENGTH)
            loader_stage1_decrypt_flag_bytes = ReadPartBytesOfFile(input, LOADER_STAGE1_ENCRYPT_FLAG_START, \
                    LOADER_STAGE1_ENCRYPT_FLAG_LENGTH)
            reserve3_bytes = ReadPartBytesOfFile(input, LOADER_STAGE1_ENCRYPT_FLAG_START + LOADER_STAGE1_ENCRYPT_FLAG_LENGTH, \
                    RESERVE3_SIZE)
            loader_stage2_decrypt_flag_bytes = ReadPartBytesOfFile(input, LOADER_STAGE2_ENCRYPT_FLAG_START, \
                    LOADER_STAGE2_ENCRYPT_FLAG_LENGTH)
            loader_stage2_decrypt_key_bytes = ReadPartBytesOfFile(input, LOADER_STAGE2_ENCRYPT_KEY_START, \
                    LOADER_STAGE2_ENCRYPT_KEY_LENGTH)
            reserve4_bytes = ReadPartBytesOfFile(input, LOADER_STAGE2_ENCRYPT_KEY_START + LOADER_STAGE2_ENCRYPT_KEY_LENGTH, \
                    RESERVE4_SIZE)

            stage2_data_bytes = ReadPartBytesOfFile(input, LOADER_STAGE2_ENCRYPT_DATA_START)

            if stage1_enc:
                stage_file_len = len(stage1_data_bytes)
                reserve_len = stage_file_len % ALIGN_VALUE
                data_len = stage_file_len - reserve_len

                DeleteFile(self.tmpDir + 'stage1_data_to_en')
                SaveData(self.tmpDir + 'stage1_data_to_en', stage1_data_bytes[:data_len])
                stage1_data_reserve = stage1_data_bytes[data_len:]

                DeleteFile(self.tmpDir + 'stage1_data.en')
                self.encrypt(self.tmpDir + 'stage1_data_to_en', self.tmpDir + 'stage1_data.en', algo, mode, stage1_key, iv)
                CheckFileFail(self.tmpDir + 'stage1_data.en')
                stage1_data_en = ReadPartBytesOfFile(self.tmpDir + 'stage1_data.en')

                output_data = bytes()
                output_data += reserve1_bytes
                output_data += stage1_data_en
                output_data += stage1_data_reserve
                output_data += reserve2_bytes

                DeleteFile(self.tmpDir + 'stage1_data.en')
                DeleteFile(self.tmpDir + 'stage1_data_to_en')

                if stage1_enc_der:
                    stage1_key_bytes = hex2byte(stage1_key)
                    SaveData(self.tmpDir + 'stage1_key', stage1_key_bytes)
                    self.encrypt(self.tmpDir + 'stage1_key', self.tmpDir + 'stage1_key_enc', algo, 'ecb', stage1_key_der)
                    output_data += ReadPartBytesOfFile(self.tmpDir + 'stage1_key_enc')
                    DeleteFile(self.tmpDir + 'stage1_key_enc')
                    DeleteFile(self.tmpDir + 'stage1_key')
                else:
                    output_data += loader_stage1_decrypt_key_bytes

                output_data += hex2byte(LOADER_STAGE1_ENCRYPT_FLAG)
                output_data += reserve3_bytes
            else:
                output_data = bytes()
                output_data += reserve1_bytes
                output_data += stage1_data_bytes
                output_data += reserve2_bytes
                output_data += loader_stage1_decrypt_key_bytes
                output_data += loader_stage1_decrypt_flag_bytes
                output_data += reserve3_bytes

            if stage2_enc:
                if stage2_enc_der:
                    stage2_key_bytes = hex2byte(stage2_key)
                    SaveData(self.tmpDir + 'stage2_key', stage2_key_bytes)
                    self.encrypt(self.tmpDir + 'stage2_key', self.tmpDir + 'stage2_key_enc', algo, 'ecb', stage2_key_der)
                    loader_stage2_decrypt_key_bytes = ReadPartBytesOfFile(self.tmpDir + 'stage1_key_enc')
                    DeleteFile(self.tmpDir + 'stage2_key_enc')
                    DeleteFile(self.tmpDir + 'stage2_key')

                loader_stage2_decrypt_flag_bytes = hex2byte(LOADER_STAGE2_ENCRYPT_FLAG)

                output_data += loader_stage2_decrypt_flag_bytes
                output_data += loader_stage2_decrypt_key_bytes
                output_data += reserve4_bytes

                stage_file_len = len(stage2_data_bytes)
                reserve_len = stage_file_len % ALIGN_VALUE
                data_len = stage_file_len - reserve_len

                DeleteFile(self.tmpDir + 'stage2_data_to_en')
                SaveData(self.tmpDir + 'stage2_data_to_en', stage2_data_bytes[:data_len])
                stage2_data_reserve = stage2_data_bytes[data_len:]

                DeleteFile(self.tmpDir + 'stage2_data.en')
                self.encrypt(self.tmpDir + 'stage2_data_to_en', self.tmpDir + 'stage2_data.en', algo, mode, stage2_key, iv)
                CheckFileFail(self.tmpDir + 'stage2_data.en')
                stage2_data_en = ReadPartBytesOfFile(self.tmpDir + 'stage2_data.en')

                output_data += stage2_data_en
                output_data += stage2_data_reserve
                DeleteFile(self.tmpDir + 'stage2_data.en')
                DeleteFile(self.tmpDir + 'stage2_data_to_en')
            else:
                output_data += loader_stage2_decrypt_flag_bytes
                output_data += loader_stage2_decrypt_key_bytes
                output_data += reserve4_bytes
                output_data += stage2_data_bytes
        else:
            print("not support %s"%Info.CHIP)


    def encrypt_for_other(self, input, algo, mode, key, iv=None, output_dir=None):
        ALIGN_VALUE = 16

        raw_data_bytes = ReadPartBytesOfFile(input)
        raw_data_len = len(raw_data_bytes)
        reserve_len = raw_data_len % ALIGN_VALUE
        data_len = raw_data_len - reserve_len

        DeleteFile(self.tmpDir + 'raw_data_to_en')
        SaveData(self.tmpDir + 'raw_data_to_en', raw_data_bytes[:data_len])
        raw_data_reserve = raw_data_bytes[data_len:]

        DeleteFile(self.tmpDir + 'raw_data.en')
        self.encrypt(self.tmpDir + 'raw_data_to_en', self.tmpDir + 'raw_data.en', algo, mode, key, iv)
        CheckFileFail(self.tmpDir + 'raw_data.en')
        stage2_data_en = ReadPartBytesOfFile(self.tmpDir + 'raw_data.en')

        stage2_data_en += raw_data_reserve
        DeleteFile(self.tmpDir + 'raw_data.en')
        DeleteFile(self.tmpDir + 'raw_data_to_en')

def help():
    print("")
    print("Instructions:")
    print("  ./signature.py <key_file | clean>")
    print("\tkey_file : the name of the generated key file, these generated files are located in the sm2_new_key directory")
    print("\tclean    : delete the sm2_new_key directory")
    print("")
    print("example:")
    print("  step1: Executing the \"./sm2_generate_key.sh test\" command")
    print("  step2: View the files in the sm2_new_key directory, as follows: ")
    print("\tsm2_new_key/")
    print("\t|-- test.pri                // private key")
    print("\t|-- test_private.bin        // private key, binary file")
    print("\t|-- test.pub                // public key")
    print("\t|-- test_public_x.bin       // the x coordinate of public key, binary file,")
    print("\t|-- test_public_y.bin       // the y coordinate of public key, binary file,")

def generate_prikey(name=None):
    gm = GmTool()
    if name == None:
        gm.generate_private_key(Info.SM2_NEW_KEY_DIR + os.sep + "blk0_pri.pem")
        gm.generate_private_bin(Info.SM2_NEW_KEY_DIR + os.sep + "blk0_pri.pem", Info.SM2_NEW_KEY_DIR + os.sep + "blk0_pri.bin")
        gm.generate_private_key(Info.SM2_NEW_KEY_DIR + os.sep + "blk1_pri.pem")
        gm.generate_private_bin(Info.SM2_NEW_KEY_DIR + os.sep + "blk1_pri.pem", Info.SM2_NEW_KEY_DIR + os.sep + "blk1_pri.bin")
    else:
        gm.generate_private_key(Info.SM2_NEW_KEY_DIR + os.sep + name + "_pri.pem")
        gm.generate_private_bin(Info.SM2_NEW_KEY_DIR + os.sep + name + "_pri.pem", Info.SM2_NEW_KEY_DIR + os.sep + name + "_pri.bin")

def generate_pubkey(name=None):
    gm = GmTool()
    if name != None:
        gm.generate_public_bin(name, Info.SM2_NEW_KEY_DIR + os.sep + "pub.bin")

def generate_pribin(name):
    gm = GmTool()
    if name != None:
        gm.generate_private_bin(name, Info.SM2_NEW_KEY_DIR + os.sep + "pri.bin")

def generate_pripem(name):
    gm = GmTool()
    if name != None:
        gm.generate_private_pem(name, Info.SM2_NEW_KEY_DIR + os.sep + "pri.pem")


def sign_bootloader(input, level):
    try:
        level = int(level)
    except:
        print("argument level is wrong, please input 1/2")
        sys.exit(-1)
    gm = GmTool()
    data = gm.sign_bootloader_file("canopus", input)
    SaveData(input, data)

def sign_general_file(input):
    gm = GmTool()
    output = input + ".sign"
    gm.sign_for_other_file(input, output)

if __name__ == '__main__':
    usage = "usage: \t%prog -g\n\t%prog -s filename\n\t%prog -b filename <level>"
    parser = OptionParser(usage)
    parser.add_option("-b", "--bootloader", action="store_true", dest="bootloader", help = "signed bootloader operation")
    parser.add_option("-s", "--sign",       action="store",      dest="sign",       help = "signed general file operation")
    parser.add_option("-g", "--gen-prikey", action="store_true", dest="gen_prikey", help = "generate private key ")
    parser.add_option("-z", "--gen-pribin", action="store_true", dest="gen_pribin", help = "generate private key bin")
    parser.add_option("-o", "--gen-pripem", action="store_true", dest="gen_pripem", help = "generate private key pem")
    parser.add_option("-p", "--gen-pubbin", action="store_true", dest="gen_pubkey", help = "generate public key bin")

    (options, args) = parser.parse_args()

    if len(parser._get_args(None)) == 0:
        parser.print_help()

    #print(options, args)

    if options.gen_prikey and (options.sign or options.bootloader):
        parser.error("options (-s OR -b) and -g are mutually exclusive")

    if options.gen_pubkey and (options.sign or options.bootloader):
        parser.error("options (-s OR -b) and -p are mutually exclusive")

    if options.gen_prikey == True:
        if len(args) == 0:
            generate_prikey()
        else:
            generate_prikey(args[0])

    if options.gen_pubkey == True:
        if len(args) == 0:
            parser.print_help()
        else:
            generate_pubkey(args[0])

    if options.gen_pribin== True:
        if len(args) == 0:
            parser.print_help()
        else:
            generate_pribin(args[0])

    if options.gen_pripem== True:
        if len(args) == 0:
            parser.print_help()
        else:
            generate_pripem(args[0])

    if options.sign != None:
        sign_general_file(options.sign)

    if options.bootloader == True:
        if len(args) != 2:
            parser.print_help()
        else:
            sign_bootloader(args[0], args[1])
