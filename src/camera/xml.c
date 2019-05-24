/*!
*****************************************************************************
** \file        ions/ctlserver/src/xml/xml.c
**
** \version     $Id$
**
** \brief       xml
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2013 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml.h"

/***************************************************************************************/
static int file_write_node(XMLN *node, FILE *p, xml_save_ws_cb cb, int col, xml_putc_cb putc_cb);
static int file_putc(int ch, void *p);
static int file_write_ws(XMLN *node, void *p, xml_save_ws_cb cb, int ws, int col, xml_putc_cb putc_cb);
static int file_write_data(const char *s, void *p, xml_putc_cb putc_cb);
void stream_startElement(void *userData, const char *nodeName, const char **attrs);
void stream_endElement(void *userData, const char *name);
void stream_charData(void *userData, const char *data, int dataLen);
int  stream_parse(LTXMLPRS * parse);
int  stream_parse_header(LTXMLPRS * parse);
int  stream_parse_element(LTXMLPRS * parse);
int  stream_parse_element_start(LTXMLPRS * parse);
int  stream_parse_attr(LTXMLPRS * parse);
int  stream_parse_element_end(LTXMLPRS * parse);
XMLN * xml_stream_parse(char *p_xml, int len);


/***************************************************************************************
 *
 * XML operational functions
 *
***************************************************************************************/
XMLN *xml_node_add_new(XMLN *parent, char *nodeName)
{
    XMLN *p_node = (XMLN *)malloc(sizeof(XMLN));
    if (p_node == NULL)
    {
        printf("xml_node_add_new:memory alloc fail!\n");
        return NULL;
    }
    memset(p_node, 0, sizeof(XMLN));

    p_node->type = NTYPE_TAG;
    p_node->name = strdup(nodeName);//p_node->name = name;

    if (parent != NULL) //if parent == NULL, p_node will be the root node, or it will be a child node of parent.
    {
        p_node->parent = parent;

        if (parent->f_child == NULL)
        {
            parent->f_child = p_node;
            parent->l_child = p_node;
        }
        else
        {
            parent->l_child->next = p_node;
            p_node->prev = parent->l_child;
            parent->l_child = p_node;
        }
    }

    return p_node;
}

XMLN *xml_node_add(XMLN *parent, XMLN *newNode)
{
    if ((parent == NULL)||(newNode == NULL))
    {
        printf("xml_node_add:parent or newNode is NULL\n");
        return NULL;
    }

    newNode->parent = parent;

    if (parent->f_child == NULL)
    {
        parent->f_child = newNode;
        parent->l_child = newNode;
    }
    else
    {
        parent->l_child->next = newNode;
        newNode->prev = parent->l_child;
        parent->l_child = newNode;
    }

    return newNode;
}

void xml_node_del(XMLN *rootNode)
{
    if (rootNode == NULL) return;

    XMLN *p_attr = rootNode->f_attrib;
    while (p_attr)
    {
        XMLN * p_next = p_attr->next;
        if(p_attr->data)
            free(p_attr->data);
        if(p_attr->name)
            free(p_attr->name);

        free(p_attr);

        p_attr = p_next;
    }

    XMLN *p_child = rootNode->f_child;
    while (p_child)
    {
        XMLN *p_next = p_child->next;
        xml_node_del(p_child);
        p_child = p_next;
    }

    if (rootNode->prev) rootNode->prev->next = rootNode->next;
    if (rootNode->next) rootNode->next->prev = rootNode->prev;

    if (rootNode->parent)
    {
        if (rootNode->parent->f_child == rootNode)
            rootNode->parent->f_child = rootNode->next;
        if (rootNode->parent->l_child == rootNode)
            rootNode->parent->l_child = rootNode->prev;
    }

    if(rootNode->name)
        free(rootNode->name);
    if(rootNode->data)
        free(rootNode->data);

    free(rootNode);
}

