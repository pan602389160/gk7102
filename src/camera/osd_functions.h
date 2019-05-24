/*!
*****************************************************************************
** \file        ./subsystem/osd_functions/include/osd_functions.h
**
** \brief       subsystem layer osd header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _OSD_FUNCTIONS_H_
#define _OSD_FUNCTIONS_H_

#include "stdio.h"

//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
#define OSD_FONT_PATH_MAX_LENGTH (256)
#define OSD_TEXT_MAX_LENGTH      (64)

//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
/*!
*******************************************************************************
** \brief osd colour enum.
*******************************************************************************
*/
typedef enum {
    OSD_COLOUR_UNKNOW = -1,
    OSD_COLOUR_BLACK  = 0,
    OSD_COLOUR_RED,
    OSD_COLOUR_BLUE,
    OSD_COLOUR_GREEN,
    OSD_COLOUR_YELLOW,
    OSD_COLOUR_MAGENTA,
    OSD_COLOUR_CYAN,
    OSD_COLOUR_WHITE,
    OSD_COLOUR_TRANSPARENT,
    OSD_COLOUR_NUMBERS,
}OSD_ColourEnumT;


typedef enum {
	OSD_VECTOR_FONT_LIB = 0, //Ê¸Á¿×Ö¿â
	OSD_LATTICE_FONT_LIB,	//µãÕó×Ö¿â
	OSD_MAX_FONT_LIB
} OSD_FONT_LIBRARY_TYPE;


//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************
/*!
*******************************************************************************
** \brief the parameters struct of text display on osd.
*******************************************************************************
*/
typedef struct
{
    /*the enable/disable flag of area(0:disable, 1:enable).*/
    unsigned char       enable;
    /*font library file path.*/
    char                halffontsPath[OSD_FONT_PATH_MAX_LENGTH];
	/*font library file path.*/
    char                allfontsPath[OSD_FONT_PATH_MAX_LENGTH];
    /*text string.*/
    char                textStr[OSD_TEXT_MAX_LENGTH];
    /*string length.*/
    unsigned int        textLen;
    /*font size.*/
    unsigned char       size;
    /*font color type.*/
    unsigned char       color;
    /*font outline width. 0: no outline*/
    unsigned int        outline;
    /*0: no bold at all; positive is bold, negetive is thin in horizontal.*/
    signed int          bold;
    /*0 is normal (no italic), 1 is 50% italic.*/
    unsigned int        italic;
    /*text area offset x, 0~100, 100 means 100% of encode width.*/
    unsigned short      startX;
    /*text area offset y, 0~100, 100 means 100% of encode height.*/
    unsigned short      startY;
    /*text area box width, 0~100, 100 means 100% of encode width.*/
    unsigned short      boxWidth;
    /*text area box height, 0~100, 100 means 100% of encode height.*/
    unsigned short      boxHeight;
}OSD_TextParamsT;

/*!
*******************************************************************************
** \brief the parameters struct of bmp display on osd.
*******************************************************************************
*/
typedef struct
{
    /*the enable/disable flag of area(0:disable, 1:enable).*/
    unsigned char       enable;
    /*bmp file path&filename.*/
    char*               bmpFile;
    /*text area offset x.*/
    unsigned short      startX;
    /*text area offset y.*/
    unsigned short      startY;
}OSD_BmpParamsT;


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif
int osd_function_init(OSD_FONT_LIBRARY_TYPE type);
int osd_function_exit(void);
int osd_function_show_text( GADI_SYS_HandleT osdHandle,
                                  unsigned short encodeWidth,
                                  unsigned short encodeHeight,
                                  int planeId,
                                  int areaId,
                                  OSD_TextParamsT *text );
int osd_function_show_date(GADI_SYS_HandleT osdHandle,
                                  unsigned short encodeWidth,
                                  unsigned short encodeHeight,
                                  int planeId,
                                  int areaId,
                                  OSD_TextParamsT *text);
int osd_function_exit_show_date(GADI_SYS_HandleT osdHandle);
int osd_function_show_bmp(GADI_SYS_HandleT osdHandle,
                                  int planeId,
                                  int areaId,
                                  OSD_BmpParamsT *bmpPars);

#ifdef __cplusplus
    }
#endif


#endif /*_OSD_FUNCTIONS_H_*/
