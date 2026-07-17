#!/usr/bin/env python2
# -*- coding: utf-8 -*-


import json
import sys
from collections import OrderedDict
import xml.etree.ElementTree as ET
from collections import Iterable


netapps_config = sys.argv[1]+"/.config"
netapps_allconfig = sys.argv[1]+"/output/allconfig.json"
xml_path = sys.argv[2]

def get_netapps_from_allconfig():
    dict_netapps = {}

    try:
        with open(netapps_allconfig, "rb") as f:
            js = json.load(f, object_pairs_hook=OrderedDict)
    except ValueError as e:
        print("can't parse the allconfig.json")
        dict_netapps = None
    except IOError as e:
        print("can't found the allconfig.json")
        dict_netapps = None

    if isinstance(dict_netapps,Iterable):
        for i in js:
            app_id = None
            menuconfig_id = None
            for j in js[i]:
                if j.decode("utf-8") == "APP_ID":
                    app_id = js[i][j.decode("utf-8")]
                if j.decode("utf-8") == "MENUCONFIG_ID":
                    menuconfig_id = js[i][j.decode("utf-8")]
            if (app_id != None)&(menuconfig_id != None):
                dict_netapps[menuconfig_id] = app_id

    return dict_netapps

def modify_default_xml(dict_netapps):
    if isinstance(dict_netapps,Iterable) == False:
        return

    xmlp = ET.XMLParser(encoding="utf-8")
    xml = ET.parse(xml_path,parser=xmlp)
    root = xml.getroot()

    netapp_group = None
    for group in root:
        if (group.tag == "group")&("name" in group.attrib):
            if (group.attrib["name"] == "netapp"):
                netapp_group = group

    if netapp_group == None:
        print("can't found netapps tag in %s"  %xml_path)
        return

    with open(netapps_config, "rb") as f:
        for i in f.readlines():
            menuconfig = i.split('=')
            if (len(menuconfig) != 2):              #需要满足格式要求
                continue
            elif (menuconfig[0] in dict_netapps)&('y' in menuconfig[1]):
                node = ET.SubElement(netapp_group,"item")
                node.attrib["display"] = dict_netapps[menuconfig[0]]
                node.attrib["value"] = "1"
                node.tail = '\n'

    xml.write(xml_path,"utf-8",True)
    return

dict_netapps = get_netapps_from_allconfig()
modify_default_xml(dict_netapps)




