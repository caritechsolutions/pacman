
from __future__ import unicode_literals
import os
import sys
import re
import conf
import rstinfo
import shutil

__version__ = "1.0.0"


TYPE_CLASS = 1
TYPE_UNION = 2
TYPE_FUNCTION = 3
TYPE_ENUM = 4
TYPE_DEFINE = 5
TYPE_TYPEDEFINE = 6
TYPE_VARIABLE = 7
struct_filename_dic = {}
en_title = {
        '结构体':'Struct',
        '枚举':'Enum',
        '类型定义':'Typedef',
        '宏定义':'Define',
        '函数列表':'Function List'
        }

def getdatastructbyfilename(filename, dtype):
    if not os.path.exists(filename):
        return None
    dname = None
    with open(filename,'r') as fd:
        lines = fd.readlines()
        for line in lines:
            #line = line.strip()
            if not line.strip():
                continue
            dname = re.sub(dtype + ' *(.+)', '\g<1>', line[:-1])
            if dname != line[:-1]:
                break
    return dname

def rst_file_process(rstfile, rstrootdir):
    info_dic = {'struct':[], 'function':[], 'union':[], 'enum':[], 'define':[], 'typedef':[], 'variable':[]}
    if not os.path.exists(rstfile):
        return None
    lines = []
    with open(rstfile,'r') as fd:
        lines = fd.readlines()
        #print(lines)
    datatype = 0
    for line in lines:
        if not line.strip():
            continue
        line = line.strip()
        if 'Classes' == line:
            datatype = TYPE_CLASS
            continue
        if 'Functions' == line:
            datatype = TYPE_FUNCTION
            continue
        if 'Unions' == line:
            datatype = TYPE_UNION
            continue
        if 'Enums' == line:
            datatype = TYPE_ENUM
            continue
        if 'Defines' == line:
            datatype = TYPE_DEFINE
            continue
        if 'Typedefs' == line:
            datatype = TYPE_TYPEDEFINE
            continue
        if 'Variables' == line:
            datatype = TYPE_VARIABLE
            continue
        if '- :ref:`' not in line:
            continue
        rstfilename = re.sub('- :ref:`exhale_(.+)`', '\g<1>.rst', line)
        rstfilename = os.path.join(rstrootdir, rstfilename)
        dname = 'unknow'
        #print(rstfilename, datatype)
        if datatype == TYPE_CLASS:
            rstfilename = re.sub('struct_struct', 'struct', rstfilename)
            dname = getdatastructbyfilename(rstfilename, 'Struct')
            info_dic['struct'].append({dname:rstfilename})
        if datatype == TYPE_FUNCTION:
            dname = getdatastructbyfilename(rstfilename, 'Function')
            info_dic['function'].append({dname:rstfilename})
        if datatype == TYPE_UNION:
            rstfilename = re.sub('union_union', 'union', rstfilename)
            dname = getdatastructbyfilename(rstfilename, 'Union')
            info_dic['union'].append({dname:rstfilename})
        if datatype == TYPE_ENUM:
            dname = getdatastructbyfilename(rstfilename, 'Enum')
            info_dic['enum'].append({dname:rstfilename})
        if datatype == TYPE_DEFINE:
            dname = getdatastructbyfilename(rstfilename, 'Define')
            info_dic['define'].append({dname:rstfilename})
        if datatype == TYPE_TYPEDEFINE:
            dname = getdatastructbyfilename(rstfilename, 'Typedef')
            info_dic['typedef'].append({dname:rstfilename})
        if datatype == TYPE_VARIABLE:
            dname = getdatastructbyfilename(rstfilename, 'Variable')
            info_dic['variable'].append({dname:rstfilename})
        struct_filename_dic[dname] = rstfilename
    return info_dic

def getheadfile_datastructinfo(rootrstdir):
    rst_files = os.listdir(rootrstdir)
    rst_h_files_dic = {}
    for filename in rst_files:
        if 'file_' in filename and 'program_listing_file__' not in filename  and 'file_view_hierarchy' not in filename:
            #rst_h_files.append(filename)
            rstpath = os.path.join(rootrstdir, filename)
            info_dic = rst_file_process(rstpath, rootrstdir)
            rst_h_files_dic[filename] = info_dic
    return rst_h_files_dic

