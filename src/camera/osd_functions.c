/******************************************************************************
** \file		/subsystem/osd_functions/src/osd_function.c
**
** \brief		subsystem layer osd functions.
**
** \attention	THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**				ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**				OMMISSIONS
**
** (C) Copyright 2014-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <wchar.h>

#include <getopt.h>

#include "basetypes.h"
#include "adi_sys.h"
#include "adi_osd.h"
#include "osd_text.h"
#include "osd_functions.h"
#include "vector_font.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define OSD_TRANSPARENT               (0)
#define OSD_NONTRANSPARENT            (255)

#define OSD_CLUT_COLOUR_NUM           (256)

#define OSD_DATE_THREAD_PRIORITY      (3)
#define OSD_DATE_THREAD_STACKSIZE     (4096)
#define OSD_DATE_THREAD_NAME          "osdDateThread"

#define	OSD_BITMAPFILE_HEADER_SIZE	  (14)
#define	OSD_BITMAPINFO_HEADER_SIZE	  (40)

#define	OSD_BMP_FILE_MAGIC_NO         (0x4D42)
#define	OSD_BMP_FILE_BIT_COUNT	      (8)

//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************

typedef struct
{
    /*the enable/disable flag of show date info(0:disable, 1:enable).*/
    unsigned char       enable;
    /*the index of area.*/
    unsigned char       areaId;
    /*font size.*/
    unsigned char       fontSize;
    /*offset x.*/
    unsigned short      osdDateOffsetX[OSD_TEXT_MAX_LENGTH];
    /*offset y.*/
    unsigned short      osdDateOffsetY[OSD_TEXT_MAX_LENGTH];
}OSD_ShowDateParams;

typedef struct
{
    /*font pixel colour.*/
    OSD_ColourEnumT    font;
    /*outline pixel colour.*/
    OSD_ColourEnumT    outline;
}OSD_TextColorT;

typedef struct
{
    /*font data pointer.*/
    RGBQUAD                     *data;
    /*font data length.*/
    unsigned int                dataLen;
    /*font box width.*/
    unsigned int                width;
    /*font box height.*/
    unsigned int                height;
}OSD_TextDataT;

/*colour look-up table struct of YUV domain.*/
typedef struct
{
    /*v value of yuv.*/
    unsigned char   v;
    /*u value of yuv.*/
    unsigned char   u;
    /*y value of yuv.*/
    unsigned char   y;
    /*transparent value.*/
    unsigned char   a;
}OSD_YUVClutT;


/*14 byptes*/
typedef struct
{
    /*bmp file header: file type.*/
	unsigned short	bfType;
    /*bmp file header: file size.*/
	unsigned int	bfSize;
    /*bmp file header: reserved.*/
	unsigned short	bfReserved1;
    /*bmp file header: reserved.*/
	unsigned short	bfReserved2;
    /*bmp file header: offset bits.*/
	unsigned int	bfOffBits;
}OSD_BmpHeader;

/*bmp inforamtion header 40bytes*/
typedef struct
{
    /*bmp inforamtion header: size*/
	unsigned int	biSize;
    /*bmp inforamtion header: width*/
	unsigned int	biWidth;
    /*bmp inforamtion header: height*/
    unsigned int	biHeight;
    /*bmp inforamtion header: planes*/
	unsigned short	biPlanes;
    /*bmp inforamtion header:  1,4,8,16,24 ,32 color attribute*/
	unsigned short	biBitCount;
	/*bmp inforamtion header: Compression*/
	unsigned int	biCompression;
    /*bmp inforamtion header: Image size*/
	unsigned int	biSizeImage;
	/*bmp inforamtion header: XPelsPerMerer*/
	unsigned int	biXPelsPerMerer;
    /*bmp inforamtion header: YPelsPerMerer*/
	unsigned int	biYPelsPerMerer;
    /*bmp inforamtion header: ClrUsed*/
	unsigned int	biClrUsed;
    /*bmp inforamtion header: ClrImportant*/
	unsigned int	biClrImportant;
}OSD_BmpInfoHeader;



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
static RGBQUAD osdColoTab[] =
{
    {   // black
        .b = 0,
        .g = 0,
        .r = 0,
        .a = OSD_NONTRANSPARENT,
    },
    {   // red
        .b = 0,
        .g = 0,
        .r = 255,
        .a = OSD_NONTRANSPARENT,
    },
    {   // blue
        .b = 255,
        .g = 0,
        .r = 0,
        .a = OSD_NONTRANSPARENT,
    },
    {   // green
        .b = 0,
        .g = 255,
        .r = 0,
        .a = OSD_NONTRANSPARENT,
    },
    {   // yellow
        .b = 0,
        .g = 255,
        .r = 255,
        .a = OSD_NONTRANSPARENT,
    },
    {   // magenta
        .b = 70,
        .g = 30,
        .r = 230,
        .a = OSD_NONTRANSPARENT,
    },
    {   // cyan
        .b = 229,
        .g = 220,
        .r = 0,
        .a = OSD_NONTRANSPARENT,
    },
    {   // white
        .b = 255,
        .g = 255,
        .r = 255,
        .a = OSD_NONTRANSPARENT,
    },
    {   // transparent
        .b = 0,
        .g = 0,
        .r = 0,
        .a = OSD_TRANSPARENT,
    }
};

