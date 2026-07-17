#/bin/sh
# ex. V1.2: 0102
BOOT_HEAD_CURRENT_VERSION='0100'

# CONFIG_BOOT_CHIP='6612'
# CONFIG_ROM_BAUD=115200
CONFIG_BOOT_CHIP=$1
CONFIG_ROM_BAUD=$2
CONFIG_HEAD_VERSION=$3

[ -z ${CONFIG_HEAD_VERSION} ] && {
	CONFIG_HEAD_VERSION=${BOOT_HEAD_CURRENT_VERSION}
}

# 12345678 -> 78563412
rev_bytes()
{
	local first=`echo $1 | cut -b -2`
	local others=`echo $1 | cut -b 3-`
	if [ -z "$others" ]; then
		echo ${first}
		return
	fi
	echo -n $(rev_bytes $others)${first}
}

boot_head_format()
{
	# %d -> %x
	printf %08x $1
}

BOOT_HEAD_MAGIC='626f6f74'
BOOT_HEAD_VERSION=${CONFIG_HEAD_VERSION}
BOOT_HEAD_CHIP=${CONFIG_BOOT_CHIP}
BOOT_HEAD_BAUD=$(boot_head_format ${CONFIG_ROM_BAUD})
BOOT_HEAD_TMP=$(rev_bytes ${BOOT_HEAD_MAGIC})$(rev_bytes ${BOOT_HEAD_VERSION})$(rev_bytes ${BOOT_HEAD_CHIP})$(rev_bytes ${BOOT_HEAD_BAUD})

num_used=`echo -n ${BOOT_HEAD_TMP} | wc -c`
num_reserve=`expr 64 - ${num_used}`
BOOT_HEAD_RESERVE=`printf %0${num_reserve}x 0`

BOOT_HEAD=${BOOT_HEAD_TMP}${BOOT_HEAD_RESERVE}

echo -n ${BOOT_HEAD}
