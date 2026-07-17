#!/bin/bash

#cd ${GXSRC_PATH}/cmock_based/gxcore_api/ && sh config os

ret=$?
if [ ${ret} -ne 0 ];then
    echo Error: ${ret}
    exit $?
fi
    

declare -a platform
platform[0]="i386 linux"
platform[1]="arm linux" 
platform[2]="arm ecos"
platform[3]="ckcore ecos"

echo ${#platform[*]}
for ((i=0; i<${#platform[*]}; i++))
#for ((i=0; i<4; i++))
do
    sed -i -e "1,5s/^\([^#].\+\)/#\1/g" ${GXSRC_PATH}/env.sh

    plat=${platform[${i}]}
    for str in ${plat}
    do
        sed -i -e "1,5s/^#\(.*${str}.*\)/\1/" ${GXSRC_PATH}/env.sh
    done

    cd ${GXSRC_PATH}
    source env.sh                                                   && \
    cd ${GXSRC_PATH}/cmock_based/gxcore_api/os/                     && \
    
    cd ${GXSRC_PATH}/cmock_based/cmockery                           && \
    make clean && make && make install                              && \
    cd -                                                            && \
    
    make clean && make

    ret=$?
    if [ ${ret} -ne 0 ];then
        echo Error: ${ret}
        exit $?
    fi
    
done