static OSD_TextColorT  textColors[OSD_COLOUR_NUMBERS] =
{
    {OSD_COLOUR_BLACK,     OSD_COLOUR_WHITE},
    {OSD_COLOUR_RED,       OSD_COLOUR_WHITE},
    {OSD_COLOUR_BLUE,      OSD_COLOUR_WHITE},
    {OSD_COLOUR_GREEN,     OSD_COLOUR_WHITE},
    {OSD_COLOUR_YELLOW,    OSD_COLOUR_WHITE},
    {OSD_COLOUR_MAGENTA,   OSD_COLOUR_WHITE},
    {OSD_COLOUR_CYAN,      OSD_COLOUR_WHITE},
    {OSD_COLOUR_WHITE,     OSD_COLOUR_BLACK},
};



/*text insert library initialize parameters.*/
static pixel_type_t pixelType =
{
    .pixel_background = {   // transparent
        .b = 0,
        .g = 0,
        .r = 0,
        .a = OSD_TRANSPARENT,
    },
    .pixel_outline    = {   // transparent
        .b = 0,
        .g = 0,
        .r = 0,
        .a = OSD_TRANSPARENT,
    },
    .pixel_font       = {   // black
        .b = 0,
        .g = 0,
        .r = 0,
        .a = OSD_NONTRANSPARENT,
    },
};

static OSD_ShowDateParams dateParams[OSD_PLANE_NUM];

static GADI_SYS_ThreadHandleT    osdDateThreadHandle = 0;
static unsigned char    osdDateThreadRunning = 0;
static unsigned char    osdDateFontSize = 0;
static RGBQUAD*   osdDateBitMap[OSD_PLANE_NUM * OSD_AREA_NUM];
static RGBQUAD*   osdBitMap[OSD_PLANE_NUM * OSD_AREA_NUM];
static GADI_OSD_BitMapAttrT bitMapAttr[OSD_PLANE_NUM * OSD_AREA_NUM];

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static int osd_fill_text_clut(OSD_TextParamsT *text);
static int osd_set_text_font(const OSD_TextParamsT *text);
static int osd_convert_text(const OSD_TextParamsT *text,
                                 unsigned short width,
                                 unsigned short height,
                                 unsigned char  dateEnable,
                                 OSD_ShowDateParams *datePar,
                                 OSD_TextDataT *textData,
                                 int equal_interval);
static int osd_create_digit_bitmap(const OSD_TextParamsT *text, GADI_OSD_AreaIndexT *areaIndex);
static int osd_destroy_digit_bitmap(void);
static int osd_udpate_date_string(GADI_SYS_HandleT osdHandle,
                                         unsigned int plane,
                                         const char *preDate,
                                         const char *curDate);
static void osd_update_date_thread(void* optArg);
static void osd_rgb_to_yuv(const RGBQUAD * rgb, OSD_YUVClutT *clut);
static int osd_read_bmp_file_header(FILE *fp, unsigned int osdSize,
                                   unsigned int *width, unsigned int *height,
                                   unsigned int *size, unsigned int *colorCount);


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
int osd_function_init(OSD_FONT_LIBRARY_TYPE type)
{
    /*init text lib.*/
    if (osd_text_lib_init(type) < 0)
    {
        return -1;
    }
    return GADI_OK;
}

int osd_function_exit(void)
{
    osd_text_lib_exit();
    return GADI_OK;
}

