/*!
*****************************************************************************
** \file        ./adi/test/src/parser.h
**
**
** \brief       adi demo parser header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _PARSER_H_
#define _PARSER_H_

#include "adi_sys.h"
#include "adi_types.h"



//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************

//#define IMAGEXML_FILE_PATH              "/usr/local/bin/image.xml"
#define VIDEOXML_FAC_FILE_PATH          "/usr/local/bin/factory/video.xml"
//#define IMAGEXML_FAC_FILE_PATH          "/usr/local/bin/factory/image.xml"
//#define IMAGE_TAG                       "Image"
#define VIDEO_TAG                       "Video"
#define VI_TAG                          "Vin"
#define VOUT_TAG                        "Vout"
#define STREAMSETTING_TAG               "StreamSetting"
#define MAX_VIDEOSTREAM_NUM             4
#define MAX_STREAM_CONFIGNUM            32
#define MAX_TEXT_LEN                    32

//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
typedef enum {
    DATA_TYPE_U32 = 0,
    DATA_TYPE_U16,
    DATA_TYPE_U8,
    DATA_TYPE_S32,
    DATA_TYPE_S16,
    DATA_TYPE_S8,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRING,
    DATA_TYPE_ARRAY,
} parser_data_type;


typedef struct
{
    /*parameter's item name.*/
    char*                       stringName;
    /*the address of store the parse value.*/
    void*                       dataAddress;
    /*parser_data_type enum*/
    parser_data_type            dataType;
} parser_map;


//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif
int parse_streamNum(char *FilePATH);
int parse_streamInfo(parser_map *map, char *FilePATH, int streamindex);
int parse_viInfo(parser_map *map, char *FilePATH);
int parse_voInfo(parser_map *map, char *FilePATH);
int parse_imageInfo(parser_map *map, char *FilePATH);

#ifdef __cplusplus
    }
#endif


#endif /* _PARSER_H_ */
