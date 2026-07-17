tar jtag jtag://127.0.0.1:1025
#tar jtag jtag://192.168.188.141:1025
set $cr18=0x7d
#set *0xa030a148|=(1<<6)
load




define osd_reset
	set $i = 0
	set *0xa4800090 &=~1
	while ($i < 150)
        set $i = $i + 1
    end
	set *0xa4800090 |= 1
end



