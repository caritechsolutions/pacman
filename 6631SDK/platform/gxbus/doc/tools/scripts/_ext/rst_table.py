
from __future__ import unicode_literals
import os
import sys
import re
import pickle
from sphinx import addnodes
from docutils import nodes

__version__ = "1.0.0"

def deal_ref(target, docname, app):
    ref_text = re.sub('\\\\ref *', '', target)
    if "struct_filename_dic" in dir(app) and\
            ref_text in app.struct_filename_dic:
        ref_path = app.struct_filename_dic[ref_text][6:-4]
    else:
        return 0
    refnode = addnodes.pending_xref('',
            refdoc = docname, #当前文件
            refdomain='std',
            refexplicit=False,
            reftype='doc',
            reftarget= ref_path,
            )
    #inner_node = nodes.inline("", ref_text, classes=["xref", "myst"])
    inner_node = nodes.inline("", ref_text, classes=["xref", "std", "std-doc"])
    refnode.append(inner_node)
    return refnode

def deal_markdown_ref(target, docname):
    ref_text = re.findall('\[.*\]' ,target)[0][1:-1] # [链接名称]
    ref_path = re.findall('\(.*#.*\)' ,target)[0][1:-1]  #  (md#title)
    refnode = addnodes.pending_xref('',
            refdoc = docname, #当前文件
            #refdomain='std',
            #refexplicit=False,
            refdomain=True,
            refexplicit=True,
            reftype='myst',
            reftarget= ref_path,
            )
    inner_node = nodes.inline("", ref_text, classes=["xref", "myst"])
    refnode.append(inner_node)
    return refnode

def deal_txt(data):
    return nodes.Text(data, data)

def ref_node(node, docname, app):
    for i in range(len(node.children)):
        child = node.children[i]
        data = child.rawsource
        sections = re.split('(\[.*?\]\(.*?#.*?\)|\\\\ref\s+\S+)', data)
        if len(sections) > 1:
            child.pop(0)
            for section in sections:
                if re.findall('\[.*?\]\(.*?#.*?\)', section):
                    child.append(deal_markdown_ref(section, docname))
                elif re.findall('\\\\ref\s+\S+', section):
                    ret = deal_ref(section, docname, app)
                    if ret != 0:
                        child.append(ret)
                    else:
                        child.append(deal_txt(section))
                else:
                    child.append(deal_txt(section))
        elif len(child.children) != 0:
            ref_node(child, docname, app)

def get_rstTable(rst_file):
    rst_doctree = rst_file[:-3] + "doctree"
    with open(rst_doctree, 'rb') as ft:
        t_doctree = pickle.load(ft)
        table = t_doctree.children[0]
    return table

def show_node(node, path):
    for i in range(len(node.children)):
        child = node.children[i]
        text = child.rawsource
        if text.find('gxtableref') >= 0:
            rstTable_file = re.findall('0000gxtableref.*rst', text)[0]
            rstTable_file = path + '/' + rstTable_file
            table_doctree = get_rstTable(rstTable_file)
            node.pop(i)
            node.insert(i,table_doctree)
        elif len(child.children) != 0:
            show_node(child, path)

def doctree_read(app, doctree):
    file_source = doctree.document['source']
    if os.path.basename(file_source).find('0000gxtableref') >= 0:
        file_path = file_source.split('/_output/')[1]
        doc_dir = os.path.dirname(file_path)
        md_file = '_'.join(os.path.basename(file_path).split('_')[1:-1])
        docname = os.path.join(doc_dir, md_file)
        ref_node(doctree, docname, app)
    else:
        doctree_path = '_build/doctrees/' + os.path.dirname(file_source.split('/_output/')[1])
        show_node(doctree, doctree_path)

def setup(app):
    app.connect("doctree-read", doctree_read)
    return {
        'version': __version__,
    }
