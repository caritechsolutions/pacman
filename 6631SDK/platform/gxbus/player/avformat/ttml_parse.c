#if defined(CONFIG_OPENSOURCE_LIBXML2)
	#include "libxml/parser.h"
	#include "libxml/tree.h"
	#include "libxml/xmlstring.h"
#endif
#include "ttml_parse.h"
#include <stdlib.h>

#if defined(CONFIG_OPENSOURCE_LIBXML2)
	#define TTMLxmlDocPtr  xmlDoc
	#define TTMLXmlNodePtr xmlNodePtr
	#define TTMLxmlFree(ptr)           xmlFree(ptr)
	#define TTMLxmlNodeGetContent xmlNodeGetContent
	#define TTMLxmlGetProp xmlGetProp
	#define TTMLxmlFirstElementChild xmlFirstElementChild
	#define TTMLxmlNextElementSibling xmlNextElementSibling
	#define TTMLxmlAttrPtr xmlAttrPtr
	#define TTMLxmlCopyNode xmlCopyNode
	#define TTMLxmlNewNode xmlNewNode
	#define TTMLxmlFreeDoc xmlFreeDoc
	#define TTMLxmlFreeNode xmlFreeNode
#else
	#include "tree.h"
	#include "parser.h"
	#define TTMLxmlDocPtr  GXxmlDocPtr
	#define TTMLXmlNodePtr GXxmlNodePtr
	#define TTMLxmlFree(ptr)           GXxmlFree(ptr)
	#define TTMLxmlNodeGetContent GXxmlNodeGetContent
	#define TTMLxmlGetProp GXxmlGetProp
	#define TTMLxmlFirstElementChild GXxmlFirstElementChild
	#define TTMLxmlNextElementSibling GXxmlNextElementSibling
	#define TTMLxmlAttrPtr GXxmlAttrPtr
	#define TTMLxmlCopyNode GXxmlCopyNode
	#define TTMLxmlNewNode GXxmlNewNode
	#define TTMLxmlFreeDoc GXxmlFreeDoc
	#define TTMLxmlFreeNode GXxmlFreeNode
	extern GXxmlDocPtr GXxmlParseBuffer(const char *filename, char *buffer);
#endif

#define MAX_BPRINT_READ_SIZE (UINT_MAX - 1)
#define AV_BPRINT_SIZE_UNLIMITED  ((unsigned)-1)
#define XML_START_TAG "<?xml"
#define TTML_END_TAG "</tt>"

static int GxxmlStrcmp(char *str1, const char *str2)
{
	int tmp = 0;

	if (str1 == str2) return(0);
	if (str1 == NULL) return(-1);
	if (str2 == NULL) return(1);
	do {
		tmp = *str1++ - *str2;
		if (tmp != 0) return(tmp);
	} while (*str2++ != 0);
	return 0;
}

static TTMLXmlNodePtr ttml_find_child (TTMLXmlNodePtr parent, const char *name)
{
#if defined(CONFIG_OPENSOURCE_LIBXML2)
	TTMLXmlNodePtr child = parent->children;
#else
	TTMLXmlNodePtr child = parent->childs;
#endif
	while (child && GxxmlStrcmp ((char *)child->name, name) != 0) {
		child = TTMLxmlNextElementSibling(child);
	}
	return child;
}

static void close_ttml_body(TtmlContext *c)
{
	int i = 0;
	ttml_body_element *body;
	if (!c)
		return ;
	for (i = 0; i < c->n_ttml_body; i++) {
		body = c->ttml_body[i];
		if (body) {
			if (body->id) {
				av_free(body->id);
				body->id = NULL;
			}
			if (body->style) {
				av_free(body->style);
				body->style = NULL;
			}
			if (body->region) {
				av_free(body->region);
				body->region = NULL;
			}
			if (body->ttml_id) {
				av_free(body->ttml_id);
				body->ttml_id = NULL;
			}
			if (body->text) {
				av_free(body->text);
				body->text = NULL;
			}
			if (body) {
				av_free(body);
				body = NULL;
			}
		}
	}
	if (c->ttml_body) {
		av_freep(&c->ttml_body);
	}
	c->n_ttml_body = 0;
}

