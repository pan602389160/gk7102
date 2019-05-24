/******************************************************************************
** \file        adi/test/src/osd.c
**
**
** \brief        ADI layer osd test.
**
** \attention    THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**                ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**                OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <getopt.h>

#include "shell.h"
#include "osd.h"
#include "venc.h"
#include "osd_functions.h"
#include "adi_osd.h"
#include "adi_types.h"
#include "adi_sys.h"
#include "venc.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************

#define TEST_OK(format, args...)    printf("\033[1;32m" format "\033[0m", ##args)
#define TEST_FAIL(format, args...)  printf("\033[1;31m" format "\033[0m", ##args)


//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************


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
}__attribute__ ((packed)) OSD_BmpHeader;

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
}__attribute__ ((packed)) OSD_BmpInfoHeader;


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
GADI_SYS_HandleT osdHandle = NULL;

static const char *shortOptions = "htp:ds:a:e:S:c:o:b:i:x:y:IOCElv:";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"text",     0, 0, 't'},
    {"picture",  0, 0, 'p'},
    {"date",     0, 0, 'd'},
    {"stream",     0, 0, 's'},
    {"area",     0, 0, 'a'},
    {"enable",     0, 0, 'e'},
    {"size",     0, 0, 'S'},
    {"color",     0, 0, 'c'},
    {"outline",  0, 0, 'o'},
    {"bold",     0, 0, 'b'},
    {"italic",     0, 0, 'i'},
    {"startx",     0, 0, 'x'},
    {"starty",     0, 0, 'y'},
    {"logo",     0, 0, 'l'},
    {"invert",     1, 0, 'v'},
    {"init",     0, 0, 'I'},
    {"open",     0, 0, 'O'},
    {"close",     0, 0, 'C'},
    {"exit",     0, 0, 'E'},
    {0,          0, 0, 0}
};

static int showTextFlag = 0;
static int showDateFlag = 0;
static int showPicFlag    = 0;
static int planeId      = 0;
static int areaId       = 0;
static int lumThreash   = 0;
static char string[] = "Main stream OSD test!!!";
static OSD_TextParamsT textPar =
{
    .enable    = 1,
    //use vector front library.
#if 0
    .allfontsPath = "/usr/share/fonts/Vera.ttf",
#else
    //use lattice front library.
    .halffontsPath     = "/etc/videoconfig/asc32",
    .allfontsPath     = "/etc/videoconfig/hzk32",
#endif
    .size       = 32,
    .color       = 0,//black
    .outline   = 1,
    .bold       = 0,
    .italic    = 0,
    .startX    = 0,
    .startY    = 0,
    .boxWidth  = 50,
    .boxHeight = 50,
};

static OSD_BmpParamsT bmpPars =
{
    .enable    = 1,
    .startX    = 0,
    .startY    = 0,
    .bmpFile   = NULL,//"/root/gokelogo.bmp",
};


//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void usage(void);
static int handle_osd_command(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
int osd_init(void)
{
    int retVal;

    retVal = gadi_osd_init();

    return retVal;
}

int osd_exit(void)
{
    int retVal;

    retVal = gadi_osd_exit();

    return retVal;
}

int osd_open(void)
{
    GADI_ERR retVal = GADI_OK;

    osdHandle = gadi_osd_open(&retVal);
    if(retVal != GADI_OK)
    {
        printf("gadi_osd_open error\n");
        return retVal;
    }

    osd_function_init(OSD_LATTICE_FONT_LIB);

    return retVal;
}

int osd_close(void)
{
    int retVal;

    osd_function_exit();
    osd_function_exit_show_date(osdHandle);
    retVal = gadi_osd_close(osdHandle);

    return retVal;
}

int osd_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "osd",
        handle_osd_command,
        "osd command",
        "---------------------------------------------------------------------\n"
        "osd -t XXXXXX\n"
        "    brief : display text.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "osd -p picture.bmp \n"
        "    brief : display date info.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "osd -d \n"
        "    brief : display date info.\n"
        "\n"
    );

    return retVal;
}


void osd_draw_square(OSD_SQUARE_AttrT *attr)
{
    GADI_ERR retVal = GADI_OK;
    GADI_U32 realLen = 0, intervalLen = 0;;
    GADI_U16  i, j;
    GADI_U8  minX = 0, minY = 0;
    GADI_OSD_AreaIndexT areaIndex;
    GADI_OSD_AreaMappingT areaMap;
    GADI_OSD_AreaParamsT areaParams;
    OSD_YUVClutT *clut = NULL;

    gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));

    GADI_U32 capLen0 = 0, capLen1 = 0;
    if(attr->realWidth - attr->offsetX < attr->offsetX){
        minX = 1;
        capLen0 = attr->realWidth - attr->offsetX;
    }else{
        minX = 0;
        capLen0 = attr->offsetX;
    }

    if(attr->realHeight - attr->offsetY < attr->offsetY){
        minY = 1;
        capLen1 = attr->realHeight - attr->offsetY;
    }else{
        minY = 0;
        capLen1 = attr->offsetY;
    }

    realLen = MIN(capLen0, capLen1);
    realLen = MIN(((attr->lenth) >> 1), realLen);

    realLen <<= 1;

//    realLen &= (~0x1f);
    if(realLen > 256)
        realLen = 256;

    if(realLen <= 0){
        //printf("real lenth equal 0\n");
        return;
    }
    intervalLen = ROUND_UP(realLen, 32);
    if(attr->enable) {
        areaIndex.areaId = attr->area;
        areaIndex.planeId = attr->streamId;


        retVal = gadi_osd_get_area_mapping(osdHandle, areaIndex, &areaMap);
        if (retVal != GADI_OK) {
            GADI_ERROR("get area map error\n");
            return;
        }
        clut = (OSD_YUVClutT *)areaMap.clutStartAddr;
        for (i = 0; i < 256; i++) {
            clut[i].y = 235;
            clut[i].u = 128;
            clut[i].v = 128;
            clut[i].a = i;
        }
        gadi_sys_memset(areaMap.areaStartAddr , 0, areaMap.areaSize);
    }

    areaParams.planeId = attr->streamId;
    areaParams.areaId  = attr->area;
    areaParams.width  = areaParams.height  = realLen;//ROUND_UP(realLen, 4);
    if(minX > 0){
        areaParams.offsetX = (attr->offsetX - (realLen>>1))&(~0x3);
    }
    else {
        areaParams.offsetX = ROUND_UP(attr->offsetX - (realLen>>1), 4);
    }
    if(minY > 0) {
        areaParams.offsetY = (attr->offsetY - (realLen>>1))&(~0x3);
    }else {
        areaParams.offsetY = ROUND_UP(attr->offsetY - (realLen>>1), 4);
    }

    if(attr->enable) {

        for (i = 0; i < realLen; i++) {
            for(j = 0; j < realLen; j++){
                if(i == 0|| i == realLen - 1 || j == 0 || j == realLen - 1)
                    areaMap.areaStartAddr[i * intervalLen + j] = attr->alpha;
            }
        }
    }
    areaParams.enable  = attr->enable;
    gadi_osd_set_area_params(osdHandle,&areaParams);

}

void osd_draw_image_by_path(const char *filepath, int streamid, int areaid,
        unsigned int offsetx, unsigned int offsety)
{
    GADI_OSD_AreaParamsT areaParams;
    GADI_OSD_BitMapAttrT bitMapAttr;
    OSD_BmpHeader bmpHeader;
    OSD_BmpInfoHeader bmpInfo;
    GADI_U32 bitMapSize;

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        GADI_ERROR("Open logo bitmap file failed\n");
        return ;
    }

    read(fd, &bmpHeader, sizeof(bmpHeader));
    read(fd, &bmpInfo, sizeof(bmpInfo));

    switch(bmpInfo.biBitCount) {
        case 32:bitMapAttr.bitMapColorFmt = GADI_OSD_COLORFMT_RGB32;break;
        case 24:bitMapAttr.bitMapColorFmt = GADI_OSD_COLORFMT_RGB24;break;
        case 16:bitMapAttr.bitMapColorFmt = GADI_OSD_COLORFMT_RGB565;break;
        default:
            GADI_ERROR("biBitCount %d no match\n", bmpInfo.biBitCount);
            close(fd);
            return;
    }

    //fill bmp info
    bitMapAttr.alpha = 255;//no transparent.
    bitMapAttr.height = bmpInfo.biHeight;
    bitMapAttr.width = bmpInfo.biWidth;
    bitMapAttr.pitch = (((bitMapAttr.width*(bmpInfo.biBitCount/8) + 3)>>2)<<2);// 4byte align in bmp
    bitMapSize = bitMapAttr.height*bitMapAttr.pitch;
    bitMapAttr.bitMapAddr = malloc(bitMapSize);
    memset(bitMapAttr.bitMapAddr, 0, bitMapSize);

    //fill bmp data
    int h_index;
    unsigned char *data = (char *)(bitMapAttr.bitMapAddr) + bitMapSize;
    lseek(fd, bmpHeader.bfOffBits, SEEK_SET);//jump read bitmap header
    for (h_index = 0; h_index < bitMapAttr.height; h_index++) {
        data -= bitMapAttr.pitch;
        read(fd, data, bitMapAttr.pitch);
    }
    //read(fd, bitMapAttr.bitMapAddr, bitMapSize);
    close(fd);

    gadi_sys_memset(&areaParams, 0, sizeof(GADI_OSD_AreaParamsT));
    areaParams.enable  = 1;
    areaParams.areaId = areaid;
    areaParams.planeId = streamid;
    areaParams.offsetX = offsetx;
    areaParams.offsetY = offsety;
    areaParams.width = bitMapAttr.width;
    areaParams.height = bitMapAttr.height;

    if (gadi_osd_load_bitmap(osdHandle, &areaParams, &bitMapAttr)) {
        printf("gadi_osd_set_bitmap error\n");
        return;
    }
    free(bitMapAttr.bitMapAddr);
}
//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void usage(void)
{
    printf("\nusage: osd [OPTION]...[Text String]...\n");
    printf("\t-h, --help                help\n"
           "\t-I, --init                init osd\n"
           "\t-O, --open                open osd\n"
           "\t-C, --close               close osd\n"
           "\t-E, --exit                exit osd\n"
           "\t-t, --text                show text\n"
           "\t-p, --picture             show picture\n"
           "\t-d, --date                show date info\n"
           "\t-s, --streamId            osd in which stream/plane (0~3)\n"
           "\t-a, --areaId              osd in which area (0~2)\n"
           "\t-e, --enable              osd enable/disable(1/0)\n"
           "\t-S, --size                font size in osd (12,24,32...)\n"
           "\t-c, --colour              font colour in osd(0~7)\n"
           "\t-o, --outline             font outline width(0,1,2...)\n"
           "\t-b, --blod                font blod(0:normal, 1:bold)\n"
           "\t-l, --logo                Display Goke logo on main stream.\n"
           "\t-i, --italic              font italic(0:normal, 1:50%% italic.)\n"
           "\t-x, --startX              text area offset x, 0~100, 100 means 100%% of encode width.\n"
           "\t-y, --startY              text area offset y, 0~100, 100 means 100%% of encode height.\n"
           "\t-v, --invert              setup invert OSD and invert threshold.\n");

    printf("example:\n");
    printf("\tosd \n"
           " --- display text in osd: in stream 0/font size 32/font colour black/text sting:XXXX\n"
           "\tosd -t -s 0 -a 0 -e 1 -S 32 -c 6 -o 0 -x 0 -y 0 XXXXXX\n"
           " --- display date info in osd: in stream 0/font size 32/font colour black/text sting:XXXX\n"
           "\tosd -d -s 0 -a 1 -e 1 -S 32 -c 6 -o 0 -x 0 -y 0\n"
           " --- display picture in osd:\n"
           "\tosd -s 0 -a 2 -p *.bmp\n");

    printf("\n");
}

static int handle_osd_command(int argc, char* argv[])
{
    int option_index, ch;
    int retVal;
    GADI_VENC_StreamFormatT formatPar;
    optind = 1;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                usage();
                goto command_exit;
            case 'l':
                osd_draw_image_by_path("/etc/videoconfig/gokelogo.bmp", 0, 0, 8, 8);
                break;
            case 't':
                showTextFlag = 1;
                showPicFlag  = 0;
                showDateFlag = 0;
                break;

            case 'v':
                lumThreash = atoi(optarg);
                {
                    GADI_OSD_AreaIndexT areaIndex;
                    GADI_OSD_ChannelAttrT chnAttr;
                    areaIndex.planeId = planeId;
                    areaIndex.areaId = areaId;
                    if (lumThreash >= 0) {
                        chnAttr.invtColorEn = GADI_TRUE;
                        chnAttr.lumThreshold = lumThreash;
                    } else {
                        chnAttr.invtColorEn = GADI_FALSE;
                        chnAttr.lumThreshold = -lumThreash;
                    }
                    chnAttr.invtAreaHeight = 32;
                    chnAttr.invtAreaWidth = 16;
                    chnAttr.invtColorMode = GADI_OSD_LT_LUM_THRESH;
                    gadi_osd_set_channel_attr(osdHandle, &areaIndex, &chnAttr);
                    lumThreash = 0;
                }
                break;

            case 'p':
                showPicFlag  = 1;
                showTextFlag = 0;
                showDateFlag = 0;
                {
                GADI_U32 len = strlen(optarg);
                if(bmpPars.bmpFile != NULL){
                    gadi_sys_free(bmpPars.bmpFile);
                    bmpPars.bmpFile = NULL;
                }
                bmpPars.bmpFile = gadi_sys_malloc(len+1);
                if(bmpPars.bmpFile == NULL) {
                    showPicFlag  = 0;
                    break;
                }
                gadi_sys_memset(bmpPars.bmpFile, 0, len+1);
                gadi_sys_memcpy(bmpPars.bmpFile, optarg, len);
                }
                break;

            case 'd':
                showDateFlag = 1;
                showPicFlag  = 0;
                showTextFlag = 0;
                break;

            case 's':
                planeId = atoi(optarg);
                if(planeId >= 4)
                {
                    printf("bad input stream index :%d\n", planeId);
                    goto command_exit;
                }
                break;

            case 'a':
                areaId = atoi(optarg);
                if(areaId >= 3)
                {
                    printf("bad input area index :%d\n", areaId);
                    goto command_exit;
                }
                break;

            case 'e':
                textPar.enable = atoi(optarg);
                if((textPar.enable != 0) && (textPar.enable != 1))
                {
                    printf("bad input text enable :%d\n", textPar.enable );
                    goto command_exit;
                }
                break;

            case 'S':
                textPar.size = atoi(optarg);
                break;

            case 'c':
                textPar.color= atoi(optarg);
                if(textPar.color >= OSD_COLOUR_NUMBERS)
                {
                    printf("bad input text colour :%d\n", textPar.color );
                    goto command_exit;
                }
                printf("input text colour :%d\n", textPar.color );
                break;

            case 'o':
                textPar.outline = atoi(optarg);
                break;


            case 'b':
                textPar.bold = atoi(optarg);
                break;

            case 'i':
                textPar.italic = atoi(optarg);
                break;

            case 'x':
                textPar.startX = atoi(optarg);
                if(textPar.startX > 100)
                {
                    printf("bad input text offset x :%d\n", textPar.startX);
                    goto command_exit;
                }
                break;

            case 'y':
                textPar.startY = atoi(optarg);
                if(textPar.startY > 100)
                {
                    printf("bad input text offset y :%d\n", textPar.startY);
                    goto command_exit;
                }
                break;
            case 'I':
                if (GADI_OK != osd_init()) {
                    GADI_ERROR("osd init error\n");
                    goto command_exit;
                }
                GADI_INFO("osd init successful\n");
                break;
            case 'O':
                if (GADI_OK != osd_open()) {
                    GADI_ERROR("osd init open\n");
                    goto command_exit;
                }
                GADI_INFO("osd open successful\n");
                break;
            case 'C':
                if (GADI_OK != osd_close()) {
                    GADI_ERROR("osd init close\n");
                    goto command_exit;
                }
                GADI_INFO("osd close successful\n");
                break;
            case 'E':
                if (GADI_OK != osd_exit()) {
                    GADI_ERROR("osd init exit\n");
                    goto command_exit;
                }
                GADI_INFO("osd exit successful\n");
                break;
            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

    if(showTextFlag == 1)
    {
        if(optind <= argc -1)
        {
            strcpy(string, argv[optind]);
            optind ++;
            while(optind <= argc -1)
            {
                strcat(string, " ");
                strcat(string, argv[optind]);
                optind ++;
            }
        }

        printf("osd display text string:%s\n",string);

        textPar.textLen = strlen(string);
        strncpy(textPar.textStr, string, sizeof(textPar.textStr));

        formatPar.streamId = planeId;
        retVal = gdm_venc_get_stream_format(&formatPar);
        if(retVal != GADI_OK)
        {
            printf("get stream format failed.");
            goto command_exit;
        }

        retVal = osd_function_show_text(osdHandle, formatPar.width, formatPar.height,
                                       planeId, areaId, &textPar);
        if(retVal != 0)
        {
            printf("show text error.");
        }
        showTextFlag = 0;
    }

    if(showDateFlag == 1)
    {
        gadi_sys_get_date(textPar.textStr);
        textPar.textLen = strlen(textPar.textStr);

        formatPar.streamId = planeId;
        retVal = gdm_venc_get_stream_format(&formatPar);
        if(retVal != GADI_OK)
        {
            printf("get stream format failed.");
            goto command_exit;
        }
        retVal = osd_function_show_date(osdHandle, formatPar.width, formatPar.height,
                                        planeId, areaId, &textPar);
        if(retVal != 0)
        {
            printf("show date error.");
        }
        showDateFlag = 0;
    }

    if(showPicFlag == 1)
    {
        printf("loading \"%s\".\n", bmpPars.bmpFile);
        osd_draw_image_by_path(bmpPars.bmpFile, planeId, areaId, bmpPars.startX, bmpPars.startY);
        gadi_sys_free(bmpPars.bmpFile);
        bmpPars.bmpFile = NULL;
        showPicFlag = 0;
    }


command_exit:
    optind = 1;
    return 0;
}