int osd_function_show_text( GADI_SYS_HandleT osdHandle,
                                  unsigned short encodeWidth,
                                  unsigned short encodeHeight,
                                  int planeId,
                                  int areaId,
                                  OSD_TextParamsT *text )
{
    int retVal;
    GADI_OSD_AreaIndexT     areaIndex;
    GADI_OSD_AreaMappingT   areaMapping;
    GADI_OSD_AreaParamsT    areaParams;
    OSD_TextDataT osdText;

    /*check input parameters.*/
    if((osdHandle == NULL) || (text == NULL) || (text->textLen == 0))
    {
        return -1;
    }

    if((planeId >= OSD_PLANE_NUM) || (areaId >= OSD_AREA_NUM))
    {
        printf("invalid planeId=%d or areaId=%d.\n", planeId, areaId);
        return -1;
    }

    /*check video width and height.*/
    if((encodeWidth == 0) || (encodeHeight == 0))
    {
        return -1;
    }

    /*get osd are mapping(clut addr & area addr.)*/
    areaIndex.areaId  = areaId;
    areaIndex.planeId = planeId;
    retVal = gadi_osd_get_area_mapping(osdHandle, areaIndex, &areaMapping);
	if(retVal != GADI_OK)
	{
		printf("get osd area mapping error.\n");
		return retVal;
	}

    gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));

    if (text->enable)
    {
        GADI_OSD_BitMapAttrT bitMapAttr;
        printf("text string:%s, text length:%d\n", text->textStr, text->textLen);

        retVal = osd_fill_text_clut(text);
        if (retVal != GADI_OK)
        {
            printf("osd_fill_text_clut: fill text clut failed!\n");
            return retVal;
        }

        retVal = osd_set_text_font(text);
        if (retVal < 0)
        {
            printf("osd_set_text_font failed!\n");
            return retVal;
        }

        osdText.dataLen = areaMapping.areaSize;
        osdText.data    = calloc(sizeof(RGBQUAD), osdText.dataLen);
        if (osdText.data == NULL)
        {
            return -1;
        }

        retVal = osd_convert_text(text, encodeWidth, encodeHeight, 0, NULL, &osdText, 0);
        if (retVal != GADI_OK)
        {
            printf("osd_convert_text: failed !\n");
            return retVal;
        }
        areaParams.planeId = planeId;
        areaParams.areaId  = areaId;
        areaParams.enable  = text->enable;
        areaParams.width   = osdText.width;
        areaParams.height  = osdText.height;
        areaParams.offsetX = ROUND_UP(text->startX * encodeWidth / 100, 4);
        areaParams.offsetY = ROUND_UP(text->startY * encodeHeight / 100, 4);
        bitMapAttr.alpha = OSD_NONTRANSPARENT;
        bitMapAttr.bitMapColorFmt = GADI_OSD_COLORFMT_RGB32;
        bitMapAttr.width = osdText.width;
        bitMapAttr.height = osdText.height;
        bitMapAttr.pitch = osdText.width * sizeof(RGBQUAD);
        bitMapAttr.bitMapAddr = osdText.data;

        gadi_osd_load_bitmap(osdHandle, &areaParams, &bitMapAttr);

        free(osdText.data);
        osdText.data = NULL;
        printf("areaParams: width=%d, height=%d, offsetX=%d, offsetY=%d\n",
            areaParams.width, areaParams.height, areaParams.offsetX, areaParams.offsetY);
    }
    else
    {
        gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));
        areaParams.planeId = planeId;
        areaParams.areaId  = areaId;
        areaParams.enable  = text->enable;
        gadi_osd_set_area_params(osdHandle, &areaParams);
    }

    return GADI_OK;
}