def genmoduleindex(project_info,  data_struct_dic):
    curdir = os.getcwd()
    modules_dic = {}
    for key, values in project_info.items():
        modules_dic[key] = {}
        for value in values:
            value = os.path.relpath(value, curdir)
            file_rst = re.sub('/', '_', value)
            file_rst = 'file__output_' + file_rst + '.rst'
            if file_rst in data_struct_dic.keys():
                modules_dic[key][value] = data_struct_dic[file_rst]
    return modules_dic

def getindexmdbymoduledic(modules_dic, rootrstdir):
    typeslist = ['class','function','enum','union','define','typedef','variable']
    modules = {}
    for modulename, moduledic in modules_dic.items():
        #modulerstfilename = module
        modules[modulename] = {'struct':'# Structs List\n',\
                'function':'# Functions List\n','enum':'Enums List\n', \
                'union':'#Unions List\n','define':'# Defines List\n', \
                'typedef':'Typedefs List\n', 'variable':'Variables List\n'}
        for hfile, data_info_dic in moduledic.items():
            for dtype, datalist in data_info_dic.items():
                if not datalist:
                    continue
                for data in datalist:
                    for key, value in data.items():
                        mddata = '  * [' + key + '](' + os.path.basename(value) + ')\n'
                        modules[modulename][dtype] = modules[modulename][dtype] + mddata
    for modulename, data_dic in modules.items():
        for dtype, data in data_dic.items():
            mdfilename = modulename + '_' + dtype + '.md'
            mdfilename = os.path.join(rootrstdir, mdfilename)
            with open(mdfilename, 'w') as fd:
                fd.write(data)
            if conf.one_page_one_api == 'yes':
                genrst_one_api(os.path.abspath(mdfilename))
            else:
                genrst_all_api(os.path.abspath(mdfilename))

def genrst_all_api(absfilename):
    print('genrst:', absfilename)
    md_file = absfilename
    tmp_name = absfilename[:-3]
    rst_file = tmp_name + '.rst'
    fd = open(md_file, 'r')
    fr = open(rst_file, 'w')
    if tmp_name.endswith('struct'):
        title = '结构体'
    elif tmp_name.endswith('enum'):
        title = '枚举'
    elif tmp_name.endswith('typedef'):
        title = '类型定义'
    elif tmp_name.endswith('define'):
        title = '宏定义'
    elif tmp_name.endswith('function'):
        title = '函数列表'
    else:
        fr.close()
        fd.close()
        return
    if conf.language == 'en':
        title = en_title[title]
    fr.write(title + '\n#####################\n\n')
    module_part = os.path.basename(absfilename).split('_')[0]
    file_dir = os.path.dirname(absfilename)
    lib_dir = os.path.join(file_dir, 'lib')
    if not os.path.exists(lib_dir):
        os.mkdir(lib_dir)
    while True:
        line = fd.readline()
        if not line:
            break
        line = line.strip('\n')
        if line.endswith('.rst)'):
            line_name = line.split('](')[0].split('[')[1]#函数名
            line_file = line.split('](')[1].strip(')')#对应的rst文件
            struct_filename_dic[line_name] = 'exhalerst/lib/' + line_file
            fr.write('.. include:: ' + 'lib/' + line_file + '\n')
            old_path = os.path.join(file_dir, line_file)
            new_path = os.path.join(lib_dir, line_file)
            os.system("sed -i 's/Function Documentation//g' " + old_path)
            os.system("sed -i 's/Typedef Documentation//g' " + old_path)
            os.system("sed -i 's/Define Documentation//g' " + old_path)
            os.system("sed -i 's/Enum Documentation//g' " + old_path)
            os.system("sed -i 's/Struct Documentation//g' " + old_path)
            shutil.copy(old_path, new_path)
    fd.close()
    fr.close()
    data_rst = os.path.join(file_dir, module_part + '_annotated.rst')
    if conf.language == 'en':
        with open(data_rst, 'w') as fa:
            fa.write('Data Structure\n===================\n\n')
            fa.write('.. toctree::\n   :titlesonly:\n\n')
            fa.write('   Define<'+ module_part + '_define.rst>\n')
            fa.write('   Typedef<'+ module_part + '_typedef.rst>\n')
            fa.write('   Enum<'+ module_part + '_enum.rst>\n')
            fa.write('   Struct<' + module_part + '_struct.rst>\n')
    else:
        with open(data_rst, 'w') as fa:
            fa.write('数据结构\n===================\n\n')
            fa.write('.. toctree::\n   :titlesonly:\n\n')
            fa.write('   宏定义<'+ module_part + '_define.rst>\n')
            fa.write('   类型定义<'+ module_part + '_typedef.rst>\n')
            fa.write('   枚举<'+ module_part + '_enum.rst>\n')
            fa.write('   结构体<' + module_part + '_struct.rst>\n')