XMLN *xml_node_get_child(XMLN *parent, const char *nodeName)
{
    if (parent == NULL || nodeName == NULL)
        return NULL;

    XMLN *p_node = parent->f_child;

    while (p_node != NULL)
    {
        if (strcasecmp(p_node->name, nodeName) == 0)
            return p_node;

        p_node = p_node->next;
    }

    return NULL;
}
XMLN *xml_node_get_next(XMLN *curNode, const char *nodeName)
{
    if (curNode == NULL || nodeName == NULL)
        return NULL;

    XMLN *p_node = curNode->next;

    while (p_node != NULL)
    {
        if (strcasecmp(p_node->name, nodeName) == 0)
            return p_node;

        p_node = p_node->next;
    }

    return NULL;
}
XMLN *xml_node_set_data(XMLN *node, const char *data)
{
    if(!node || !data)
        return NULL;
    if(node->data != NULL)
    {
        free(node->data);
        node->data = NULL;
    }
    node->data = malloc((strlen(data) + 1) * sizeof(char));
    if(NULL == node->data)
    {
        printf("xml_node_set_data: memory alloc fail!\n");
    }
    memset(node->data, 0, (strlen(data) + 1));
    strncpy(node->data, data, strlen(data));
    node->data[strlen(data)] = '\0';

    return node;
}

/***************************************************************************************/
XMLN *xml_attr_add(XMLN *node, const char *attrName, const char *attrData)
{
    if (node == NULL || attrName == NULL || attrData == NULL)
        return NULL;

    XMLN *p_attr = (XMLN *)malloc(sizeof(XMLN));
    if (p_attr == NULL)
    {
        printf("xml_attr_add: memory alloc fail!\n");
        return NULL;
    }
    memset(p_attr, 0, sizeof(XMLN));
    //p_attr is a new node , and it will be connected to p_node which is its element.
    p_attr->type = NTYPE_ATTRIB;
    p_attr->name = strdup(attrName);    //p_attr->name = name;
    p_attr->data = strdup(attrData);    //p_attr->data = value;
    p_attr->dlen = strlen(attrData);

    if (node->f_attrib == NULL)
    {
        node->f_attrib = p_attr;
        node->l_attrib = p_attr;
    }
    else
    {
        p_attr->prev = node->l_attrib;
        node->l_attrib->next = p_attr;
        node->l_attrib = p_attr;
    }

    return p_attr;
}

void xml_attr_del(XMLN *node, const char *attrName)
{
    if (node == NULL || attrName == NULL)
        return;

    XMLN *p_attr = node->f_attrib;
    while (p_attr != NULL)
    {
        if (strcasecmp(p_attr->name, attrName) == 0)
        {
            xml_node_del(p_attr);
            return;
        }

        p_attr = p_attr->next;
    }
}

XMLN *xml_attr_get(XMLN *node, const char *attrName)
{
    if (node == NULL || attrName == NULL)
        return NULL;

    XMLN *p_attr = node->f_attrib;
    while (p_attr != NULL)
    {
        if ((NTYPE_ATTRIB == p_attr->type) && (0 == strcasecmp(p_attr->name, attrName)))
            return p_attr;

        p_attr = p_attr->next;
    }

    return NULL;
}

const char *xml_attr_get_data(XMLN *node, const char *attrName)
{
    if (node == NULL || attrName == NULL)
        return NULL;

    XMLN *p_attr = node->f_attrib;
    while (p_attr != NULL)
    {
        if ((NTYPE_ATTRIB == p_attr->type) && (0 == strcasecmp(p_attr->name, attrName)))
            return p_attr->data;

        p_attr = p_attr->next;
    }

    return NULL;
}

/***************************************************************************************/
void xml_cdata_set(XMLN *node, const char *data, int dataLen)
{
    if (node == NULL || data == NULL || dataLen <= 0)
        return;

    node->data = strdup(data);
    node->dlen = dataLen;
}

