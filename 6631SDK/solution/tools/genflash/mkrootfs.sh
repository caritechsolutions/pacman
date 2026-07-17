#!/bin/sh
if [ -z "$1" ];then
	rootfs="/opt/gxrootfs_nfs"
else
	rootfs="$1"
fi

cp "$rootfs" /tmp/gxrootfs_nfs -ar
rm -rf /tmp/gxrootfs_nfs/dvb/*
rm -rf /tmp/gxrootfs_nfs/home/gx/*
find /tmp/gxrootfs_nfs -type d -name ".svn" | xargs rm -rf
mv /tmp/gxrootfs_nfs/etc/rcS.d/_S01mount /tmp/gxrootfs_nfs/etc/rcS.d/S01mount
./mksquashfs /tmp/gxrootfs_nfs ./rootfs.bin -noappend -no-duplicates > /dev/null
sync
rm -rf /tmp/gxrootfs_nfs