def genrst_one_api(absfilename):
    print('genrst:', absfilename)
    md_file = absfilename
    tmp_name = absfilename[:-3]
    rst_file = tmp_name + '.rst'
    fd = open(md_file, 'r')
    fr = open(rst_file, 'w')
    if tmp_name.endswith('struct'):
        title = '结构体'
    elif tmp_name.endswith('enum'):
        title = '枚举'
    elif tmp_name.endswith('typedef'):
        title = '类型定义'
    elif tmp_name.endswith('define'):
        title = '宏定义'
    elif tmp_name.endswith('function'):
        title = '函数列表'
    else:
        fr.close()
        fd.close()
        return
    if conf.language == 'en':
        title = en_title[title]
    fr.write(title + '\n===================\n\n')
    fr.write('.. toctree::\n   :titlesonly:\n\n')
    module_part = os.path.basename(absfilename).split('_')[0]
    file_dir = os.path.dirname(absfilename)
    lib_dir = os.path.join(file_dir, 'lib')
    if not os.path.exists(lib_dir):
        os.mkdir(lib_dir)
    while True:
        line = fd.readline()
        if not line:
            break
        line = line.strip('\n')
        if line.endswith('.rst)'):
            line_name = line.split('](')[0].split('[')[1]#函数名
            line_file = line.split('](')[1].strip(')')#对应的rst文件
            struct_filename_dic[line_name] = 'exhalerst/lib/' + line_file
            fr.write('   ' + line_name + '<lib/' + line_file + '>\n')
            old_path = os.path.join(file_dir, line_file)
            new_path = os.path.join(lib_dir, line_file)
            shutil.copy(old_path, new_path)
    fd.close()
    fr.close()
    data_rst = os.path.join(file_dir, module_part + '_annotated.rst')
    if conf.language == 'en':
        with open(data_rst, 'w') as fa:
            fa.write('Data Structure\n===================\n\n')
            fa.write('.. toctree::\n   :titlesonly:\n\n')
            fa.write('   Define<'+ module_part + '_define.rst>\n')
            fa.write('   Typedef<'+ module_part + '_typedef.rst>\n')
            fa.write('   Enum<'+ module_part + '_enum.rst>\n')
            fa.write('   Struct<' + module_part + '_struct.rst>\n')
    else:
        with open(data_rst, 'w') as fa:
            fa.write('数据结构\n===================\n\n')
            fa.write('.. toctree::\n   :titlesonly:\n\n')
            fa.write('   宏定义<'+ module_part + '_define.rst>\n')
            fa.write('   类型定义<'+ module_part + '_typedef.rst>\n')
            fa.write('   枚举<'+ module_part + '_enum.rst>\n')
            fa.write('   结构体<' + module_part + '_struct.rst>\n')

def getRelativepath(source_abs_dir, target_abs_path):
    abs_list = re.findall('/[^/]+', source_abs_dir)
    if not abs_list:
        print("     ")
        sys.exit(0)
    dir_str = ''
    list_len = len(abs_list)
    i = 0
    relative_path = ''
    for onedir in abs_list:
        isfind = target_abs_path.find(os.path.join(dir_str, onedir))
        if isfind < 0:
            sfilename = re.sub(dir_str, '', target_abs_path)
            relative_path = os.path.join('../'*(list_len - i), sfilename[1:])
            break
        i = i + 1
        if dir_str == '':
            dir_str = onedir
        else:
            dir_str = os.path.join(dir_str, onedir[1:])
    return relative_path


