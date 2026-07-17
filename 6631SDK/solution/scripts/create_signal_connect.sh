#!/bin/bash
file_name_cplus=$GXSRC_PATH/app/signal_connect_cplus.cpp
func_name=signal_connect_handler
pre_file=$GXSRC_PATH/app/prepare_for_signal.c
signal_handler_file=$GXSRC_PATH/app/signal_handler.c

chmod +x $GXSRC_PATH/scripts/prepare_signal.sh
$GXSRC_PATH/scripts/prepare_signal.sh $pre_file $signal_handler_file $1

echo '//create signal_connect_cplus.cpp by bash!!'> $file_name_cplus
echo '#include "gui_core.h"'>> $file_name_cplus
echo " ">>$file_name_cplus
echo " ">>$file_name_cplus

echo "extern \"C\" status_t $func_name(void);">>$file_name_cplus

echo "__BEGIN_DECLS">>$file_name_cplus

cat $signal_handler_file | grep -a SIGNAL_HANDLER | sort |awk '{print  $3}'|awk -F'(' '{print "extern int "$1 "(const char* widgetname, void *usrdata);"}' >>$file_name_cplus

echo " ">>$file_name_cplus
echo " ">>$file_name_cplus

echo "__END_DECLS">>$file_name_cplus

echo "status_t $func_name(void)">>$file_name_cplus
echo "{">>$file_name_cplus
echo "	status_t ret = GXCORE_SUCCESS;">>$file_name_cplus

cat $signal_handler_file | grep -a SIGNAL_HANDLER | sort |awk '{print  $3}'|awk -F'(' '{print "\tret |= SIGNAL_CONNECT("$1");"}' >>$file_name_cplus

echo " ">>$file_name_cplus

echo "	return ret;">>$file_name_cplus
echo "}">>$file_name_cplus

if [ -e $pre_file ];then
	rm -f $pre_file
fi

if [ -e $signal_handler_file ];then
	rm -f $signal_handler_file
fi