/***************************************************************************************/
int xml_calc_buf_len(XMLN *node)
{
    int xml_len = 0;
    xml_len += 1 + strlen(node->name);    //sprintf(xml_buf+xml_len, "<%s",p_node->name);

    XMLN *p_attr = node->f_attrib;
    while (p_attr)
    {
        if (p_attr->type == NTYPE_ATTRIB)
            xml_len += strlen(p_attr->name) + 4 + strlen(p_attr->data);    //sprintf(xml_buf+xml_len," %s=\"%s\"",p_attr->name,p_attr->data);
        else if (p_attr->type == NTYPE_CDATA)
        {
            xml_len += 1 + strlen(p_attr->data) + 2 + strlen(node->name) + 1;    //sprintf(xml_buf+xml_len,">%s</%s>",p_attr->data,p_node->name);
            return xml_len;
        }
        else
            ;

        p_attr = p_attr->next;
    }

    if (node->f_child)
    {
        xml_len += 1;    //sprintf(xml_buf+xml_len, ">");

        XMLN * p_child = node->f_child;
        while (p_child)
        {
            xml_len += xml_calc_buf_len(p_child);    //xml_write_buf(p_child,xml_buf+xml_len);
            p_child = p_child->next;
        }

        xml_len += 2 + strlen(node->name) + 1;    //sprintf(xml_buf+xml_len, "</%s>",p_node->name);
    }
    else
    {
        xml_len += 2;    //sprintf(xml_buf+xml_len, "/>");
    }

    return xml_len;
}

int xml_write_buf(XMLN *node, char *xmlBuf)
{
    int xml_len = 0;

    xml_len += sprintf(xmlBuf+xml_len, "<%s", node->name);

    XMLN * p_attr = node->f_attrib;
    while (p_attr)
    {
        if (p_attr->type == NTYPE_ATTRIB)
            xml_len += sprintf(xmlBuf+xml_len, " %s=\"%s\"", p_attr->name, p_attr->data);
        else if(p_attr->type == NTYPE_CDATA)
        {
            xml_len += sprintf(xmlBuf+xml_len,  ">%s</%s>", p_attr->data, node->name);
            return xml_len;
        }
        else
            ;

        p_attr = p_attr->next;
    }

    if (node->f_child)
    {
        xml_len += sprintf(xmlBuf+xml_len, ">");

        XMLN * p_child = node->f_child;
        while (p_child)
        {
            xml_len += xml_write_buf(p_child, xmlBuf+xml_len);
            p_child = p_child->next;
        }

        xml_len += sprintf(xmlBuf+xml_len, "</%s>", node->name);
    }
    else
    {
        xml_len += sprintf(xmlBuf+xml_len, "/>");
    }

    return xml_len;
}

int xml_nwrite_buf(XMLN *node, char *xmlBuf, int bufLen)
{
    int xml_len = 0;

    if ((NULL == node) || (NULL == node->name))
        return -1;

    if (strlen(node->name) >= bufLen)
        return -1;

    xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, "<%s", node->name);

    XMLN * p_attr = node->f_attrib;
    while (p_attr)
    {
        if (p_attr->type == NTYPE_ATTRIB)
        {
            if ((strlen(p_attr->name) + strlen(p_attr->data) + xml_len) > bufLen)
                return -1;
            xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, " %s=\"%s\"", p_attr->name, p_attr->data);
        }
        else if (p_attr->type == NTYPE_CDATA)
        {
            if (0x0a == (*p_attr->data))
            {
                p_attr = p_attr->next;
                continue;
            }
            if ((strlen(p_attr->data) + strlen(node->name) + xml_len) >= bufLen)
                return -1;
            xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, ">%s</%s>", p_attr->data, node->name);
            return xml_len;
        }
        else
            ;

        p_attr = p_attr->next;
    }

    int ret = 0;

    if (node->f_child)
    {
        xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, ">");

        XMLN * p_child = node->f_child;
        while (p_child)
        {
            ret = xml_nwrite_buf(p_child, xmlBuf+xml_len, bufLen-xml_len);
            if (ret < 0)
                return ret;
            xml_len += ret;
            p_child = p_child->next;
        }

        xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, "</%s>", node->name);
    }
    else
    {
        xml_len += snprintf(xmlBuf+xml_len, bufLen-xml_len, "/>");
    }

    return xml_len;
}

