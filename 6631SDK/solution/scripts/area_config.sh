#!/bin/bash
##############################################################################
# zhangzhy, zhangzhy@nationalchip.com
# Copyright(C) 2013-2020, HangZhou NationalChip Science and Technology Corp.
# parse project areauage config and generate app_t2_area_config.h, app_t2_area_config.c
##############################################################################

output_config_file_h=$1
output_config_file_c=$2
output_config_area=$3

#######################################################################################################
# Area Setting

echo "#include \"app_config.h\"" > $output_config_file_c
echo "#if DEMOD_DVB_T || DEMOD_ISDBT" >> $output_config_file_c
echo -e "#include \"app_t2_area_config.h\"\n" >> $output_config_file_c
echo "char *g_CountryName[] = {">> $output_config_file_c

#parsing default Area
config_area_num=0
## BR2_AREA_DEFAULT_AAA_XXX_XXX
select_area_short_default=`cat .config|grep "=y" | grep "BR2_AREA_DEFAULT_" |awk -F"=" '{print $1}'|awk -F"_" '{print $4}'`
select_area_default=`cat .config|grep "=y" | grep "BR2_AREA_DEFAULT_" |awk -F"=" '{print $1}'|awk -F"_" '{for(i=5;i<=NF;i++) {if(i!=NF) printf $i"_";else print $i}}'`
select_area_capital=`echo $select_area_default | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'| awk -F '_' '{for(i=1;i<=NF;i++) {if(i!=NF) printf $i" ";else print $i}}'`

echo "#ifndef __APP_T2_AREA_CONFIG_H__">$output_config_file_h
echo "#define __APP_T2_AREA_CONFIG_H__">>$output_config_file_h
echo "">>$output_config_file_h
echo "#include \"app_config.h\"" >>$output_config_file_h
echo "#if DEMOD_DVB_T || DEMOD_ISDBT">>$output_config_file_h

if [[ -n $select_area_default ]];then
	echo -e "#define DEFAULT_T2_COUNTRY\t\t\t\"${select_area_capital}\"" >> $output_config_file_h
	echo -e "#define COUNTRY_${select_area_short_default}\t\t\t\"${select_area_capital}\"\t\t\t/*$((config_area_num+1))*/" >> $output_config_file_h
	echo -e "\tCOUNTRY_${select_area_short_default}," >> $output_config_file_c
	echo "\"${select_area_capital}\"" > ${output_config_area}
	config_area_num=$((config_area_num+1))
fi

# parsing AREA
## BR2_AREA_AAA_XXX_XXX
select_area_short=`cat .config|grep "=y" | grep "BR2_AREA_" |grep -v ${select_area_default} |awk -F"=" '{print $1}'|awk -F"_" '{print $3}'`
select_area_list=`cat .config|grep "=y" | grep "BR2_AREA_" |grep -v ${select_area_default} | awk -F"=" '{print $1}'|awk -F"_" '{for(i=4;i<=NF;i++) {if(i!=NF) printf $i"_";else print $i}}'`

area_short_array=(${select_area_short})
area_list_array=(${select_area_list})
area_count=${#area_list_array[*]}

echo -e "Selected Area:\033[032m["$select_area_default $select_area_list"]\033[0m"
for i in `seq 0 $((area_count-1))`
do
	each_area_capital=`echo ${area_list_array[i]} | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'| awk -F '_' '{for(i=1;i<=NF;i++) {if(i!=NF) printf $i" ";else print $i}}'`
	echo -e "#define COUNTRY_${area_short_array[i]}\t\t\t\"${each_area_capital}\"\t\t\t/*$((config_area_num+1))*/" >> $output_config_file_h
	echo -e "\tCOUNTRY_${area_short_array[i]}," >> $output_config_file_c
	echo "\"${each_area_capital}\"" >> $output_config_area
	config_area_num=$((config_area_num+1))
done

echo "#define COUNTRY_MAX ${config_area_num}" >> $output_config_file_h
echo "" >> $output_config_file_h
echo "extern char *g_CountryName[];" >> $output_config_file_h
echo "" >> $output_config_file_h

echo "};" >> $output_config_file_c
echo "#endif" >> $output_config_file_c

echo "">>$output_config_file_h
echo "#endif">>$output_config_file_h
echo "#endif">>$output_config_file_h

