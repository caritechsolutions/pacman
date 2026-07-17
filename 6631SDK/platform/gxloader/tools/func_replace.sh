#!/bin/bash

#help: ${1} is input file, ${2} is parse directory/file
auto_replace(){

	inputfile=${1}
	replace_dir=${2}
	input_match_str_pre="multiple definition of \`"
	input_match_str_suf="'"
	replace_str_pre="gx_tmp_"
	replace_str_suf=""

	echo ""
	echo "input file name=${inputfile}"
	echo "replace dir=${replace_dir}"
	echo "input match str prefix=${input_match_str_pre}"
	echo "input match str suffix=${input_match_str_suf}"
	echo "replace str prefix=${replace_str_pre}"
	echo "replace str suffix=${replace_str_suf}"
	echo ""
	echo "start to replace:"
	echo ""

	tmpfile="tmp.${inputfile}"
	grep -r "${input_match_str_pre}" ${inputfile} > ${tmpfile}
	sed -i "s/.*${input_match_str_pre}\(.*\)${input_match_str_suf}.*/\1/g" ${tmpfile}

	insert_line=`sed -n '$=' ${tmpfile} `   #get total line of file.
	for((i=1;i<=${insert_line};i++))
	do
		replace_str=`sed -n "${i}p" ${tmpfile}`
		if [ -n "${replace_str}" ]; then

			src_str="${replace_str}("	#add '(' to match function define/invoke
			dst_str="${replace_str_pre}${replace_str}${replace_str_suf}("
			exists_flag=`grep -rl "${src_str}" ${replace_dir} --exclude=${inputfile}`
			if [ -n "$exists_flag" ]; then
				echo "replace: ${src_str} -> ${dst_str}"
				echo "${exists_flag}"|xargs sed -i "s/${src_str}/${dst_str}/g"
			fi

			src_str="${replace_str};"	#add ';' to match function define/invoke
			dst_str="${replace_str_pre}${replace_str}${replace_str_suf};"
			exists_flag=`grep -rl "${src_str}" ${replace_dir} --exclude=${inputfile}`
			if [ -n "$exists_flag" ]; then
				echo "replace: ${src_str} -> ${dst_str}"
				echo "${exists_flag}"|xargs sed -i "s/${src_str}/${dst_str}/g"
			fi

		fi
	done

	rm ${tmpfile}
	echo ""
	echo "replace completely."
	echo ""
}

auto_replace ${1} ${2}