/***************************************************************************************/
XMLN * xml_stream_parse(char *p_xml, int len)
{
    XMLN * p_root = NULL;

    LTXMLPRS parse;
    memset(&parse, 0, sizeof(parse));

    parse.xmlstart = p_xml;
    parse.xmlend = p_xml + len;
    parse.ptr = parse.xmlstart;

    parse.userdata = &p_root;//address of p_root pointer for handling what it point to.
    parse.startElement = stream_startElement; //handle new node and its attributes.
    parse.endElement = stream_endElement; //handle end of one node.
    parse.charData = stream_charData;    //handle data of one element.

    int status = stream_parse(&parse);
    if (status < 0)
    {
        printf("xml_stream_parse:err[%d]\r\n", status);
        xml_node_del(p_root);
        p_root = NULL;
    }

    return p_root;
}

XMLN *onvif_xml_file_parse(char *buf,int len)
{
    XMLN *rootNode = NULL;
    rootNode = xml_stream_parse(buf,len);
    if (!rootNode)
        return NULL;
    return rootNode;
}

XMLN *xml_file_parse(FILE *fp)
{
    int XMLsize;
    XMLN *rootNode = NULL;
    char *XMLdata = NULL;
    if(!fp)
    {
        printf("Parameters error!\n");
        return NULL;
    }

    if(0 != fseek(fp, 0, SEEK_END))
        return NULL;
    XMLsize = ftell(fp);
    if(0 != fseek(fp, 0, SEEK_SET))
        return NULL;
    XMLdata = malloc(XMLsize * sizeof(char));
    if(!XMLdata)
        return NULL;
    memset(XMLdata, 0, XMLsize);
    fread(XMLdata, 1, XMLsize-1, fp);
    *(XMLdata+XMLsize-1) = '\0';

    rootNode = xml_stream_parse(XMLdata, strlen(XMLdata));
    if (!rootNode)
        return NULL;

    free(XMLdata);
    XMLdata = NULL;

    return rootNode;
}

int xml_file_save(XMLN *node, FILE *fp, xml_save_ws_cb cb)
{
    int col;
    if ((col = file_write_node(node, fp, cb, 0, file_putc)) < 0)
        return (-1);

    if (col > 0)
    {
        if (putc('\n', fp) < 0)
            return (-1);
    }
    return (0);
}

