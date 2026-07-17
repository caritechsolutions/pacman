#!/bin/bash

###############################################################################
# check parameter and set env
###############################################################################
FAMILY_NAME=$1

echo "Input family name : " $FAMILY_NAME
	

BUILD_DIR=$(pwd)
config_proj_path=${BUILD_DIR}/projects
config_family_list=$(ls -l ${config_proj_path} | awk '/^d/ {print $NF}')
echo "Selectable Family List:" $config_family_list


for each_family in $config_family_list
do
        if [[ $1 = $each_family ]];then
                FAMILY_FOUND=y
                break
        fi
done

echo "+-------------------------------------------------"
echo "| Relase package "
echo "+-------------------------------------------------"
if [[ $FAMILY_FOUND = y ]];then
       echo "Release Family Name:" $FAMILY_NAME
else
        echo "Cannot find family in projects"
        exit 1
fi

config_proj_list=$(ls -l ${config_proj_path}/${FAMILY_NAME} | awk '/^d/ {print $NF}')
echo "Release Project List:" $config_proj_list

CONFIG_CHIP_LIST=""
CONFIG_OS_LIST=""
CONFIG_COMPILE_LIST=""
CONFIG_LIB_LIST=""

for each_proj in $config_proj_list
do
	# abstract chip name
	chip_name=`echo ${each_proj} | awk -F "_" '{print $1}' | tr [:upper:] [:lower:]`
	if [[ $chip_name != "" ]];then
		find_chip=`echo ${CONFIG_CHIP_LIST} | grep -c $chip_name`	
		if [[ $find_chip == "0" ]];then
			CONFIG_CHIP_LIST=`echo ${CONFIG_CHIP_LIST} ${chip_name}`
		fi
	fi

	# abstrace os name
	os_name=`echo ${each_proj} | awk -F "_" '{print $2}' | tr [:upper:] [:lower:]`
	if [[ $os_name != "" ]];then
		find_os=`echo ${CONFIG_OS_LIST} | grep -c $os_name`
		if [[ $find_os == "0" ]];then
			CONFIG_OS_LIST=`echo ${CONFIG_OS_LIST} ${os_name}`
		fi
	fi

	# analyse compile
	if [[ $chip_name == "gx3113" || $chip_name == "gx3200" ]];then
		arch_name="arm"	
	elif [[ $chip_name == "gx3201" || $chip_name == "gx6601" || $chip_name == "gx6602" ]];then
		arch_name="csky"
	else
		arch_name=""
	fi

	if [[ $chip_name == "" || $os_name == "" ]];then
		break;
	else
		compile_name="${arch_name}-${os_name}"
		find_compile=`echo $CONFIG_COMPILE_LIST | grep -c $compile_name`
		if [[ $find_compile == "0" ]];then
			CONFIG_COMPILE_LIST=`echo ${CONFIG_COMPILE_LIST} ${compile_name}`
		fi
	fi

	other_lib_files=`find ${BUILD_DIR}/lib/${compile_name} -name "*.a"
    # echo "Other lib:" $other_lib_files
	if [[ $other_lib_files != "" ]];then
		CONFIG_LIB_LIST=`echo ${CONFIG_LIB_LIST} ${other_lib_files}`
	fi
done
uni_lib_list=""
for each_lib in $CONFIG_LIB_LIST
do
	find_lib=`echo $uni_lib_list | grep -c $each_lib`
	if [[ $find_lib == "0" ]];then
		uni_lib_list=`echo $uni_lib_list $each_lib`
	fi	
done

echo "CHIP:" $CONFIG_CHIP_LIST
echo "OS:" $CONFIG_OS_LIST
echo "COMPILE:" $CONFIG_COMPILE_LIST
echo "LIBRARY:" $uni_lib_list


###############################################################################
# copy source code, theme, config, library and tools
###############################################################################
config_release_path=${BUILD_DIR}/output/release
release_filter_dir=${config_release_path}/solution
if [ -e ${release_filter_dir} ];then
	rm -rf ${release_filter_dir}
fi
mkdir -p ${config_release_path}
mkdir -p ${release_filter_dir}

# copy bsp
mkdir -p ${release_filter_dir}/bsp
cp -R bsp/* ${release_filter_dir}/bsp

# copy preconfig files
mkdir -p ${release_filter_dir}/configs
for each_proj in $config_proj_list
do
	PROJ_NAME_CAP=$(echo ${each_proj} | tr [:lower:] [:upper:])
	grep -r "BR2_PROJ_${PROJ_NAME_CAP}=y" configs/ --exclude-dir=.svn |awk -F ":" -vconfig_dir=${release_filter_dir}/configs '{print "cp "$1" "config_dir}'|bash
done

# copy header
mkdir -p ${release_filter_dir}/include
cp -R include/* ${release_filter_dir}/include

# copy library
for each_lib in $CONFIG_LIB_LIST
do
	release_lib_path=`echo ${each_lib} | xargs dirname | sed -e 's/solution\/lib/solution\/output\/release\/solution\/lib/g'`
#	echo "Lib path:" $release_lib_path
	mkdir -p $release_lib_path
	cp $each_lib  $release_lib_path
done

# copy flash image tools 
mkdir -p ${release_filter_dir}/system
cp -r system/ecos_template  ${release_filter_dir}/system
cp -r system/configin ${release_filter_dir}/system
rm -f ${release_filter_dir}/system/Config.in.lang*

# copy chip configs
mkdir -p ${release_filter_dir}/system/chip
for each_chip in $CONFIG_CHIP_LIST
do
	cp -R system/chip/${each_chip} ${release_filter_dir}/system/chip
done

# copy projects of the family which will be released
mkdir -p ${release_filter_dir}/projects
cp -R projects/${FAMILY_NAME} ${release_filter_dir}/projects

# remove protect config
#rm -f ${release_filter_dir}/system/configin/Config.in.protect
#cp system/configin/Makefile ${release_filter_dir}/system/configin

# copy solution Config.in and Makefile
cp Config.in ${release_filter_dir}
cp Makefile ${release_filter_dir}
cp README ${release_filter_dir}

# copy app source code , theme template, scripts, tools
cp -R app ${release_filter_dir}
cp -R tools ${release_filter_dir}
# copy preconfig file
mkdir -p ${release_filter_dir}/configs
if [ -e configs/${FAMILY_NAME}_defconfig ];then
	cp configs/${FAMILY_NAME}_defconfig ${release_filter_dir}/configs
fi
#cp -R theme ${release_filter_dir}
mkdir -p ${release_filter_dir}/theme
#get theme name from rules config file: $theme_config_file
theme_config_file=scripts/rule_theme.conf
get_theme_list=`cat $theme_config_file | grep -v "^#" | awk -F":" -v family=$FAMILY_NAME -v os=$CONFIG_OS_NAME 'BEGIN{str="";}{if(match($1,family) > 0 && match($2,os) > 0) str=str" "$3;} END{print str}' `
echo "Get Theme List:" $get_theme_list
get_theme_list=`echo $get_theme_list | sed 's/ /\n/g' | sort | uniq`

for each_theme in $get_theme_list
do 
	cp -R theme/$each_theme ${release_filter_dir}/theme
done

cp -R scripts ${release_filter_dir}
RELEASE_SCRIPT_FILE=${release_filter_dir}/scripts/release_projects.sh
if [ -e ${RELEASE_SCRIPT_FILE} ];then
	rm -f ${RELEASE_SCRIPT_FILE}
fi

touch $RELEASE_SCRIPT_FILE
chmod +x $RELEASE_SCRIPT_FILE

echo "#!/bin/bash" > $RELEASE_SCRIPT_FILE
echo "echo \"Input Family Name: \" \$1" >> $RELEASE_SCRIPT_FILE
echo "echo -e \"\033[031mThis is the release project, you cannot release any more.\033[0m\"" >> $RELEASE_SCRIPT_FILE
echo "exit 0" >> $RELEASE_SCRIPT_FILE
echo "" >> $RELEASE_SCRIPT_FILE

###############################################################################
# create tar package
###############################################################################
if [ -e .svn ];then
	svn_version=$(svn info | sed -n '5p' | awk -F ":" '{print $2}')
fi
if [[ $svn_version = "" ]];then
        echo "Warning : Cannot find svn version number"
        svn_version="0000"
fi
svn_version=$(echo $svn_version | sed  -e 's/^[ \t]*//' -e 's/[ \t]*$//')

# create version file
version_file=$config_proj_path/$FAMILY_NAME/version
touch $version_file
echo "SVN_VERSION=$svn_version" > $version_file

build_date=$(date +%Y%m%d)
#FAMILY_NAME=$(echo $FAMILY_NAME | tr [:lower:] [:upper:])
TARBALL_NAME=$(echo ${FAMILY_NAME}_svn_${svn_version}_date_${build_date}.tar.bz2)

cd $config_release_path

find ./ -name .svn | xargs rm -rf
tar cjf  ${TARBALL_NAME} solution

echo "Create Package :" ${TARBALL_NAME}