static void close_ttml_loyout(TtmlContext *c)
{
	int i = 0;
	ttml_head_loyout *loyout;
	if (!c)
		return ;
	for (i = 0; i < c->n_head_loyout; i++) {
		loyout = c->head_loyout[i];
		if (loyout) {
			if (loyout->id) {
				av_free(loyout->id);
				loyout->id = NULL;
			}
			if (loyout->origin) {
				av_free(loyout->origin);
				loyout->origin = NULL;
			}
			if (loyout->extent) {
				av_free(loyout->extent);
				loyout->extent = NULL;
			}
			if (loyout->displayAlign) {
				av_free(loyout->displayAlign);
				loyout->displayAlign = NULL;
			}
			if (loyout->textAlign) {
				av_free(loyout->textAlign);
				loyout->textAlign = NULL;
			}
			if (loyout->fontSize) {
				av_free(loyout->fontSize);
				loyout->fontSize = NULL;
			}
			if (loyout->fontFamily) {
				av_free(loyout->fontFamily);
				loyout->fontFamily = NULL;
			}
			if (loyout->overflow) {
				av_free(loyout->overflow);
				loyout->overflow = NULL;
			}
			av_free(loyout);
			loyout = NULL;
		}
	}
	if (c->head_loyout) {
		av_freep(&c->head_loyout);
	}
	c->n_head_loyout = 0;
}

void close_ttml_context(TtmlContext *c)
{
	if (c) {
		if (c->cellResolution) {
			av_free(c->cellResolution);
			c->cellResolution = NULL;
		}
		if (c->pixelAspectRatio) {
			av_free(c->pixelAspectRatio);
			c->pixelAspectRatio = NULL;
		}
		if (c->tickRate) {
			av_free(c->tickRate);
			c->tickRate = NULL;
		}
		if (c->extent) {
			av_free(c->extent);
			c->extent = NULL;
		}
		if (c->space) {
			av_free(c->space);
			c->space = NULL;
		}
		if (c->lang) {
			av_free(c->lang);
			c->lang = NULL;
		}
		close_ttml_loyout(c);
		close_ttml_body(c);
		av_free(c);
		c = NULL;
	}
}