/***************************************************************************************/
static int file_write_node(XMLN *node, FILE *p, xml_save_ws_cb cb, int col, xml_putc_cb putc_cb)
{
    int width;
    XMLN *attr = NULL;

    col = file_write_ws(node, p, cb, XML_WS_BEFORE_OPEN, col, putc_cb);

    if((*putc_cb)('<', p) < 0)
        return (-1);

    if(node->name[0] == '?' || !strncmp(node->name, "!--", 3) || !strncmp(node->name, "![CDATA[", 8))//write xml header or comment or CDATA.
    {
        const char *ptr = NULL;
        for (ptr = node->name; *ptr; ptr++)
        {
            if ((*putc_cb)(*ptr, p) < 0)
                return (-1);
        }
    }
    else //write name of element.
    {
        if(file_write_data(node->name, p, putc_cb) < 0)
            return (-1);
    }

    col += strlen(node->name) + 1;

    for (attr = node->f_attrib; attr; attr = attr->next) // write attibutes.
    {
        width = strlen(attr->name);

        if ((*putc_cb)(' ', p) < 0)
            return (-1);

        col ++;

        if (file_write_data(attr->name, p, putc_cb) < 0)
            return (-1);

        if (attr->data)
        {
            width += strlen(attr->data) + 3;

            if ((*putc_cb)('=', p) < 0)
                return (-1);
            if ((*putc_cb)('\"', p) < 0)
                return (-1);
            if (file_write_data(attr->data, p, putc_cb) < 0)
                return (-1);
            if ((*putc_cb)('\"', p) < 0)
                return (-1);
        }

        col += width;
    }

    if (node->f_child)//when node->f_child is not NULL, keep finding bottom node.
    {
        XMLN *child;

        if ((*putc_cb)('>', p) < 0)
            return (-1);
        else
            col ++;

        col = file_write_ws(node, p, cb, XML_WS_AFTER_OPEN, col, putc_cb);

        for (child = node->f_child; child; child = child->next)//go into child node of parent node.
        {
            if ((col = file_write_node(child, p, cb, col, putc_cb)) < 0)//go into child node of parent node.
                return (-1);
        }

        if (node->name[0] != '!' && node->name[0] != '?')//if element is not comments or header, write end of element.  </.....>
        {
            col = file_write_ws(node, p, cb, XML_WS_BEFORE_CLOSE, col, putc_cb);

            if ((*putc_cb)('<', p) < 0)
                return (-1);
            if ((*putc_cb)('/', p) < 0)
                return (-1);
            if (file_write_data(node->name, p, putc_cb) < 0)
                return (-1);
            if ((*putc_cb)('>', p) < 0)
                return (-1);

            col += strlen(node->name) + 3;

            col = file_write_ws(node, p, cb, XML_WS_AFTER_CLOSE, col, putc_cb);
        }
    }
    else if (node->name[0] == '!' || node->name[0] == '?')//xml header or comments
    {

        if ((*putc_cb)('>', p) < 0)
            return (-1);
        else
            col ++;

        col = file_write_ws(node, p, cb, XML_WS_AFTER_OPEN, col, putc_cb);
    }
    else //write data of node.
    {
        if ((*putc_cb)('>', p) < 0)
            return (-1);
        else
            col ++;

        file_write_data(node->data, p, putc_cb);

        col += strlen(node->data);

        if (node->name[0] != '!' && node->name[0] != '?') // write end of node.
        {
            col = file_write_ws(node, p, cb, XML_WS_BEFORE_CLOSE, col, putc_cb);
            if ((*putc_cb)('<', p) < 0)
                return (-1);
            if ((*putc_cb)('/', p) < 0)
                return (-1);
            if (file_write_data(node->name, p, putc_cb) < 0)
                return (-1);
            if ((*putc_cb)('>', p) < 0)
                return (-1);

            col += strlen(node->name) + 3;

            col = file_write_ws(node, p, cb, XML_WS_AFTER_CLOSE, col, putc_cb);
        }

    }

    return (col);

}

static int file_putc(int ch, void *p)
{
    return (fputc(ch, (FILE *)p) == EOF ? -1 : 0);
}

static int file_write_ws(XMLN *node, void *p, xml_save_ws_cb cb, int ws, int col, xml_putc_cb putc_cb)
{
    const char *s;
    if (cb && (s = (*cb)(node, ws)) != NULL)
    {
        while (*s)
        {
            if ((*putc_cb)(*s, p) < 0)
                return (-1);
            else if (*s == '\n')
                col = 0;
            else if (*s == '\t')
            {
                col += XML_TAB;
                col = col - (col % XML_TAB);
            }
            else
                col ++;

            s ++;
        }
    }

    return (col);
}

static int file_write_data(const char *s, void *p, xml_putc_cb putc_cb)
{
    while (*s)
    {
        if ((*putc_cb)(*s, p) < 0)
            return (-1);

        s ++;
    }

    return (0);
}

void stream_startElement(void *userData, const char *nodeName, const char **attrs)//handle new node and attributes.
{
    XMLN **pp_node = (XMLN **)userData;
    if (pp_node == NULL)
    {
        return;
    }

    XMLN *parent = *pp_node;
    XMLN *p_node = xml_node_add_new(parent,(char *)nodeName);
    if (attrs)
    {
        int i=0;
        while (attrs[i] != NULL)//attributes of one element will be connected to each other one by one.
        {
            if (attrs[i+1] == NULL)
                break;

            XMLN *p_attr = xml_attr_add(p_node,attrs[i],attrs[i+1]);    //(XMLN *)malloc(sizeof(XMLN));
            if(p_attr == NULL)
            {
                printf("xml add attr node failed!\n");
                break;
            }

            i += 2;
        }
    }

    *pp_node = p_node;
}

