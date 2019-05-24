/******************************************************************************
** \file        adi/test/src/parser.c
**
** \brief       ADI demo configuration file/parameters parser test.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "parser.h"
#include "xml.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static int covert_value(void *dataaddr, int datatype, const char *xmlvalue);


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
int parse_streamNum(char *FilePATH)
{
    int configNum;
    XMLN *rootNode = NULL;
    XMLN *streamSettingNode = NULL;

    FILE *fp = fopen(FilePATH, "rb");
    if(!fp)
    {
        GADI_ERROR("open %s config file failed!\n", FilePATH);
        return -1;
    }
    rootNode = xml_file_parse(fp);
    if (!rootNode)
    {
        GADI_ERROR("Unable to read %s file!\n", FilePATH);
        return -1;
    }
    fclose(fp);
    streamSettingNode = xml_node_get_child(rootNode, "StreamSetting");
    const char *pNum = xml_attr_get_data(streamSettingNode, "Num");
    configNum = atoi(pNum);
    xml_node_del(rootNode);

    return configNum;
}

int parse_streamInfo(parser_map *map, char *FilePATH, int streamindex)
{
    XMLN *rootNode = NULL;
    XMLN *streamSettingNode = NULL;
    XMLN *streamNode = NULL;
    XMLN *valueNode = NULL;

    FILE *fp = fopen(FilePATH, "rb");
    if(NULL == fp)
    {
        GADI_ERROR("open %s config file failed!\n", FilePATH);
        return -1;
    }
    rootNode = xml_file_parse(fp);
    if (!rootNode)
    {
        GADI_ERROR("Unable to read %s file!\n", FilePATH);
        return -1;
    }
    fclose(fp);
    streamSettingNode = xml_node_get_child(rootNode, "StreamSetting");
    char cfgbuf[32];
    memset(cfgbuf, 0, 32);
    snprintf(cfgbuf, sizeof(cfgbuf), "Stream%d", streamindex);
    streamNode = xml_node_get_child(streamSettingNode, cfgbuf);

    int i =0;
    while(map[i].stringName != NULL)
    {
        valueNode = xml_node_get_child(streamNode, map[i].stringName);
        if(valueNode == NULL)
        {
            i++;
            continue;
        }
        if(valueNode->data != NULL)
        {
            covert_value(map[i].dataAddress, map[i].dataType, valueNode->data);
        }
        i++;
    }
    xml_node_del(rootNode);

    return 0;
}

int parse_viInfo(parser_map *map, char *FilePATH)
{
    XMLN *rootNode = NULL;
    XMLN *viNode = NULL;
    XMLN *valueNode = NULL;

    FILE *fp = fopen(FilePATH, "rb");
    if(!fp)
    {
        GADI_ERROR("open %s config file failed!\n", FilePATH);
        return -1;
    }
    rootNode = xml_file_parse(fp);
    if (!rootNode)
    {
        GADI_ERROR("Unable to read %s file!\n", FilePATH);
        return -1;
    }
    fclose(fp);
    viNode = xml_node_get_child(rootNode, "Vin");
    int i =0;
    while(map[i].stringName != NULL)
    {
        valueNode = xml_node_get_child(viNode, map[i].stringName);
        if(valueNode == NULL)
        {
            i++;
            continue;
        }
        if(valueNode->data != NULL)
        {
            covert_value(map[i].dataAddress, map[i].dataType, valueNode->data);
        }
        i++;
    }
    xml_node_del(rootNode);

    return 0;
}

int parse_voInfo(parser_map *map, char *FilePATH)
{
    XMLN *rootNode = NULL;
    XMLN *voNode = NULL;
    XMLN *valueNode = NULL;

    FILE *fp = fopen(FilePATH, "rb");
    if(!fp)
    {
        GADI_ERROR("open %s config file failed!\n", FilePATH);
        return -1;
    }
    rootNode = xml_file_parse(fp);
    if (!rootNode)
    {
        GADI_ERROR("Unable to read %s file!\n", FilePATH);
        return -1;
    }
    fclose(fp);
    voNode = xml_node_get_child(rootNode, "Vout");

    int i =0;
    while(map[i].stringName != NULL)
    {
        valueNode = xml_node_get_child(voNode, map[i].stringName);
        if(valueNode == NULL)
        {
            i++;
            continue;
        }
        if(valueNode->data != NULL)
        {
            covert_value(map[i].dataAddress, map[i].dataType, valueNode->data);
        }
        i++;
    }
    xml_node_del(rootNode);

    return 0;
}

int parse_imageInfo(parser_map *map, char *FilePATH)
{
    XMLN *rootNode = NULL;
    XMLN *valueNode = NULL;

    FILE *fp = fopen(FilePATH, "rb");
    if(!fp)
    {
        GADI_ERROR("open %s config file failed!\n", FilePATH);
        return -1;
    }

    rootNode = xml_file_parse(fp);
    if (!rootNode)
    {
        GADI_ERROR("Unable to read %s file!\n", FilePATH);
        return -1;
    }
    fclose(fp);
    int i =0;
    while(map[i].stringName != NULL)
    {
        valueNode = xml_node_get_child(rootNode, map[i].stringName);
        if(valueNode == NULL)
        {
            i++;
            continue;
        }
        if(valueNode->data != NULL)
        {
            covert_value(map[i].dataAddress, map[i].dataType, valueNode->data);
        }
        i++;
    }
    xml_node_del(rootNode);

    return 0;
}

static int covert_value(void *dataaddr, int datatype, const char *xmlvalue)
{
    switch (datatype)
    {
        case DATA_TYPE_U32:
            *((unsigned int *)(dataaddr)) = (unsigned int)atoi(xmlvalue);
            break;
        case DATA_TYPE_U16:
            *((unsigned short *)(dataaddr)) = (unsigned short)atoi(xmlvalue);
            break;
        case DATA_TYPE_U8:
            *((unsigned char *)(dataaddr)) = (unsigned char)atoi(xmlvalue);
            break;
        case DATA_TYPE_S32:
            *((int *)(dataaddr)) = atoi(xmlvalue);
            break;
        case DATA_TYPE_S16:
            *((signed short *)(dataaddr)) = (signed short)atoi(xmlvalue);
            break;
        case DATA_TYPE_S8:
            *((signed char *)(dataaddr)) = (signed char)atoi(xmlvalue);
            break;
        case DATA_TYPE_DOUBLE:
            *((double *)(dataaddr)) = atof(xmlvalue);
            break;
        case DATA_TYPE_STRING:
            if(dataaddr == NULL)
            {
                dataaddr = (void *)calloc(MAX_TEXT_LEN, 1);
                break;
            }
            strncpy((char *)dataaddr, xmlvalue, MAX_TEXT_LEN - 1);
            break;
        default:
            GADI_ERROR("unknown data type in map definition!\n");
            return -1;
    }

    return 0;
}