int osd_function_show_date(GADI_SYS_HandleT osdHandle,
                                  unsigned short encodeWidth,
                                  unsigned short encodeHeight,
                                  int planeId,
                                  int areaId,
                                  OSD_TextParamsT *text)
{
    GADI_ERR retVal;
    GADI_OSD_AreaIndexT     areaIndex;
    GADI_OSD_AreaMappingT   areaMapping;
    GADI_OSD_AreaParamsT    areaParams;
    OSD_TextDataT osdText;
    OSD_ShowDateParams *datePar = NULL;

    /*check input parameters.*/
    if((osdHandle == NULL) || (text == NULL) || (text->textLen == 0))
    {
        return -1;
    }

    if((planeId >= OSD_PLANE_NUM) || (areaId >= OSD_AREA_NUM))
    {
        printf("invalid planeId=%d or areaId=%d.\n", planeId, areaId);
        return -1;
    }

    /*check video width and height.*/
    if((encodeWidth == 0) || (encodeHeight == 0))
    {
        return -1;
    }

    /*get osd are mapping(clut addr & area addr.)*/
    areaIndex.areaId  = areaId;
    areaIndex.planeId = planeId;
    retVal = gadi_osd_get_area_mapping(osdHandle, areaIndex, &areaMapping);
	if(retVal != GADI_OK)
	{
		printf("get osd area mapping error.\n");
		return retVal;
	}

    gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));

    if (text->enable)
    {
        GADI_OSD_BitMapAttrT *bitAttr = &bitMapAttr[planeId * OSD_AREA_NUM + areaId];
        retVal = osd_fill_text_clut(text);
        if (retVal != GADI_OK)
        {
            printf("osd_fill_text_clut: fill text clut failed!\n");
            return retVal;
        }

        printf("text string:%s, text length:%d\n", text->textStr, text->textLen);

        retVal = osd_set_text_font(text);
        if (retVal < 0)
        {
            printf("osd_set_text_font failed!\n");
            return retVal;
        }

        osdText.dataLen = areaMapping.areaSize;
        osdText.data = osdBitMap[planeId * OSD_AREA_NUM + areaId] =
            calloc(sizeof(RGBQUAD), osdText.dataLen);

        retVal = osd_convert_text(text, encodeWidth, encodeHeight, 1,
                                        &(dateParams[planeId]), &osdText, 0);
        if (retVal != GADI_OK)
        {
            printf("osd_convert_text: failed !\n");
            return retVal;
        }

        areaParams.planeId = planeId;
        areaParams.areaId  = areaId;
        areaParams.enable  = text->enable;
        areaParams.width   = osdText.width;
        areaParams.height  = osdText.height;
        areaParams.offsetX = ROUND_UP(text->startX * encodeWidth / 100, 4);
        areaParams.offsetY = ROUND_UP(text->startY * encodeHeight / 100, 4);
        bitAttr->alpha = OSD_NONTRANSPARENT;
        bitAttr->bitMapColorFmt = GADI_OSD_COLORFMT_RGB32;
        bitAttr->width = osdText.width;
        bitAttr->height = osdText.height;
        bitAttr->pitch = osdText.width * sizeof(RGBQUAD);
        bitAttr->bitMapAddr = osdText.data;

        gadi_osd_load_bitmap(osdHandle, &areaParams, bitAttr);
        printf("areaParams: width=%d, height=%d, offsetX=%d, offsetY=%d\n",
            areaParams.width, areaParams.height, areaParams.offsetX, areaParams.offsetY);
    }
    else
    {
        gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));
        areaParams.planeId = planeId;
        areaParams.areaId  = areaId;
        areaParams.enable  = text->enable;
        gadi_osd_set_area_params(osdHandle, &areaParams);
    }

    datePar = &(dateParams[planeId]);
    if(text->enable)
    {
        if(osdDateFontSize == 0)
        {
            osdDateFontSize = text->size;
        }
        else
        {
            if(osdDateFontSize != text->size)
            {
                printf("we used only one bitmap for 4 streams' data info. \
                            so the date info font size must the same.");
                return GADI_OSD_ERR_BAD_PARAMETER;
            }
        }

        datePar->enable   = text->enable;
        datePar->fontSize = text->size;
        datePar->areaId   = areaId;

        retVal = osd_create_digit_bitmap(text, &areaIndex);
        if(retVal != GADI_OK)
        {
            return retVal;
        }

        if(osdDateThreadRunning == 0)
        {
            osdDateThreadRunning = 1;
            gadi_sys_thread_create(osd_update_date_thread,
                                   osdHandle,
                                   OSD_DATE_THREAD_PRIORITY,
                                   OSD_DATE_THREAD_STACKSIZE,
                                   OSD_DATE_THREAD_NAME,
                                   &osdDateThreadHandle);
        }
    }
    else
    {
        datePar->enable   = 0;
        datePar->areaId   = 0;
        datePar->fontSize = 0;
    }

    return GADI_OK;
}

