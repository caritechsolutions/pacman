#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import subprocess
#from log import logger

class UsrError(Exception):
    def __init__(self, message):
        self.msg = message

    def __str__(self):
        return self.msg

def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

def get_self_dir():
    if getattr(sys, 'frozen', False) and hasattr(sys, '_MEIPASS'):
        return os.path.dirname(sys.executable)
    else:
        return os.path.dirname(os.path.abspath(__file__))

def run_cmd(full_cmd):
    #print(full_cmd)

    try:
        ret = subprocess.check_output(full_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        msg = "exception! return: %d\ncmd: %s\nstdout: %s"%(e.returncode, e.cmd, bytes.decode(e.output))
        raise UsrError(msg)
    except Exception as e:
        msg = "cmd: {0}\nexception: {1}".format(full_cmd, e)
        raise UsrError(msg)
    else:
        msg = "%s"%bytes.decode(ret)

    msg = msg.strip()
    return msg

def byte2hex(data_bytes):
    """
    Convert a byte string to it's hex string representation e.g. for output.
    """
    return ''.join([ "%02X" % x for x in data_bytes]).strip()

def hex2byte(hexStr):
    """
    Convert a string hex byte values into a byte string. The Hex Byte values may
    or may not be space separated.
    """
    return bytes.fromhex(hexStr)

def TakePartStr(content, start, end):
    """
    take part from string, start and end are string type.
    """
    content = content.split(start, 1)[1]
    content = content.split(end, 1)[0]
    return content

def TakePartBytes(data, offset, length = None):
    """
    take part from bytes. offset and length are number
    """
    if length == None:
        return data[offset:]
    else:
        if len(data) >= length:
            return data[offset:offset+length]
        else:
            return data

def ReadPartBytesOfFile(name, offset = 0, length = None):
    """
    take part bytes from file. offset and length are number
    """
    data = None
    try:
        fo = open(name, "rb")
        data = fo.read()
        fo.close()
    except:
        msg = "open file fail: %s"%name
        raise UsrError(msg)
    else:
        return TakePartBytes(data, offset, length)

def ReadFileContent(name):
    try:
        content = ""
        fo = open(name, "r")
        for line in fo.readlines():
            line = line.strip()
            content += line
        fo.close()
    except:
        msg = "open file fail: %s"%name
        raise UsrError(msg)
    return content

def CreateParentDir(name):
    dirname = os.path.dirname(name)

    if dirname.strip() != '' and not os.path.exists(dirname):
        os.makedirs(dirname)

def SaveData(name, data):
    CreateParentDir(name)
    fo = open(name, 'wb')
    fo.write(data)
    fo.flush()
    fo.close()

def SaveStr(name, string):
    CreateParentDir(name)
    fo = open(name, 'w')
    fo.write(string)
    fo.flush()
    fo.close()

def DeleteFile(name):
    if os.path.exists(name):
        os.remove(name)

def CheckFileFail(name):
    if not (os.path.exists(name) and os.path.getsize(name) != 0):
        msg = "file not exist: \"%s\""%name
        raise UsrError(msg)

def CheckGenFileOK(name):
    if os.path.exists(name) and len(ReadPartBytesOfFile(name)) != 0:
        #print("generate file OK: %s"%name)
        return 0
    else:
        msg = "generate file fail: %s"%name
        raise UsrError(msg)

def TakeBaseName(path):
    name = os.path.basename(path)
    return os.path.splitext(name)[0]

def TakeNameExt(path):
    name = os.path.basename(path)
    return os.path.splitext(name)[1]

def TrimLines(string):
    content = ""
    for line in string.splitlines():
        line = line.strip()
        content += line
    return content

def int2(x):
    if x.startswith('0x'):
        return int(x, 16)
    else:
        return int(x, 10)

def is5CopyFirmware(name):
    copyLen = [0x800, 0x1000, 0x2000, 0x4000, 0x8000]

    for length in copyLen:
        for i in range(5):
            offset = length * i
            try:
                magic = ReadPartBytesOfFile(name, offset, 4)
                if magic != b'\xaa\x55\xaa\x55':
                    break
            except:
                return False
        else:
            return True
    return False

def is_nand_firmware(name):
    magic = ReadPartBytesOfFile(name, 0, 4)

    if magic == b'\xbb\x55\xbb\x55':
        ''' spi nand'''
        return True
    elif magic == b'\xaa\x55\xaa\x55':
        if is5CopyFirmware(name):
            '''paralled nand'''
            return True
        else:
            '''nor'''
            return False
    elif magic == b'\x74\x6f\x6f\x62':
        '''boot firmware'''
        return False
    else:
        raise UsrError("unknown firmware")

def get_chip_and_flash_type(name):
    magic = ReadPartBytesOfFile(name, 0, 4096)
    if len(magic) < 4096:
        raise UsrError("unknown firmware")
    else:
        if all(data == 0xFF for data in magic[1024+56:4095]):
            return "canopus", "nand"
        elif all(data == 0xFF for data in magic[1024+56:2047]):
            return "canopus", "nand"
    if is_nand_firmware(name):
        return "sirius", "nand"
    else:
        return "sirius", "nor"

def ParseConf(conf):
    binPath = {}
    try:
        fo = open(conf, "r")
        content = fo.read()
        fo.close()
    except:
        msg = "open file fail: %s"%conf
        raise UsrError(msg)
    index = content.rfind("#-------")
    content = content[index:]
    #print(content)
    lines = content.splitlines()
    #print(lines)

    for line in lines:
        if line.startswith('#'):
            lines.remove(line)
        if line.strip() == '':
            lines.remove(line)

    for line in lines:
        items = line.split()
        if len(items) >= 2:
            binPath[items[0]] = items[1]

    return binPath



