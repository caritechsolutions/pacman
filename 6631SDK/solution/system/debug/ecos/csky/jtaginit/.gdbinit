tar jtag jtag://127.0.0.1:1025
set $cr18=0x7d
#set *0x0030a178 |= 0x7
#set *0x0030a178 |= 0x3
#set *0x0030a178 |= (1<<4)|(1<<5)

load
set print pretty
#b main
#display /i $pc
#c
#q