int osd_function_exit_show_date(GADI_SYS_HandleT osdHandle)
{
    int retVal = GADI_OK;
    int i;

    /*check input parameters.*/
    if(osdHandle == NULL)
    {
        return -1;
    }

    if(osdDateThreadHandle != 0)
    {
        osdDateThreadRunning = 0;
        retVal = gadi_sys_wait_end_thread(osdDateThreadHandle);
        if (retVal != GADI_OK)
        {
            printf("destroy osd date thread failed %d\n", retVal);
        }
        osdDateThreadHandle = 0;
    }

    for (i = 0; i < OSD_PLANE_NUM * OSD_AREA_NUM; i++) {
        if (osdBitMap[i]) {
            free(osdBitMap[i]);
            osdBitMap[i] = NULL;
        }
    }

    osd_destroy_digit_bitmap();

    return retVal;
}


int osd_function_show_bmp(GADI_SYS_HandleT osdHandle,
                                  int planeId,
                                  int areaId,
                                  OSD_BmpParamsT *bmpPars)
{
    int retVal;
    unsigned int width = 0, height = 0, size = 0, colorCount = 0, cnt = 0;
    unsigned int areaSize, clutSize;
    unsigned char *bmpData = NULL;
    FILE *fp = NULL;
	RGBQUAD rgb;
    OSD_YUVClutT* clutData = NULL;
    GADI_OSD_AreaParamsT  areaParams;
    GADI_OSD_AreaMappingT areaMapping;
    GADI_OSD_AreaIndexT   areaIndex;

    /*check input parameters.*/
    if((osdHandle == NULL) || (bmpPars == NULL) || (bmpPars->bmpFile == NULL))
    {
        printf("invalid input parameters.\n");
        return -1;
    }
    if((planeId >= OSD_PLANE_NUM) || (areaId >= OSD_AREA_NUM))
    {
        printf("invalid planeId=%d or areaId=%d.\n", planeId, areaId);
        return -1;
    }

    /*get osd area mapping(clut addr & area addr.)*/
    areaIndex.areaId  = areaId;
    areaIndex.planeId = planeId;
    retVal = gadi_osd_get_area_mapping(osdHandle, areaIndex, &areaMapping);
	if(retVal != GADI_OK)
	{
		printf("get osd area mapping error.\n");
		return retVal;
	}

    areaSize  = areaMapping.areaSize;
    bmpData   = areaMapping.areaStartAddr;
	gadi_sys_memset(bmpData, 0, areaSize);
    clutSize  = areaMapping.clutSize;
    clutData  = (OSD_YUVClutT*)areaMapping.clutStartAddr;
	gadi_sys_memset(clutData, 0, clutSize);

    if(bmpPars->enable)
    {
		if ((fp = fopen(bmpPars->bmpFile, "r")) == NULL)
        {
			printf("Cannot open bmp file [%s].\n", bmpPars->bmpFile);
			return GADI_OSD_ERR_BAD_PARAMETER;
		}

        retVal = osd_read_bmp_file_header(fp, areaSize,& width, &height,
                                          &size, &colorCount);
        if(retVal != GADI_OK)
        {
            fclose(fp);
            return retVal;
        }

        /*fill bmp clut table and data*/
		for (cnt = 0; cnt < colorCount; cnt++)
        {
			fread(&rgb, sizeof(RGBQUAD), 1, fp);
			osd_rgb_to_yuv(&rgb, &clutData[cnt]);
		}
		for (cnt = 0; cnt < height; cnt++)
        {
			fread(bmpData + (height - 1 - cnt) * width, 1, width, fp);
		}

        fclose(fp);

        areaParams.width   = width;
        areaParams.height  = height;
        areaParams.offsetX = bmpPars->startX;
        areaParams.offsetY = bmpPars->startY;
    }else{
        areaParams.width  = 0;
        areaParams.height  = 0;
        areaParams.offsetX = 0;
        areaParams.offsetY = 0;
    }


    areaParams.planeId = planeId;
    areaParams.areaId  = areaId;
    areaParams.enable  = bmpPars->enable;

    retVal = gadi_osd_set_area_params(osdHandle, &areaParams);
    if(retVal != GADI_OK)
    {
        printf("set area parameters failed !\n");
        return retVal;
    }

    return GADI_OK;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static int osd_fill_text_clut(OSD_TextParamsT *text)
{
    pixelType.pixel_background = osdColoTab[OSD_COLOUR_TRANSPARENT];
    pixelType.pixel_outline    = osdColoTab[textColors[text->color].outline];
    pixelType.pixel_font       = osdColoTab[textColors[text->color].font];

    return GADI_OK;
}

static int osd_set_text_font(const OSD_TextParamsT *text)
{
    font_attribute_t font;

    gadi_sys_memset(&font, 0, sizeof(font));
    strcpy(font.all_path, text->allfontsPath);
    strcpy(font.hlaf_path, text->halffontsPath);
    font.size = text->size;
    font.outline_width = text->outline * 2;
    font.hori_bold = font.vert_bold = text->bold;
    font.italic = text->italic * 50;
    font.disable_anti_alias = 0;

    printf("font parameters: all_path=%s, size=%d, outline_width=%d, hor_bold=%d, italic=%d\n",
              font.all_path,font.size, font.outline_width, font.hori_bold, font.italic);

    if (osd_text_set_font_attribute(&font) < 0)
    {
        printf("osd_text_set_font_attribute failed!\n");
        return -1;
    }

    return GADI_OK;
}

static int osd_convert_text(const OSD_TextParamsT *text,
                                 unsigned short width,
                                 unsigned short height,
                                 unsigned char  dateEnable,
                                 OSD_ShowDateParams *datePar,
                                 OSD_TextDataT *textData,
                                 int equal_interval)
{
    unsigned int cnt;
    unsigned int offsetX, offsetY, maxOffsetX, maxOffsetY;
    unsigned short lineHeight, lines;
    unsigned short boxWidth, boxHeight;
    const char *character = text->textStr;
    RGBQUAD *rgbData = textData->data;
	int use_nbyte = 0;
    maxOffsetY = (100 - text->startY) * height / 100;
    boxHeight = text->boxHeight * height / 100;
    boxHeight = MIN(boxHeight, maxOffsetY);

    maxOffsetX = (100 - text->startX) * width / 100;
    boxWidth  = text->boxWidth * width / 100;
    boxWidth  = MIN(MIN(text->textLen * text->size, maxOffsetX), boxWidth);
    boxWidth  = ROUND_UP(boxWidth, 32);

    lineHeight = text->size;// * 3 / 2;
    lines = offsetX = offsetY = 0;

    if (lineHeight > boxHeight)
    {
        printf("text area box [%dx%d] is too smaller to insert the string [%s] with font size [%d].\n",
            boxWidth, boxHeight, text->textStr, text->size);
        return -1;
    }

    /*setup background*/
    if (memcmp(&pixelType.pixel_background, &osdColoTab[OSD_COLOUR_TRANSPARENT]
        , sizeof(RGBQUAD)) == 0) {
        gadi_sys_memset(textData->data, 0, textData->dataLen);
    } else {
        GADI_S32 index = textData->dataLen / sizeof(RGBQUAD);
        RGBQUAD *colorTable = (RGBQUAD *)textData->data;
        while(index--) {
            colorTable[index] = pixelType.pixel_background;
        }
    }

	LATTICE_INFO lattice_info;
	MAP_INFO map_info;
	map_info.mapaddr = rgbData;
	map_info.color = pixelType;
    for (cnt = 0; cnt < text->textLen; cnt += use_nbyte, character += use_nbyte)
    {
        if ((offsetX + text->size) > boxWidth)
        {
            printf("Text string width [%d] arrives the boundary [%d] "
             "of text area box. Start from a new line.\n", offsetX, boxWidth);
            offsetX  = 0;
            offsetY  += lineHeight;
            rgbData += boxWidth * lineHeight;
            if (offsetY + lineHeight > boxHeight)
            {
                printf("Text string height [%d] arrives the boundary [%d] "
                    "of text area box. Stop add more text.\n", offsetY, boxHeight);
                break;
            }
            ++lines;
        }
        if ((rgbData + boxWidth * lineHeight) > (textData->data + textData->dataLen))
        {
            printf("osd area size [%d] is larger than pre-allocated memory [%d].\n",
                       (rgbData - textData->data + boxWidth * lineHeight),
                       textData->dataLen);
            printf("discard the text line [%d] from character position [%d].\n",
                lines + 1, cnt + 1);
            --lines;
            break;
        }

        map_info.width = boxWidth;
		map_info.height = lineHeight * (lines + 1);
		map_info.startx = offsetX;
		map_info.starty = offsetY;
        if ((use_nbyte = osd_text_convert_character(character, &lattice_info,
			(void*)&map_info)) < 0)
        {
            printf("text2bitmap_convert_character failed !\n");
            return -1;
        }

		if (dateEnable)
        {
            if(datePar != NULL)
            {
                datePar->osdDateOffsetX[cnt] = offsetX;
                datePar->osdDateOffsetY[cnt] = offsetY;
            }
		}
		offsetX += (text->size >> 1) * use_nbyte;
	}
    textData->width  = boxWidth;
    textData->height = lineHeight * (lines + 1);

    return GADI_OK;
}


static int osd_create_digit_bitmap(const OSD_TextParamsT *text,
                                    GADI_OSD_AreaIndexT *areaIndex)
{
    static char date[] = "0123456789";
    unsigned int   totalSize;
    unsigned int   cnt, dateLen;
    unsigned short linePitch, lineHeight;
	LATTICE_INFO lattice_info;
	MAP_INFO map_info;
	int ret = 0;
    GADI_U32 offset = areaIndex->planeId * OSD_AREA_NUM + areaIndex->areaId;

    dateLen = strlen(date);
    linePitch  = (text->size >> 1) * dateLen;
    lineHeight = text->size;
    totalSize  = linePitch * lineHeight;
    if (osdDateBitMap[offset] == NULL)
    {
        osdDateBitMap[offset] = gadi_sys_malloc(totalSize * sizeof(RGBQUAD));
    }
    if (osdDateBitMap[offset] == NULL)
    {
        printf("cannot malloc memory [%d] for osd time update !\n", totalSize);
        return -1;
    }
    /*setup background*/
    if (memcmp(&pixelType.pixel_background, &osdColoTab[OSD_COLOUR_TRANSPARENT]
        , sizeof(RGBQUAD)) == 0) {
        gadi_sys_memset(osdDateBitMap[offset], 0, totalSize * sizeof(RGBQUAD));
    } else {
        GADI_S32 index = totalSize;
        RGBQUAD *colorTable = osdDateBitMap[offset];
        while(index--) {
            colorTable[index] = pixelType.pixel_background;
        }
    }
	map_info.mapaddr = osdDateBitMap[offset];
	map_info.height = lineHeight;
	map_info.width = linePitch;
	map_info.startx = 0;
	map_info.starty = 0;
    map_info.color = pixelType;

    for (cnt = 0; cnt < dateLen; cnt += ret)
    {
        if ((ret = osd_text_convert_character(&date[cnt], &lattice_info,
			(void*)&map_info)) < 0)
        {
            printf("osd_text_convert_character failed !\n");
            return -1;
        }
        map_info.startx += (text->size >> 1) * ret;
    }
    return GADI_OK;
}

static int osd_destroy_digit_bitmap(void)
{
    GADI_S32 i;
    for (i = 0; i < OSD_PLANE_NUM * OSD_AREA_NUM; i++) {
        if (osdDateBitMap[i]) {
            gadi_sys_free(osdBitMap[i]);
            osdDateBitMap[i] = NULL;
        }
    }
    osdDateFontSize = 0;

    return GADI_OK;
}

static int osd_udpate_date_string(GADI_SYS_HandleT osdHandle,
                                         unsigned int plane,
                                         const char *preDate,
                                         const char *curDate)
{
    int retVal;
    RGBQUAD *dest = NULL, *src = NULL;
    GADI_U32 fontSize;
    unsigned short linePitch;
    unsigned int cnt, index, row;
    OSD_ShowDateParams *datePar = NULL;
    GADI_OSD_AreaParamsT  areaParams;
    GADI_OSD_AreaIndexT   areaIndex;
    GADI_OSD_AreaMappingT areaMapping;
    datePar= &(dateParams[plane]);
    GADI_U32 offset = plane * OSD_AREA_NUM + datePar->areaId;

    fontSize = datePar->fontSize;

    areaParams.planeId = plane;
    areaParams.areaId  = datePar->areaId;
    retVal = gadi_osd_get_area_params(osdHandle, &areaParams);
    if(retVal != GADI_OK)
    {
        printf("get osd area params failed.\n");
        return retVal;
    }

    areaIndex.planeId  = plane;
    areaIndex.areaId   = datePar->areaId;
    retVal = gadi_osd_get_area_mapping(osdHandle, areaIndex, &areaMapping);
    if(retVal != GADI_OK)
    {
        printf("get osd area mapping failed.\n");
        return retVal;
    }

    linePitch = areaParams.width;
    if (linePitch == 0)
    {
        return -1;
    }

    for (cnt = 0; curDate[cnt] != '\0'; cnt++)
    {
        if (curDate[cnt] == preDate[cnt])
        {
            continue;
        }
        index = curDate[cnt] - '0';
        dest  = osdBitMap[offset] + datePar->osdDateOffsetX[cnt] +
                datePar->osdDateOffsetY[cnt] * linePitch;
        src = osdDateBitMap[offset] + index * (fontSize >> 1);

        for (row = 0; row < fontSize; ++row)
        {
            gadi_sys_memcpy(dest, src, (fontSize >> 1)*sizeof(RGBQUAD));
            dest += linePitch;
            src += (fontSize >> 1) * 10;
        }
    }

    gadi_osd_load_bitmap(osdHandle, &areaParams, &bitMapAttr[offset]);

    return GADI_OK;
}

static void osd_update_date_thread(void* optArg)
{
    unsigned int  planeId;
    char curDate[OSD_TEXT_MAX_LENGTH], preDate[OSD_TEXT_MAX_LENGTH];
    GADI_SYS_HandleT osdHandle = (GADI_SYS_HandleT)optArg;

    gadi_sys_get_date(preDate);
    while (osdDateThreadRunning)
    {
        gadi_sys_thread_sleep(1000);

        gadi_sys_get_date(curDate);
        for (planeId = 0; planeId < OSD_PLANE_NUM; planeId++)
        {
            if (dateParams[planeId].enable)
            {
                osd_udpate_date_string(osdHandle, planeId, preDate, curDate);
            }
        }
        strncpy(preDate, curDate, OSD_TEXT_MAX_LENGTH);
    }
}

static void osd_rgb_to_yuv(const RGBQUAD * rgb, OSD_YUVClutT *clut)
{
	clut->y = (u8)(0.257f * rgb->r + 0.504f * rgb->g + 0.098f * rgb->b + 16);
	clut->u = (u8)(0.439f * rgb->b - 0.291f * rgb->g - 0.148f * rgb->r + 128);
	clut->v = (u8)(0.439f * rgb->r - 0.368f * rgb->g - 0.071f * rgb->b + 128);
	clut->a = OSD_NONTRANSPARENT;
}

static int osd_read_bmp_file_header(FILE *fp, unsigned int osdSize,
                                   unsigned int *width, unsigned int *height,
                                   unsigned int *size, unsigned int *colorCount)
{
	OSD_BmpHeader fileHeader;
	OSD_BmpInfoHeader infoHeader;
	gadi_sys_memset(&fileHeader, 0, sizeof(OSD_BmpHeader));
	gadi_sys_memset(&infoHeader, 0, sizeof(OSD_BmpInfoHeader));

	fread(&fileHeader.bfType, sizeof(fileHeader.bfType), 1, fp);
	fread(&fileHeader.bfSize, sizeof(fileHeader.bfSize), 1, fp);
	fread(&fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1), 1, fp);
	fread(&fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2), 1, fp);
	fread(&fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits), 1, fp);
	if (fileHeader.bfType != OSD_BMP_FILE_MAGIC_NO)
    {
		printf("this is not a BMP file.\n");
		return -1;
	}

	fread(&infoHeader, sizeof(OSD_BmpInfoHeader), 1, fp);
	if (infoHeader.biBitCount > OSD_BMP_FILE_BIT_COUNT)
    {
		printf("only support 8bit BMP file.\n");
		return -1;
	}
	if (infoHeader.biSizeImage != (infoHeader.biWidth * infoHeader.biHeight))
    {
		printf("BMP file size [%d] is not equal to [%dx%d].\n",
			infoHeader.biSizeImage, infoHeader.biWidth, infoHeader.biHeight);
		return -1;
	}
	if ((infoHeader.biWidth & 0x1F) || (infoHeader.biHeight & 0x3))
    {
		printf("BMP width [%d] must be multiple of 32, height [%d] must be multiple of 4.\n",
			infoHeader.biWidth, infoHeader.biHeight);
		return -1;
	}
	if (infoHeader.biSizeImage > osdSize)
    {
		printf("BMP file size [%d] is larger than pre-allocated memory [%d].\n",
			infoHeader.biSizeImage, osdSize);
		return -1;
	}
	*width = infoHeader.biWidth;
	*height = infoHeader.biHeight;
	*size = infoHeader.biSizeImage;
	*colorCount = (fileHeader.bfOffBits - OSD_BITMAPFILE_HEADER_SIZE - OSD_BITMAPINFO_HEADER_SIZE) / sizeof(RGBQUAD);
	printf("BMP file header : size %d = %d x %d, color count %d.\n",
		*size, *width, *height, *colorCount);

	return GADI_OK;
}