void stream_endElement(void *userData, const char *name)
{
    XMLN **pp_node = (XMLN **)userData;
    if (pp_node == NULL)
    {
        return;
    }

    XMLN *p_node = *pp_node;
    if (p_node == NULL)
    {
        return;
    }

    p_node->finish = 1; //the flag of ending handling the new element.

    if (p_node->type == NTYPE_TAG && p_node->parent == NULL)
    {
        // parse finish
    }
    else
    {
        *pp_node = p_node->parent; //put the current node to parent.
    }
}

void stream_charData(void *userData, const char *data, int dataLen)
{
    XMLN **pp_node = (XMLN **)userData;
    if (pp_node == NULL)
    {
        return;
    }

    XMLN *p_node = *pp_node;
    if (p_node == NULL)
    {
        return;
    }

    p_node->data = strdup(data);   //p_node->data = data;
    p_node->dlen = dataLen;         //length of data.
}

int stream_parse(LTXMLPRS *parse)
{
    char *ptr = parse->ptr;
    char *xmlend = parse->xmlend;

    while(IS_WHITE_SPACE(*ptr) && (ptr != xmlend))
        ptr++;

    if(ptr == xmlend)
        return -1;

    if(IS_XMLH_START(ptr))
    {
        int ret = stream_parse_header(parse);
        if(ret < 0)
        {
            printf("hxml parse xml header failed!!!\r\n");
            return -1;
        }
    }
    return stream_parse_element(parse);
}

int stream_parse_header(LTXMLPRS *parse)
{
    char *ptr = parse->ptr;
    char *xmlend = parse->xmlend;

    while(IS_WHITE_SPACE(*ptr) && (ptr != xmlend))
        ptr++;

    if(ptr == xmlend)
        return -1;

    if(!IS_XMLH_START(ptr))
        return -1;
    ptr += 2;

    while((!IS_XMLH_END(ptr)) && (ptr != xmlend))
        ptr++;

    if(ptr == xmlend)
        return -1;

    ptr += 2;

    parse->ptr = ptr;

    return 0;
}

int stream_parse_element(LTXMLPRS *parse)
{
    char * xmlend = parse->xmlend;
    int parse_type = CUR_PARSE_START;
    while(1)
    {
        int ret = RF_NO_END;

xml_parse_type:
        while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
            (parse->ptr)++;
        if((parse->ptr) == xmlend)
        {
            if(parse->e_stack_index == 0)
                return 0;

            return -1;
        }
        if(IS_XML_COMMENT_START(parse->ptr))
        {
            parse->ptr += 4;

            while(!IS_XML_COMMENT_END(parse->ptr))
                (parse->ptr)++;

            parse->ptr += 3;
        }
        while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
            (parse->ptr)++;
        if((parse->ptr) == xmlend)
        {
            if(parse->e_stack_index == 0)
                return 0;

            return -1;
        }

        if(*(parse->ptr) == '<' && *((parse->ptr)+1) == '/')
        {
            (parse->ptr)+=2;
            parse_type = CUR_PARSE_END;
        }
        else if(*(parse->ptr) == '<')
        {
            (parse->ptr)++;
            parse_type = CUR_PARSE_START;
        }
        else
        {
            return -1;
        }

//xml_parse_point:

        while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
            (parse->ptr)++;
        if((parse->ptr) == xmlend)
        {
            if(parse->e_stack_index == 0)
                return 0;
            return -1;
        }

        if(parse_type == CUR_PARSE_END)
        {
            ret = stream_parse_element_end(parse);
            if(ret < 0)
                return -1;

            if(parse->e_stack_index == 0) //when e_stack_index = 0, end parsing XML.
                return 0;

            parse_type = CUR_PARSE_START;
        }
        else
        {
            ret = stream_parse_element_start(parse);
            if(ret < 0)
                return -1;
            if(ret == RF_ELEE_END)
            {
                if(parse->e_stack_index == 0)
                    return 0;

                parse_type = CUR_PARSE_START;
                goto xml_parse_type;
            }

            while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend)) (parse->ptr)++;
            if((parse->ptr) == xmlend) return -1;

            if(*(parse->ptr) == '<') goto xml_parse_type; //if *(parse->ptr) != '<', start parsing data of element.

            char * cdata_ptr = (parse->ptr);
            while(*(parse->ptr) != '<' && (parse->ptr) != xmlend) (parse->ptr)++;
            if((parse->ptr) == xmlend) return -1;

            int len = (parse->ptr) - cdata_ptr;
            if(len > 0)
            {
                *(parse->ptr) = '\0'; (parse->ptr)++;
                if(parse->charData)
                    parse->charData(parse->userdata, cdata_ptr, len);

                if(*(parse->ptr) != '/')
                    return -1;

                (parse->ptr)++;

                if(stream_parse_element_end(parse) < 0)
                    return -1;
            }

            goto xml_parse_type;
        }
    }

    return 0;
}