static int parse_ttml_tt_info(TTMLXmlNodePtr root_node, TtmlContext *c)
{
	int ret = 0;
	TTMLxmlAttrPtr attr = NULL;
	unsigned char *val = NULL;
	if (av_strcasecmp ((const char *)root_node->name, "tt")) {
		return AVERROR_INVALIDDATA;
	}

	attr = root_node->properties;
	while (attr) {
		val = TTMLxmlGetProp(root_node, attr->name);
		if (!av_strcasecmp((const char *)attr->name, "ttp:cellResolution") || !av_strcasecmp((const char *)attr->name, "ttp:cellResolution")) {
			if (!(c->cellResolution = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		} else if (!av_strcasecmp((const char *)attr->name, "ttp:pixelAspectRatio")) {
			if (!(c->pixelAspectRatio = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		} else if (!av_strcasecmp((const char *)attr->name, "ttp:tickRate")) {
			if (!(c->tickRate = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		} else if (!av_strcasecmp((const char *)attr->name, "tts:extent")) {
			if (!(c->extent = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		} else if (!av_strcasecmp((const char *)attr->name, "tts:space")) {
			if (!(c->space = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		} else if (!av_strcasecmp((const char *)attr->name, "lang")) {
			if (!(c->lang = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		}
		attr = attr->next;
		if (val) {
			TTMLxmlFree(val);
			val = NULL;
		}
	}
cleanup:
	if (val) {
		TTMLxmlFree(val);
		val = NULL;
	}
	return ret;
}

static int64_t tt_ParseTime(char *s)
{
	int h1 = 0, m1 = 0, s1 = 0, d1 = 0;
	char c = 0;
	int64_t ttml_time = 0;

	if( sscanf(s, "%d:%d:%d%c%d", &h1, &m1, &s1, &c, &d1 ) == 5
		|| sscanf(s, "%d:%d:%d",  &h1, &m1, &s1          ) == 3) {
			ttml_time = h1 * 3600* 1000 + m1 * 60 * 1000 + s1 * 1000 + d1;
	} else {
		char *psz_end = (char *) s;
		double v = strtod(s, &psz_end);
		if( psz_end != s && *psz_end ) {
			if (*psz_end == 'm') {
				ttml_time = 60*v*1000;
			} else if (*psz_end == 's') {
				ttml_time = v*1000;
			} else if (*psz_end == 'h') {
				ttml_time = v * 3600*1000;
			} else if (*psz_end == 't') {
				ttml_time = v;
			}
		}
	}
	return ttml_time;
}

static int parse_ttml_body(TTMLXmlNodePtr root_node, TtmlContext *c)
{
	int ret = 0;
	unsigned char *val = NULL;
	TTMLXmlNodePtr node = NULL;
	TTMLXmlNodePtr p_node = NULL;
	TTMLxmlAttrPtr attr = NULL;
	ttml_body_element *ttml_body = NULL;

	attr = root_node->properties;
	c->n_ttml_body = 0;
	if (attr) {
		val = TTMLxmlGetProp(root_node, attr->name);
		if (!av_strcasecmp((const char *)attr->name, "body_style")) {
			if (!(c->body_style = av_strdup((char *)val))) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
		}
		if (val) {
			TTMLxmlFree(val);
			val = NULL;
		}
	}
	p_node = TTMLxmlFirstElementChild(root_node);
	do {
		if (!av_strcasecmp((const char *)p_node->name, "p")) {
			break;
		}
		p_node = TTMLxmlFirstElementChild(p_node);
	} while(1);
	node = p_node;
    while (node) {
        if (!av_strcasecmp((const char *)node->name, "p")) {
			attr = node->properties;
			ttml_body = av_mallocz(sizeof(ttml_body_element));
			if (!ttml_body) {
				ret = AVERROR_NOMEM;
				goto cleanup;
			}
			ttml_body->cont_mark = 0;
			while (attr) {
				val = TTMLxmlGetProp(node, attr->name);
				if (!av_strcasecmp((const char *)attr->name, "id")) {
					if (!(ttml_body->id = av_strdup((char *)val))) {
						ret = AVERROR_NOMEM;
						goto cleanup;
					}
				} else if (!av_strcasecmp((const char *)attr->name, "style")) {
					if (!(ttml_body->style = av_strdup((char *)val))) {
						ret = AVERROR_NOMEM;
						goto cleanup;
					}
				} else if (!av_strcasecmp((const char *)attr->name, "region")) {
					if (!(ttml_body->region = av_strdup((char *)val))) {
						ret = AVERROR_NOMEM;
						goto cleanup;
					}
				}  else if (!av_strcasecmp((const char *)attr->name, "ttml:id")) {
					if (!(ttml_body->ttml_id = av_strdup((char *)val))) {
						ret = AVERROR_NOMEM;
						goto cleanup;
					}
				} else	if (!av_strcasecmp((const char *)attr->name, "begin")) {
					ttml_body->begin = tt_ParseTime((char *)val);
				} else	if (!av_strcasecmp((const char *)attr->name, "end")) {
					ttml_body->end = tt_ParseTime((char *)val);
				}
				attr = attr->next;
				if (val) {
					TTMLxmlFree(val);
					val = NULL;
				}
			}
			unsigned char *content = TTMLxmlNodeGetContent(node);
			if (content) {
				if (!(ttml_body->text = av_strdup((char *)content))) {
					ret = AVERROR_NOMEM;
					TTMLxmlFree(content);
					content = NULL;
					goto cleanup;
				}
				TTMLxmlFree(content);
				content = NULL;
			} else {
				//TTMLxmlFree(content);
				//content = NULL;
				p_node = TTMLxmlFirstElementChild(node);
				if (p_node) {
					unsigned char *content = TTMLxmlNodeGetContent(p_node);
					if (content) {
						if (!(ttml_body->text = av_strdup((char *)content))) {
							ret = AVERROR_NOMEM;
							goto cleanup;
						}
					}
					TTMLxmlFree(content);
					content = NULL;
				}
			}
			dynarray_add(&c->ttml_body, &c->n_ttml_body, ttml_body);
        }
        node = TTMLxmlNextElementSibling(node);
    }
	return 0;
cleanup:
	if (val) {
		TTMLxmlFree(val);
		val = NULL;
	}
	close_ttml_body(c);
	return ret;
}

static int parse_ttml_head_layout(TTMLXmlNodePtr root_node, TtmlContext *c)
{
	int ret = 0;
	unsigned char *val = NULL;
	TTMLxmlAttrPtr attr = NULL;
	TTMLXmlNodePtr layout_node = NULL;
	ttml_head_loyout *head_loyout;

	if (!av_strcasecmp((const char *)root_node->name, "head")) {
		if ((layout_node = ttml_find_child (root_node, "layout"))) {
			layout_node = TTMLxmlFirstElementChild(layout_node);
			while (layout_node) {
				if (!av_strcasecmp((const char *)layout_node->name, "region")) {
					if (!(head_loyout = av_mallocz(sizeof(ttml_head_loyout)))) {
						ret = AVERROR_NOMEM;
						goto fail;
					}
					attr = layout_node->properties;
					while (attr) {
						if (!(val = TTMLxmlGetProp(layout_node, attr->name))) {
							continue;
						}
						if (!av_strcasecmp((const char *)attr->name, "id")) {
							if (!(head_loyout->id = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "origin")) {
							if (!(head_loyout->origin = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "extent")) {
							if (!(head_loyout->extent = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "displayAlign")) {
							if (!(head_loyout->displayAlign = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "textAlign")) {
							if (!(head_loyout->textAlign = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "fontSize")) {
							if (!(head_loyout->fontSize = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "fontFamily")) {
							if (!(head_loyout->fontFamily = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						} else if (!av_strcasecmp((const char *)attr->name, "overflow")) {
							if (!(head_loyout->overflow = av_strdup((char *)val))) {
								ret = AVERROR_NOMEM;
								goto fail;
							}
						}
						attr = attr->next;
						if (val) {
							TTMLxmlFree(val);
							val = NULL;
						}
					}
					dynarray_add(&c->head_loyout, &c->n_head_loyout, head_loyout);
				}
				layout_node = TTMLxmlNextElementSibling(layout_node);
			}
		}
	}
	return ret;
fail:
	if (val) {
		TTMLxmlFree(val);
		val = NULL;
	}
	close_ttml_loyout(c);
	return ret;
}

#if 0
static void ttml_parse_printf(TtmlContext *c)
{
	int i = 0;
	ttml_head_loyout *head_loyout;
	ttml_body_element *ttml_body;
	gxlogd_raw_l ("[%s.%d] ttml parse.[%s].[%s].[%s].[%s].[%s].[%s].[%s].\n", __func__, __LINE__, c->cellResolution, 
		c->pixelAspectRatio, c->tickRate, c->extent, c->space, c->lang, c->body_style);
	for (i = 0; i < c->n_head_loyout; i++) {
		head_loyout = c->head_loyout[i];
		if (head_loyout) {
			gxlogd_raw_l ("[%s.%d] ttml parse loyout:%d(%d).[%s].[%s].[%s].[%s].[%s].[%s].[%s].[%s]\n", __func__, __LINE__, i, c->n_head_loyout, 
				head_loyout->id, head_loyout->origin, head_loyout->extent, head_loyout->displayAlign, head_loyout->textAlign, head_loyout->fontSize,
				head_loyout->fontFamily, head_loyout->overflow);

		}
	}
	for (i = 0; i < c->n_ttml_body; i++) {
		ttml_body = c->ttml_body[i];
		if (ttml_body) {
			gxlogd_raw_l ("[%s.%d] ttml parse loyout:%d(%d).[%s].[%s].[%s].[%s].[%s].[%lld..%lld..].\n", __func__, __LINE__, i, c->n_ttml_body, 
				ttml_body->id, ttml_body->style, ttml_body->region, ttml_body->ttml_id, ttml_body->text, ttml_body->begin, ttml_body->end);

		}
	}
}
#endif

int ttml_parse(TtmlContext *c, char *ttml_buf, int ttml_size)
{
	int ret = 0;
	TTMLxmlDocPtr doc = NULL;
	TTMLXmlNodePtr root_node;

	if (ttml_size <= 0) {
		gxloge ("ttml size is <= 0.\n");
		return AVERROR_INVALIDDATA;
	}
#if defined(CONFIG_OPENSOURCE_LIBXML2)
	if (!(doc = xmlReadMemory(ttml_buf, ttml_size, "sub_ttml.xml", NULL, 0))) {
	  gxloge ("Failed to parse document.\n");
	  return -1;
	}
	if (!(root_node = xmlDocGetRootElement(doc))) {
		ret = AVERROR_INVALIDDATA;
		gxloge ("Unable to parse xml file.\n");
		goto cleanup;
	}
#else
	doc = GXxmlParseBuffer("sub_ttml.xml", ttml_buf);
	root_node = doc->root;
#endif

	if ((ret = parse_ttml_tt_info(root_node, c)) < 0)
		goto cleanup;

    root_node = TTMLxmlFirstElementChild(root_node);
    while (root_node) {
        if (!av_strcasecmp((const char *)root_node->name, "head")) {
			if ((ret = parse_ttml_head_layout(root_node, c)) < 0)
				goto cleanup;
        } else if (!av_strcasecmp((const char *)root_node->name, "body")) {
        	if ((ret = parse_ttml_body(root_node, c)) < 0 )
				goto cleanup;
        }
        root_node = TTMLxmlNextElementSibling(root_node);
    }
	TTMLxmlFreeDoc(doc);
	return ret;
cleanup:
    TTMLxmlFreeDoc(doc);
	close_ttml_context(c);
    return ret;
}

#if 0
void test_ttml_parse_file(void)
{
	TtmlContext *c = NULL;
	if (!(c = av_mallocz(sizeof(TtmlContext)))) {
		return AVERROR_NOMEM;
	}
	char xml_string[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<tt\n    xmlns=\"http://www.w3.org/ns/ttml\"\n    xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\"\n    xmlns:tts=\"http://www.w3.org/ns/ttml#styling\"\n    xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\"\n  ttp:cellResolution=\"384 288\"\n  xml:lang=\"eng\">\n    <head>\n        <layout>\n            <region xml:id=\"Default\"\n        tts:origin=\"3% 0%\"\n        tts:extent=\"97% 97%\"\n        tts:displayAlign=\"after\"\n        tts:textAlign=\"center\"\n        tts:fontSize=\"16c\"\n        tts:fontFamily=\"Arial\"\n        tts:overflow=\"visible\" />\n        </layout>\n    </head>\n    <body>\n        <div>\n            <p\n        begin=\"15:31:51.000\"\n        end=\"15:31:51.766\">\n                <span region=\"Default\">but it's clear that protecting this country\n                    <br/>is not one of them\n                </span>\n            </p>\n            <p\n        begin=\"15:31:51.766\"\n        end=\"15:31:52.000\"></p>\n            <p\n        begin=\"15:31:52.000\"\n        end=\"15:31:53.126\"></p>\n            <p\n        begin=\"15:31:53.126\"\n        end=\"15:31:54.000\">\n                <span region=\"Default\">That used to be called treason</span>\n            </p>\n        </div>\n    </body>\n</tt>";
	ttml_parse(xml_string, strlen(xml_string));
	close_ttml_context(c);
}
#endif
