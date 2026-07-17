#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import subprocess
from common import *
import tempfile
import shutil

class GmToolBase(object):

    def __init__(self):
        if sys.platform == "win32":
            self.gmssl = resource_path("bin" + os.sep + "gmssl.exe")
            cnf_path = r"C:\gmssl\openssl.cnf"
            if not (os.path.exists(cnf_path) and len(ReadPartBytesOfFile(cnf_path)) != 0):
                data_cnf = ReadPartBytesOfFile(resource_path(r"bin\openssl.cnf"))
                SaveData(cnf_path, data_cnf)
                CheckFileFail(cnf_path)
        else:
            self.gmssl = resource_path("bin" + os.sep + "gmssl.elf")

    def PrepareLinuxEnv(self):
        source_path = resource_path(r"bin/openssl.cnf")
        openssl_cnf_path = "/tmp/nc_gmssl"
        if not os.path.exists(openssl_cnf_path + os.sep + "openssl.cnf"):
            try:
                os.mkdir(openssl_cnf_path)
            except:
                pass

            try:
                shutil.copy(source_path, openssl_cnf_path)
            except:
                pass

    def RunSubProcess(self, args, workdir=None):
        if sys.platform != "win32":
            self.PrepareLinuxEnv()
        return run_cmd(args)

    def RunGmsslVersion(self):
        return self.RunSubProcess([self.gmssl, "version"])