int stream_parse_element_start(LTXMLPRS *parse)
{
    char * xmlend = parse->xmlend;

    while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend)) (parse->ptr)++;
    if((parse->ptr) == xmlend) return -1;

    char * element_name = (parse->ptr);
    while((!IS_WHITE_SPACE(*(parse->ptr))) && ((parse->ptr) != xmlend) && (!IS_ELEMS_END((parse->ptr)))) (parse->ptr)++;
    if((parse->ptr) == xmlend) return -1;

    parse->e_stack_index++; parse->e_stack[parse->e_stack_index] = element_name;//put the name of element into e_stack.
    CHECK_XML_STACK_RET(parse);

    if(*(parse->ptr) == '>')
    {
        *(parse->ptr) = '\0'; (parse->ptr)++;
        if(parse->startElement)
            parse->startElement(parse->userdata, element_name, parse->attr);
        return RF_ELES_END;
    }
    else if(*(parse->ptr) == '/' && *((parse->ptr)+1) == '>')
    {
        *(parse->ptr) = '\0'; (parse->ptr)+=2;
        if(parse->startElement)
            parse->startElement(parse->userdata, element_name, parse->attr);
        if(parse->endElement)
            parse->endElement(parse->userdata, element_name);

        parse->e_stack[parse->e_stack_index] = NULL; parse->e_stack_index--;
        CHECK_XML_STACK_RET(parse);
        return RF_ELEE_END;
    }
    else
    {
        *(parse->ptr) = '\0'; (parse->ptr)++;

        int ret = stream_parse_attr(parse); if(ret < 0) return -1;

        if(parse->startElement)
            parse->startElement(parse->userdata, element_name, parse->attr);

        memset(parse->attr, 0, sizeof(parse->attr));

        if(ret == RF_ELEE_END)
        {
            if(parse->endElement)
                parse->endElement(parse->userdata, element_name);

            parse->e_stack[parse->e_stack_index] = NULL; parse->e_stack_index--;
            CHECK_XML_STACK_RET(parse);
        }

        return ret;
    }
}

