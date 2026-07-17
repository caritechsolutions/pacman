#!/usr/bin/env python3

import os
import sys
import pdfkit
sys.path.append('../../')
import conf

import PyPDF2
from PIL import Image

def get_image_size(image_path):
    with Image.open(image_path) as img:
        return img.size

def get_pdf_dimensions(pdf_file):
    with open(pdf_file, 'rb') as file:
        pdf_reader = PyPDF2.PdfReader(file)
        page = pdf_reader.pages[0]  # 获取第一页，可以更改页码以获取其他页面的尺寸
        width = page.mediabox.width
        height = page.mediabox.height
        return width, height

def get_coeffi(w, h):
    print(500/w * h)
    if 500/w * h >  700: #宽度放大时，高度是否超范围
        h_coeffi = 700 /h
        print(h_coeffi)
        if h_coeffi  > 2:
            h_coeffi = 1
        coeffi = h_coeffi*w / 500
    else:
        #coeffi = 460/w
        coeffi = 1
    return str(round(coeffi, 1))

#def get_coeffi(w, h):
#    x = 500
#    y = 700
#    if w < x and h < y: #图片在页面范围内，不做缩放
#        coeffi = w/x
#    elif h/w >= y/x: #图片超过页面范围，且更窄，以h进行缩放
#        h_coeffi = y/h
#        coeffi = w*h_coeffi/x
#    elif h/w < y/x: #图片超过页面范围，且更宽，以w进行缩放
#        coeffi = x/w
#    return str(round(coeffi, 1))

#def get_coeffi(w, h):
#    x = 500 #pdf有效显示宽度, A4大小
#    y = 700 #pdf有效显示高度, A4大小
#
#    print(">>>>>>>>>>>>", w, h, w/h)
#    print(w/h >= 1.2)
#    if h/w >= y/x: #图片较窄，以h进行缩放
#        if h<y:
#            # h<pdf高度，无需缩放
#            coeffi = w/x
#        else:
#            # h>pdf高度，将h缩小至y
#            h_coeffi = y/h
#            coeffi = w*h_coeffi/x
#    elif h/w < y/x: #图片较宽，以w进行缩放
#        coeffi = 1
#        #if w<x:
#        #    # w<pdf宽度
#        #    if w/h >= 1.2:
#        #        # 图片较扁，拉长至x
#        #        coeffi = 1
#        #    else:
#        #        # 图片较方，不缩放
#        #        coeffi = w/x
#        #else:
#        #    # w>pdf宽度, 将w缩小至x
#        #    coeffi = 1
#    print(str(round(coeffi, 1)))
#    return str(round(coeffi, 1))

