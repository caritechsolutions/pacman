#!/bin/sh
if [ -z "$1" ];then
	rootfs="/opt/gxrootfs_nfs"
else
	rootfs="$1"
fi

cp "$rootfs""/home/gx" /tmp/ -ar
find /tmp/gx -type d -name ".svn" | xargs rm -rf
mkfs.jffs2 -r /tmp/gx -o ./cfg.bin
sync
rm -rf /tmp/gx