int stream_parse_attr(LTXMLPRS *parse)
{
    char * ptr = parse->ptr;
    char * xmlend = parse->xmlend;
    int ret = RF_NO_END;
    int cnt = 0;

    while(1)
    {
        ret = RF_NO_END;

        while(IS_WHITE_SPACE(*ptr) && (ptr != xmlend)) ptr++;
        if(ptr == xmlend) return -1;

        if(*ptr == '>')
        {
            *ptr = '\0'; ptr++;
            ret = RF_ELES_END; // node start finish
            break;
        }
        else if(*ptr == '/' && *(ptr+1) == '>')
        {
            *ptr = '\0'; ptr += 2;
            ret = RF_ELEE_END;    // node end finish
            break;
        }

        char * attr_name = ptr;
        while(*ptr != '=' && (!IS_ELEMS_END(ptr)) && ptr != xmlend) ptr++;
        if(ptr == xmlend) return -1;
        if(IS_ELEMS_END(ptr))
        {
            if(*ptr == '>')
            {
                ret = RF_ELES_END;
                *ptr = '\0'; ptr++;
            }
            else if(*ptr == '/' && *(ptr+1) == '>')
            {
                ret = RF_ELEE_END;
                *ptr = '\0'; ptr+=2;
            }
            break;
        }

        *ptr = '\0';    // '=' --> '\0'
        ptr++;

        char * attr_value = ptr;
        if(*ptr == '"')
        {
            attr_value++;
            ptr++;
            while(*ptr != '"' && ptr != xmlend) ptr++;
            if(ptr == xmlend) return -1;
            *ptr = '\0'; // '"' --> '\0'

            ptr++;
        }
        else
        {
            while((!IS_WHITE_SPACE(*ptr)) && (!IS_ELEMS_END(ptr)) && ptr != xmlend) ptr++;
            if(ptr == xmlend) return -1;

            if(IS_WHITE_SPACE(*ptr))
            {
                *ptr = '\0';
                ptr++;
            }
            else
            {
                if(*ptr == '>')
                {
                    ret = RF_ELES_END;
                    *ptr = '\0'; ptr++;
                }
                else if(*ptr == '/' && *(ptr+1) == '>')
                {
                    ret = RF_ELEE_END;
                    *ptr = '\0'; ptr+=2;
                }
            }
        }

        int index = cnt << 1;
        parse->attr[index] = attr_name;
        parse->attr[index+1] = attr_value;
        cnt++;
        if(ret > RF_NO_END)
            break;
    }

    parse->ptr = ptr;
    return ret;
}

int stream_parse_element_end(LTXMLPRS *parse)
{
    char * stack_name = parse->e_stack[parse->e_stack_index];
    if(stack_name == NULL)
        return -1;

    char * xmlend = parse->xmlend;

    while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend)) (parse->ptr)++;
    if((parse->ptr) == xmlend) return -1;

    char * end_name = (parse->ptr);
    while((!IS_WHITE_SPACE(*(parse->ptr))) && ((parse->ptr) != xmlend) && (*(parse->ptr) != '>')) (parse->ptr)++;
    if((parse->ptr) == xmlend) return -1;
    if(IS_WHITE_SPACE(*(parse->ptr)))
    {
        *(parse->ptr) = '\0'; (parse->ptr)++;
        while(IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend)) (parse->ptr)++;
        if((parse->ptr) == xmlend) return -1;
    }

    if(*(parse->ptr) != '>') return -1;
    *(parse->ptr) = '\0';
    (parse->ptr)++;

    if(strcasecmp(end_name, stack_name) != 0)
    {
        printf("stream_parse_element_end::cur name[%s] != stack name[%s]!!!\r\n", end_name, stack_name);
        return -1;
    }

    if(parse->endElement)
        parse->endElement(parse->userdata, end_name);

    parse->e_stack[parse->e_stack_index] = NULL; parse->e_stack_index--;
    CHECK_XML_STACK_RET(parse);

    return 0;
}

/***************************************************************************************
 *
 * soap functions
 *
***************************************************************************************/
int soap_strcmp(const char *str1, const char *str2)
{
    if (strcasecmp(str1, str2) == 0)
        return 0;

    const char *ptr1 = strchr(str1, ':');
    const char *ptr2 = strchr(str2, ':');

    if (ptr1 && ptr2)
        return strcasecmp(ptr1+1, ptr2+1);
    else if (ptr1)
        return strcasecmp(ptr1+1, str2);
    else if (ptr2)
        return strcasecmp(str1, ptr2+1);
    else
        return -1;
}

XMLN *xml_node_soap_get(XMLN *parent, const char *nodeName)
{
    if (parent == NULL || nodeName == NULL)
        return NULL;

    XMLN * p_node = parent->f_child;
    while (p_node != NULL)
    {
        if (soap_strcmp(p_node->name, nodeName) == 0)
            return p_node;

        p_node = p_node->next;
    }

    return NULL;
}

