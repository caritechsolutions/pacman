#!/usr/bin/env python3
# coding=utf-8

#eg: python3 net_print_recv.py

import socket
import struct

mcast_group_ip = "224.0.0.8"
mcast_group_port = 9001
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
local_ip = socket.gethostbyname(socket.gethostname())
s.bind(("0.0.0.0", mcast_group_port))
mreq = struct.pack("=4sl", socket.inet_aton(mcast_group_ip), socket.INADDR_ANY)
s.setsockopt(socket.IPPROTO_IP,socket.IP_ADD_MEMBERSHIP,mreq)

while 1:
    # 接收数据:
    print(s.recv(1024).decode('utf-8'))
s.close()