def main():
    in_file = sys.argv[1]
    new_file = "new_file.tex"
    pdfkit_options = {
        'encoding':'UTF-8'
    	}

    def check_pdf_tex(pdf_tex):
        new_pdf_tex = 'new_' + pdf_tex
        ft = open(pdf_tex, 'r')
        fn = open(new_pdf_tex, 'w')
        while 1:
            line = ft.readline()
            if line.find('_') >= 0 and line.find('\includegraphics') <= 0:
                fn.write(line.replace('_', '\\_').replace('&', '\\&'))
            elif line.find('&') >= 0 :
                fn.write(line.replace('&', '\\&'))
            else:
                fn.write(line)
            if not line:
                break
        ft.close()
        fn.close()
        mv_cmd = 'mv ' + new_pdf_tex + ' ' + pdf_tex
        os.system(mv_cmd)
    
    fi = open(in_file, 'r')
    fo = open(new_file, 'w')
    table_start = 0
    longtable_start = 0
    longtable_tmp = 0
    title_start = 0
    table_width = ''
    get_width = 0
    chapter_name = ''
    first_chapter = 1
    is_no_number_chapter = 0
    set_page_header = 0
    while 1:
        line = fi.readline()
        #if line.find('&') >= 0 :
        #    fo.write(line.replace('&', '\\&'))
        if line.find('\\sphinxattablestart') >= 0:
            table_start = 1
        if line.find('\\sphinxattableend') >= 0:
            table_start = 0
        if line.startswith('\\sphinxincludegraphics') and \
                line.find('.svg') >= 0 and line.find('.drawio') < 0:
            # SVG图片，不是drawio
            reg_name = line.split('{{')[1].split('}')[0]
            if not os.path.exists(reg_name + '.svg'):
                continue
            svg_cmd = "inkscape -D -z --file=" + reg_name + ".svg --export-pdf=" + \
                    reg_name + ".pdf --export-latex"
            print(reg_name)
            os.system(svg_cmd)
            width, height = get_pdf_dimensions(reg_name + '.pdf')
            coeffi = get_coeffi(width, height)
            check_pdf_tex(reg_name + '.pdf_tex')
    
            ##msg = '\\FloatBarrier\n'
            #msg += '\\begin{figure}[htbp]\n\\centering\n\\def\\svgwidth{' + coeffi + '\\columnwidth}\n'
            #msg += '\\begin{figure}[h]\n\\centering\n\\def\\svgwidth{' + coeffi + '\\columnwidth}\n'
            msg = '\\begin{figure}[H]\n\\centering\n\\def\\svgwidth{' + coeffi + '\\columnwidth}\n'
            msg += '\\footnotesize\n'
            msg += '\\input{' + reg_name + '.pdf_tex}\n'
            msg += '\\caption{' + reg_name.replace("_", "\\_") + '}\n'
            msg += '\\end{figure}\n'
            ##msg += '\\FloatBarrier\n'
            fo.write(msg)
            print(line, end = '')
        elif line.find('\\sphinxincludegraphics') >= 0 and \
                line.find('.svg') >= 0 and line.find('.drawio') >= 0:
            # SVG图片，是drawio
            svg_path = line.split('sphinxincludegraphics')[1].split('{{')[1].split('}')[0]
            svg_name = os.path.basename(svg_path)
            new_svg_name = svg_name.replace('.drawio', '')
            if not os.path.exists(svg_name + '.svg'):
                continue
            os.system("cp " + svg_name + '.svg ' + new_svg_name + '.svg')
            drawio_to_pdf_cmd = "xvfb-run -a -s '-screen 0 1280x1024x24' drawio -x --width 500 -f pdf " + new_svg_name +'.svg'
            os.system(drawio_to_pdf_cmd)
            os.system('pdfcrop "'  + new_svg_name + '.pdf"')
            os.system('mv "'  + new_svg_name + '-crop.pdf" ' + '"'  + new_svg_name + '.pdf"')

            width, height = get_pdf_dimensions(new_svg_name + '.pdf')
            coeffi = get_coeffi(width, height)
    
            ##msg = '\\FloatBarrier\n'
            #msg += '\\begin{figure}[htbp]\n\\centering\n'
            #msg += '\\begin{figure}[h]\n\\centering\n'
            msg = '\\begin{figure}[H]\n\\centering\n'
            msg += '\\footnotesize\n'
            msg += '\\includegraphics[width=' + coeffi + '\\textwidth]{' + new_svg_name + '.pdf}\n'
            msg += '\\caption{' + new_svg_name.replace("_", "\\_") + '}\n'
            msg += '\\end{figure}\n'
            ##msg += '\\FloatBarrier\n'
            fo.write(msg)
            print(line, end = '')
        elif line.startswith('\\sphinxincludegraphics'):
            # 其他图片，如png等
            image_path = line.split('sphinxincludegraphics')[1].split('{{')[1].split('}')[0]
            image_type = line.split('sphinxincludegraphics')[1].strip('{}\n').split('.')[-1]
            image_name = os.path.basename(image_path)
            image_msg = line.split('sphinxincludegraphics')[1]

            width, height = get_image_size(image_path + '.' + image_type)
            coeffi = get_coeffi(width, height)

            ##msg = '\\FloatBarrier\n'
            #msg += '\\begin{figure}[htbp]\n\\centering\n'
            #msg += '\\begin{figure}[h]\n\\centering\n'
            msg = '\\begin{figure}[H]\n\\centering\n'
            msg += '\\footnotesize\n'
            #msg += '\\includegraphics[width=' + coeffi + '\\textwidth]{' + new_svg_name + '.pdf}\n'
            msg += '\\includegraphics[width=' + coeffi + '\\textwidth]' + image_msg + '\n'
            #msg += '\\includegraphics' + image_msg
            msg += '\\caption{' + image_name.replace("_", "\\_") + '}\n'
            msg += '\\end{figure}\n'
            ##msg += '\\FloatBarrier\n'
            fo.write(msg)
        elif line.startswith('\\sphinxstylestrong'):
            fo.write(line)
            fo.write('\\\\' +  '\n')
        elif line.find('0000gxhtmltableref') >= 0:
            print(line)
            html_file = line.strip('\n').replace('\\','')
            print(html_file)

            pdf_file = os.path.basename(html_file)[:-5] + '.pdf'
            pdfkit.from_file(html_file, pdf_file, options = pdfkit_options)
            #os.system('pdfcrop ' + pdf_file)
            #crop_pdf = pdf_file[:-4] + '-crop.pdf'
            #os.system('mv ' + crop_pdf + ' ' + pdf_file)

            width, height = get_pdf_dimensions(pdf_file)
            coeffi = get_coeffi(width, height)
            print(">>>>>>>..", coeffi)
            print(">>>>>>>..", width, height)

            msg = '\\begin{center}\n'
            #msg += '\\includegraphics{' + pdf_file + '}\n'
            msg += '\\includegraphics[width=' + coeffi + '\\textwidth]{' + pdf_file + '}\n'
            msg += '\\end{center}\n'
            fo.write(msg)
        #elif line.find('gxpdfref') >= 0 and line.find('sphinxhref') >= 0:
        #    line = line.replace('\\sphinxhref', '')
        #    section = line.split('gxpdfref')
        #    new_line = ''
        #    for sec in section:
        #        if sec.find('}{') >= 0:
        #            tmp = sec.split('}')
        #            link_path = tmp[0]
        #            link_name = tmp[1]
        #            txt = tmp[2] 
        #            new_line +=  "\hyperref[\detokenize{" + link_path + "}]{\sphinxcrossref{\DUrole{std,std-doc}" + link_name + "}}}}" + txt
        #        else:
        #            new_line += sec
        #    msg = new_line
        #    fo.write(msg)
        elif line.startswith('\\usepackage{xeCJK}'):
            msg = line
            msg += '\\usepackage{ctex}\n'
            msg += '\\usepackage{fontspec}\n'
            msg += '\\usepackage[numbered]{bookmark}\n'
            msg += '\\usepackage[section]{placeins}\n'
            msg += '\\usepackage{setspace}\n'
            msg += '\\usepackage{tabularx}\n'
            msg += '\\usepackage{xltabular}\n'
            msg += '\\usepackage{float}\n'
            #msg += '\\newlength{\customtablewidth}'
            #msg += '\\setlength{\customtablewidth}{1\linewidth}'
            msg += '\\usepackage{array}\n'
            msg += '\\usepackage{seqsplit}\n'
            msg += '\\newcolumntype{Y}[1]{>{\\hsize=#1\\hsize\\seqsplit}X}\n'
            msg += '\\usepackage{makecell}\n'
            msg += '\\usepackage{longtable}\n'
            msg += '\\usepackage[table]{xcolor}\n'
            msg += '\\usepackage{graphicx}\n'
            msg += '\\renewcommand\\arraystretch{1.5}\n'
            msg += '\\renewcommand\\baselinestretch{1.5}\n'
            fo.write(msg)
        #elif line.startswith('\\chapter{前言}'):
        #    print(conf.no_numbered_chapter)
        #    sys.exit()
        #    msg = '\\chapter*{前言}\n'
        #    msg += '\\addcontentsline{toc}{chapter}{前言}\n'
        #    fo.write(msg)
        elif line.startswith('\\chapter{'):
            chapter_name = line.split('chapter{')[1].split('}')[0]
            if chapter_name in conf.no_numbered_chapter : 
                is_no_number_chapter = 1
                msg = '\\chapter*{' + chapter_name + '}\n'
                msg += '\\addcontentsline{toc}{chapter}{' + chapter_name + '}\n'
                fo.write(msg)
            else:
                is_no_number_chapter = 0
                if first_chapter:
                    msg = '\\mtcaddchapter\n' + line
                    first_chapter = 0
                else:
                    msg = line
                msg += '\\minitoc\n'
                msg += '\\minilof\n'
                msg += '\\minilot\n'
                fo.write(msg)
        elif line.startswith('\\section'):
            if chapter_name in conf.no_numbered_chapter : 
                section_name = line.split('section{')[1].split('}')[0]
                msg = '\\section*{' + section_name + '}\n'
                msg += '\\addcontentsline{toc}{section}{' + section_name + '}\n'
                fo.write(msg)
            else:
                msg = line
                fo.write(msg)
        elif line.startswith('\\subsection'):
            if chapter_name in conf.no_numbered_chapter : 
                section_name = line.split('subsection{')[1].split('}')[0]
                msg = '\\subsection*{' + section_name + '}\n'
                msg += '\\addcontentsline{toc}{subsection}{' + section_name + '}\n'
                fo.write(msg)
            else:
                msg = line
                fo.write(msg)
        elif line.startswith('\\subsubsection'):
            if chapter_name in conf.no_numbered_chapter : 
                section_name = line.split('subsubsection{')[1].split('}')[0]
                msg = '\\subsubsection*{' + section_name + '}\n'
                msg += '\\addcontentsline{toc}{subsubsection}{' + section_name + '}\n'
                fo.write(msg)
            else:
                msg = line
                fo.write(msg)
        elif line.startswith('\\subsubsubsection'):
            if chapter_name in conf.no_numbered_chapter : 
                section_name = line.split('subsubsubsection{')[1].split('}')[0]
                msg = '\\subsubsubsection*{' + section_name + '}\n'
                msg += '\\addcontentsline{toc}{subsubsubsection}{' + section_name + '}\n'
                fo.write(msg)
            else:
                msg = line
                fo.write(msg)
        elif not set_page_header and line.startswith('\\phantomsection\\label'):
            set_page_header = 1
            msg = line
            msg += '\\renewcommand{\\chaptermark}[1]{\\markboth{\\thechapter.\\ #1}{}}\n'
            msg += '\\newfontfamily\\myfont{FreeSans}\n'
            msg += '\\newcommand{\\mytext}[1]{{\\myfont #1}}\n'
            msg += '\\lhead{\\normalsize\\textbf{\\heiti\\includegraphics[width=1cm]{icon.png} 芯片测试介绍}}\n'
            msg += '\\rhead{\\normalsize\\textbf{\\mytext{\\heiti\\leftmark}}}\n'
            #msg += '\\lhead{\includegraphics[width=1cm]{icon.png}}\n'
            fo.write(msg)
        elif line.find('\\sphinxtablecontinued') >= 0 and line.find('续上页') >= 0:
            msg = '{\\makebox[0pt]{\\sphinxtablecontinued{续上页}}}\\\\'
            fo.write(msg)
        elif line.startswith('\\title{'):
            title_start = 1
        elif title_start == 1:
            if line.startswith('\\sphinxtableofcontents'):
                title_start = 0
                msg = '\\counterwithin{figure}{chapter}\n'
                msg += '\\counterwithin{table}{chapter}\n'
                msg += '\\renewcommand{\\thefigure}{图\\thechapter-\\arabic{figure}}\n'
                msg += '\\renewcommand{\\thetable}{表\\thechapter-\\arabic{table}}\n'
                msg += '\\pdfbookmark[1]{Contents}{toc}\n'
                msg += '\\dominitoc\n'
                msg += '\\dominilof\n'
                msg += '\\dominilot\n'
                msg += '\\tableofcontents\n'
                msg += '\\listoffigures\n'
                msg += '\\listoftables\n'
                fo.write(msg)
            else:
                continue
        elif table_start == 1 and line.startswith('\\begin{tabulary}'):
            if get_width:
                msg = "\\begin{xltabular}{\\linewidth}{|"
                print('table_start>>>', line)
                for t in range(len(line.strip('\n').split('[t]')[-1].strip('{}|').split('|'))):
                    print(line.strip('\n').split('[t]')[-1].strip('{}|').split('|'))
                    print(table_width)
                    msg += 'Y{' + table_width[t] + '}|'
                fo.write(msg + '}\n')
                get_width = 0
                table_width = ''
            else:
                msg = "\\begin{xltabular}{\\linewidth}"
                #msg = "\\begin{xltabular}{\\customtablewidth}"
                msg += line.split('[t]')[-1].replace('T', 'l')
                #msg += line.split('[t]')[-1].replace('T', 'X')
                msg = msg.replace('l|}', 'X|}')
                fo.write(msg)
        elif table_start == 1 and line.startswith('\\end{tabulary}'):
            if is_no_number_chapter:
                msg = "\\end{xltabular}"
            else:
                msg = "\\caption{ }\n"
                msg += "\\end{xltabular}"
            fo.write(msg)
        #elif line.startswith('\\begin{savenotes}\\sphinxatlongtablestart\\begin{longtable}'):
        elif line.startswith('\\begin{longtable}'):
            longtable_start = 1
            if get_width:
                msg = "\\begin{xltabular}{\\linewidth}{|"
                print('table_start>>>', line)
                for t in range(len(line.strip('\n').split('[c]')[-1].strip('{}|').split('|'))):
                    msg += 'Y{' + table_width[t] + '}|'
                fo.write(msg + '}\n')
                get_width = 0
                table_width = ''
            else:
                msg = line.split('\\begin{longtable}')[0] + '\n'
                msg += "\\begin{xltabular}{\\linewidth}"
                msg += line.split('[c]')[-1]
                msg = msg.replace('l|}', 'X|}')
                fo.write(msg)
        elif line.startswith('\\sphinxtableatstartofbodyhook') and longtable_start == 1:
            fo.write(line)
            table_start = 1
        #elif line.startswith('\\end{longtable}\\sphinxatlongtableend\\end{savenotes}'):
        elif line.startswith('\\end{longtable}'):
            table_start = 0
            longtable_start = 0
            if is_no_number_chapter:
                msg = "\\end{xltabular}"
            else:
                msg = "\\caption{ }\n"
                msg += "\\end{xltabular}"
            msg += line.split('{longtable}')[-1]
            fo.write(msg)
        elif table_start == 1 and line.find(";") >= 0:
            #msg = '\\makecell[l]{' + line.replace(';', '\\\\') + '}'
            msg = line.strip(';').replace(';', '\\newline ')
            if table_start == 1 and not line.startswith(('\\', '&', '\n')) \
                    and line.find('\\sphinx') < 0 and line.find('\\DUrole') < 0:
                msg = '\\seqsplit{' + msg[:-1].strip('   ').replace(' ', '\\ \\ ') + '}\n'
            fo.write(msg)
        elif table_start == 1 and not line.startswith(('\\', '&', '\n')) \
                and line.find('\\sphinx') < 0 and line.find('\\DUrole') < 0:
            msg = '\\seqsplit{' + line[:-1].strip('   ').replace(' ', '\\ \\ ') + '}\n'
            fo.write(msg)
        elif table_start == 1 and line == '\n':
            msg = '\\quad\n'
            fo.write(msg)
        elif line.startswith('gxtable\\_col\\_width\\_percent'):
            table_width = line[:-4].replace('gxtable\\_col\\_width\\_percent:{[}', '').split(',')
            get_width = 1
            print(line, table_width)
        elif line.startswith('\\multicolumn'):
            file_position = fi.tell()
            next_line = fi.readline()
            if next_line.find('\\sphinxtablecontinued') >= 0:
                next_line = fi.readline()
            else:
                fo.write(line)
                fi.seek(file_position)
        elif line.find('\&nbsp;') >= 0  and table_start == 1:
            fo.write(line.replace('\&nbsp;', ''))
        elif line.find('\&nbsp;') >= 0  and table_start == 1:
            fo.write(line.replace('\&nbsp;', ''))
        elif line.strip('\n').endswith('\\end{titlepage}'):
            copydata = '''
\\textbf{\large 版权声明}

本文档版权归属杭州杭州国芯微股份有限公司。非经本公司书面许可，任何单位和个人不得擅自摘抄、复制或转译本文档内容的部分或全部，并不得以任何形式进行转载、传播。

\copyright{}  杭州杭州国芯微股份有限公司2023。保留一切权利。

\\textbf{\large 商标声明}

本文档提及的 \includegraphics[height=1cm]{icon.png}、\includegraphics[height=0.6cm]{text.png} 以及与之相关的商标均为杭州杭州国芯微股份有限公司的商标。非经杭州杭州国芯微股份有限公司书面许可，任何单位和个人不得擅自使用此类商标。

本文档提及的第三方所有商标或注册商标，由各自的所有人拥有。

\\textbf{\large 注意事项}

您购买的产品、服务或特性等应受杭州杭州国芯微股份有限公司商业合同和条款的约束，本文档中描述的全部或部分产品、服务或特性可能不在您的购买或使用范围之内。除非合同另有约定，杭州杭州国芯微股份有限公司对本文档内容不做任何明示或默示的声明或保证。

由于产品版本升级或其他原因，本文档内容会不定期进行更新，如有内容更新，恕不另行通知。除非另有约定，本文档仅作为使用指导，本文档中的所有陈述、信息和建议均不构成任何明示或暗示的担保。


\\vspace*{\\fill}

\\textbf{\Large 杭州杭州国芯微股份有限公司}

\\textbf{中国\hspace{2em}杭州总部}

地址：浙江省 杭州市 西湖区 文三路90号东部软件园创新大厦A座五楼\\\\
邮编：310012\\\\
网址：www.nationalchip.com\\\\
总机：+86-571-88156088\\\\
传真：+86-571-88156083\\\\

\\thispagestyle{empty}
\\addtocounter{page}{-1}
\\clearpage
'''
            fo.write(line)
            fo.write(copydata)

        elif line.find('‣') >= 0:
            if table_start == 1:
                fo.write(line.replace('‣', '\\newline'))
            else:
                fo.write(line.replace('‣', '\\\\'))
        else:
            fo.write(line)
        if not line:
            break
    fi.close()
    fo.close()
    mv_cmd = 'mv "' + new_file + '" "' + in_file + '"'
    print(mv_cmd)
    os.system(mv_cmd)

if __name__ == "__main__":
    main()