def onefilerocesss(absfilename, target_abs_path):
    print('processing ' + absfilename)
    if absfilename == None:
        return None

    with open(absfilename, 'r') as fd:
        data = fd.read()
        #data = data.rstrip('\r\n')
        # gxdocref process
        gxdocreflist = re.findall('gxdocref[ \t]+\S+', data, flags = re.ASCII)
        for gxdocref in gxdocreflist:
            _struct = re.sub('gxdocref *', '', gxdocref)
            if _struct in struct_filename_dic.keys():
                sfilename = struct_filename_dic[_struct]
                sfilename = os.path.abspath(sfilename)
                abs_dir = os.path.dirname(absfilename)
                relative_path = getRelativepath(abs_dir, sfilename)
                rep = '[**' + _struct + '**](' + relative_path + ')'
                data = re.sub(gxdocref, rep, data)

        reflist = re.findall("\\\\ref\s+\S+", data, flags = re.ASCII) #为什么需要四个'\'
        for gxdocref in reflist:
            _struct = re.sub('\\\\ref *', '', gxdocref)
            if _struct in struct_filename_dic.keys():
                sfilename = struct_filename_dic[_struct]
                sfilename = os.path.abspath(sfilename)
                abs_dir = os.path.dirname(absfilename)
                relative_path = getRelativepath(abs_dir, sfilename)
                rep = '[**' + _struct + '**](' + relative_path + ')'
                data = re.sub('\\' + gxdocref, rep, data)

        style_list = re.findall('\[\*\*\S+?\*\*\] *\(\S+?\)', data, flags = re.ASCII)
        for style in style_list:
            _struct = re.sub('\[\*\*(\S+)\*\*\].+', '\g<1>', style)
            _struct = re.sub('\\_', '_', _struct)
            if _struct in struct_filename_dic.keys():
                sfilename = struct_filename_dic[_struct]
                sfilename = os.path.abspath(sfilename)
                abs_dir = os.path.dirname(absfilename)
                relative_path = getRelativepath(abs_dir, sfilename)
                rep = '[**' + _struct + '**](' + relative_path + ')'
                #print(style, rep, data.find(style))
                #style = '\[\*\*GxDmxSetSource\*\*\]\(group__autotest_1gafb6123a7202108566823f263e3318f74\.md\)'
                style = re.sub('\[', '\\\[', style)
                style = re.sub('\]', '\\\]', style)
                style = re.sub('\*', '\\\*', style)
                style = re.sub('\(', '\\\(', style)
                style = re.sub('\)', '\\\)', style)
                style = re.sub('\.', '\\\\.', style)
                data = re.sub(style, rep, data)
    with open(absfilename, 'w') as fd:
        fd.write(data)

def mdprocesss(md_dir, rst_dir):
    excluded_dirs = ['_build', 'copyinclude', 'doxyoutput', 'exhalerst', '__pycache__', 'scripts', '_static', '_templates']
    for dirpath,dirnames,filenames in os.walk(md_dir):
        dirnames[:] = [d for d in dirnames if d not in excluded_dirs]

        for filename in filenames:
            if filename.endswith('.md'):
                absfilename = os.path.abspath(os.path.join(dirpath,filename))
                onefilerocesss(absfilename, rst_dir)

def clean_mdfile(rootdir):
    for i in os.listdir(rootdir):
        path = os.path.join(rootdir, i)
        libpath = os.path.join(rootdir, 'lib', i)
        if path.endswith('md'):
            os.system('rm ' + path)
        if os.path.exists(libpath):
            os.system('rm ' + path)

def genmodule(app):
    rootdir = 'exhalerst'
    dic_info = getheadfile_datastructinfo(rootdir)
    module_dic = genmoduleindex(rstinfo.project_info, dic_info)
    getindexmdbymoduledic(module_dic, rootdir)
    rst_dir = ''
    bookdir = './'
    mdprocesss(bookdir, rst_dir)
    app.struct_filename_dic = struct_filename_dic
    clean_mdfile(rootdir)


def setup(app):
    app.setup_extension("exhale")
    app.connect("builder-inited", genmodule)
    return {
        'version': __version__,
    }

if __name__ == "__main__":
    genmodule(1)
