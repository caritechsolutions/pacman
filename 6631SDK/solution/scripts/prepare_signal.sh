#!/bin/bash

pre=$1
signal=$2
CROSS_COMPILE=$3

if [[ $CROSS_COMPILE = "" ]]; then
    echo -e "\033[1;31mplease check cross compile setting about CROSS_COMPILE!\033[0m"
    exit 1
fi

if [ -e $pre ];then
	rm -f $pre
fi

if [ -e $signal ];then
	rm -f $signal
fi

echo "#include \"app_config.h\"" > $1

if [[ $OS == "linux" ]]; then
    echo "#define LINUX_OS" >> $1
fi
if [[ $OS == "ecos" ]]; then
    echo "#define ECOS_OS" >> $1
fi

files=`find app -name "*.c" | xargs grep "SIGNAL_HANDLER" | awk -F ':' '{print $1}' | sort -u`

for file in $files; do
    cat $file | grep -E "#ifndef|#ifdef|#if|#else|#elif|#endif|SIGNAL_HANDLER" --color >> $1
done

${CROSS_COMPILE}gcc -E -I./app/include $1 > $2
