#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import sys
import json
import os

class config(object):
    defaultJson =  {
                    'version' : '1.7', \
                    'sign_file' : '',  \
                    'auto_type' : True,  \
                    'flash_type' : 'nand',  \
                    'kernel_file' : '',  \
                    'flash_all_config' : '',  \
                    'fill' : False,  \
                    'max_len' : '151',  \
                    'update_mrs' : True,  \
                    'conf' : '',  \
                    'chip_type' : 'sirius',  \
                }


    def __init__(self):
        self.json = None
        self.jsonPath = "config/config.json"

        ret = self.LoadConfig()
        if ret == -1:
            self.CreateDefaultJson()
            self.LoadConfig()
        elif ret == -2:
            self.json = None

        if self.json != None and \
                self.json['version'] != self.defaultJson['version']:
            self.CreateDefaultJson()
            self.LoadConfig()

    def LoadConfig(self):
        try:
            self.json = json.load(open(self.jsonPath))
        except IOError:
            print("json not exist")
            return -1
        except:
            print("parse json file fail.")
            return -2
        return 0

    def CreateDefaultJson(self):
        try:
            if not os.path.exists('config'):
                os.mkdir('config')

            json.dump(self.defaultJson, open(self.jsonPath, 'w'), sort_keys=True, indent=4)
        except:
            print("write json file fail.")
            return -1
        return 0

    def SaveJson(self, conf):
        try:
            json.dump(conf, open(self.jsonPath, 'w'), sort_keys=True, indent=4)
        except:
            print("write json file fail.")
            return -1
        return 0

if __name__ == '__main__':
    try:
        conf = json.load(open("config/config.json"))
        print("version = %s" %conf["version"])
        print("path= %s" %conf["path"])
    except IOError:
        print("json not exist")
    except:
        print("parse json file fail.")

    cw = config()
    cw.CreateDefaultJson()
