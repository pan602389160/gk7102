/*!
*****************************************************************************
** \file        /test/src/md.c
**
**
** \brief       motion detection test demo
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adi_types.h"
#include "adi_sys.h"
#include "adi_pda.h"
#include "adi_vout.h"

#include <unistd.h>
#include <fcntl.h>

#include <getopt.h>

#include "shell.h"
#include "venc.h"
#include "osd.h"
#include "osd_functions.h"

#include "tidestop_book.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************
#define MAX_CAP_FRAEM_SIZE 1920*1080


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
static GADI_SYS_HandleT handle = 0;
static GADI_U8 threadExist = 0;
static GADI_U8 pdaStart = 0;
static GADI_U8 pdaOpen = 0;
static GADI_SYS_ThreadHandleT pdaThreadNum = 0;

static OSD_TextParamsT textPar =
{
    .enable    = 1,
    .size       = 32,
    .color       = 6,
    .outline   = 0,
    .bold       = 0,
    .italic    = 0,
    .startX    = 0,
    .startY    = 0,
    .boxWidth  = 50,
    .boxHeight = 50,
};
static GADI_VENC_StreamFormatT formatPar;

static const char *shortOptions = "hSPI:OCEy:Y:ltb:i:v:o:";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"start",    0, 0, 'S'},
    {"stop",     0, 0, 'P'},
    {"init",     1, 0, 'I'},
    {"open",     0, 0, 'O'},
    {"close",    0, 0, 'C'},
    {"exit",     0, 0, 'E'},
    {"yuv",      1, 0, 'y'},
    {"bmp",      1, 0, 'Y'},
    {"look",     0, 0, 'l'},
    {"set",      0, 0, 't'},
    {"bufnum",   1, 0, 'b'},
    {"Intvel",   1, 0, 'i'},
    {"threshlod",1, 0, 'v'},
    {"osd"       ,1, 0, 'o'},
    {0,          0, 0, 0}
};
static GADI_PDA_AttrU attr =
{
            .mdAttr.mdAlg = GADI_PDA_MD_ALG_REF,
            .mdAttr.mbSize = GADI_PDA_MD_MB_16PIXEL,
            .mdAttr.mbSadBits = GADI_PDA_MD_MB_SAD_16BIT,
            .mdAttr.mdBufNum = 10,
            .mdAttr.mdFrameIntvel = 5,
};
static volatile GADI_U8 useSet = 0;
static volatile GADI_U32 threshlodValue = 0x10;
static volatile GADI_BOOL useOSD = GADI_FALSE;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void pda_thread(void *data);
static GADI_ERR pda_p_handle_cmd(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************



typedef struct BIT_MAP_FILE_HEADER
{
    GADI_U8 type[2]; // "BM" (0x4d42)
    GADI_U32 file_size;
    GADI_U32 reserved_zero;
    GADI_U32 off_bits; // data area offset to the file set (unit. byte)
    GADI_U32 info_size;
    GADI_U32 width;
    GADI_U32 height;
    GADI_U16 planes; // 0 - 1
    GADI_U16 bit_count; // 0 - 1
    GADI_U32 compression; // 0 - 1
    GADI_U32 size_image; // 0 - 1
    GADI_U32 xpels_per_meter;
    GADI_U32 ypels_per_meter;
    GADI_U32 clr_used;
    GADI_U32 clr_important;
}__attribute__((packed)) BIT_MAP_FILE_HEADER_t; //



GADI_U32 gk_yuv420sem_to_bitmap888(const void *yuv420sem_y,const void *yuv420sen_uv, GADI_S32 yuv_width, GADI_S32 yuv_height, GADI_S32 yuv_stride,
    void *bmp888)
{
    GADI_S32 i = 0, ii = 0;

    // allocate the bitmap data
    GADI_S32 bitmap24_size = yuv_width * yuv_height * 3;
    GADI_U8 *bitmap_offset = bmp888;

    printf("BMP [%dx%d] stride %d\n", yuv_width, yuv_height, yuv_stride);

    if(yuv420sem_y){
        GADI_S32 y, u, v, yy, vr, ug, vg, ub;
        GADI_S32 r, g, b;
        const GADI_U8 *py = (GADI_U8*)(yuv420sem_y);
        const GADI_U8 *puv = (GADI_U8*)(yuv420sen_uv);

        // yuv420 to rgb888
        for(i = 0; i < yuv_height; ++i)
        {
            py  = (GADI_U8*)(yuv420sem_y + yuv_stride * i);
            puv  = (GADI_U8*)(yuv420sen_uv + yuv_stride * (i>>1));
            for(ii = 0; ii < yuv_width; ++ii){

                y = py[0];
                yy = y * 256;

                u = puv[0] - 128;
                ug = 88 * u;
                ub = 454 * u;

                v = puv[1] - 128;
                vg = 183 * v;
                vr = 359 * v;

                ///////////////////////////////////
                // convert
                r = (yy + vr) >> 8;
                g = (yy - ug - vg) >> 8;
                b = (yy + ub) >> 8;

                if(r < 0){
                    r = 0;
                }
                if(r > 255){
                    r = 255;
                }
                if(g < 0){
                    g = 0;
                }
                if(g > 255){
                    g = 255;
                }
                if(b < 0){
                    b = 0;
                }
                if(b > 255){
                    b = 255;
                }


                *bitmap_offset++ = (GADI_U8)b;
                *bitmap_offset++ = (GADI_U8)g;
                *bitmap_offset++ = (GADI_U8)r;

                ///////////////////////////////////
                ++py;
                if(0 != (ii % 2)){
                    // even
                    puv += 2;
                }
            }
        }
        return bitmap24_size;
    }
    return -1;
}
/*
static GADI_S32 pda_set_attr(int argc, char *argv[])
{
    GADI_S32 retVal;
    GADI_PDA_AttrU attr;

    gadi_sys_memset(&attr, 0, sizeof(attr));
    attr.mdAttr.mdAlg = (GADI_PDA_MD_Alg_t)atoi(argv[2]);
    attr.mdAttr.mbSize = (GADI_PDA_MD_MB_SizeT)atoi(argv[3]);
    attr.mdAttr.mbSadBits = (GADI_PDA_MD_MB_SAD_BitT)atoi(argv[4]);
    attr.mdAttr.mdBufNum = atoi(argv[5]);
    attr.mdAttr.mdFrameIntvel = atoi(argv[6]);
    GADI_INFO("pda attr:\n"
                    "\tmdAlg = %s\n"
                    "\tmbSize = %s\n"
                    "\tmbSadBits = %s\n"
                    "\tmdBufNum = %d\tmdFrameIntvel = %d\n",
                    attr.mdAttr.mdAlg == GADI_PDA_MD_ALG_REF?"GADI_PDA_MD_ALG_REF":"GADI_PDA_MD_ALG_NUM",
                    attr.mdAttr.mbSize == GADI_PDA_MD_MB_16PIXEL?"GADI_PDA_MD_MB_16PIXEL":"GADI_PDA_MD_MB_8PIXEL",
                    attr.mdAttr.mbSadBits == GADI_PDA_MD_MB_SAD_16BIT?"GADI_PDA_MD_MB_SAD_16BIT":"GADI_PDA_MD_MB_SAD_8BIT",
                    attr.mdAttr.mdBufNum,
                    attr.mdAttr.mdFrameIntvel);

    retVal = gadi_pda_set_attr(handle, &attr);
    if (retVal != GADI_OK) {
        GADI_INFO("pda:set attr fail\n");
    }
    return retVal;
}*/

static GADI_S32 pda_print_attr(void)
{
    GADI_S32 retVal;
    GADI_PDA_AttrU attr;
    GADI_PDA_Buffer_InfoT bufferInfo;

    gadi_sys_memset(&attr, 0, sizeof(attr));
    retVal = gadi_pda_get_attr(handle, &attr);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_pda_get_attr fail\n");;
    } else {
        GADI_INFO("gadi_pda_get_attr ok\n");
        GADI_INFO("pda attr:\n"
                    "\tmdAlg = %s\n"
                    "\tmbSize = %s\n"
                    "\tmbSadBits = %s\n"
                    "\tmdBufNum = %d\tmdFrameIntvel = %d\n",
                    attr.mdAttr.mdAlg == GADI_PDA_MD_ALG_REF?"GADI_PDA_MD_ALG_REF":"GADI_PDA_MD_ALG_NUM",
                    attr.mdAttr.mbSize == GADI_PDA_MD_MB_16PIXEL?"GADI_PDA_MD_MB_16PIXEL":"GADI_PDA_MD_MB_8PIXEL",
                    attr.mdAttr.mbSadBits == GADI_PDA_MD_MB_SAD_16BIT?"GADI_PDA_MD_MB_SAD_16BIT":"GADI_PDA_MD_MB_SAD_8BIT",
                    attr.mdAttr.mdBufNum,
                    attr.mdAttr.mdFrameIntvel);
    }
    retVal = gadi_pda_buffer_info(&bufferInfo);
    if (retVal != GADI_OK) {
        GADI_ERROR("gadi_pda_buffer_info fail\n");;
    } else {
        GADI_INFO("gadi_pda_buffer_info ok\n");
        GADI_INFO("[buffer info]width:%d height:%d stride:%d\n",
            bufferInfo.width, bufferInfo.height, bufferInfo.stride);
    }

    return GADI_OK;

}


static GADI_S32 pda_capture_bmp(GADI_U32 channelIndex)
{

    GADI_PDA_Frame_InfoT video_frame_info;
    GADI_PDA_CAP_Buffer_TypeT buffer = GADI_PDA_CAP_MAIN_BUFFER;
    FILE *bitmap_stream = fopen("/opt/pda_888.bmp","wb");

    if(bitmap_stream == NULL)
    {
        GADI_ERROR("pda_capture_bmp: open file error\n");
        return -1;
    }
    if (0 == channelIndex)
    {
        buffer = GADI_PDA_CAP_MAIN_BUFFER;
    }
    else if (1 == channelIndex)
    {
        buffer = GADI_PDA_CAP_SECOND_BUFFER;
    }
    else if (3 == channelIndex)
    {
        buffer = GADI_PDA_CAP_FOURTH_BUFFER;
    }

    if(gadi_pda_capture_buffer(buffer, &video_frame_info) != GADI_OK)
    {
        GADI_ERROR("pda_capture_bmp: failed\n");
        fclose(bitmap_stream);
        return -1;
    }
    else
    {
        GADI_INFO("gadi_pda_capture_buffer ok\n");
        if(FRAME_FORMAT_YUV_SEMIPLANAR_420 == video_frame_info.frameFormat)
        {
            GADI_S32 const width = video_frame_info.width;
            GADI_S32 const height = video_frame_info.height;
            GADI_S32 const stride = video_frame_info.stride;
            GADI_S32 i;
            // allocate the bitmap data
            GADI_S32 bitmap24_size = width * height * 3;
            GADI_U8* bitmap24_data = malloc(bitmap24_size);

            GADI_INFO("Frame [%dx%d] stride %d\n",  width, height, stride);

            gk_yuv420sem_to_bitmap888(video_frame_info.yAddr,video_frame_info.uvAddr, width, height, stride, bitmap24_data);

            if(0 == fseek(bitmap_stream, 0, SEEK_SET))
            {
                BIT_MAP_FILE_HEADER_t bmp_header;
                memset(&bmp_header, 0, sizeof(bmp_header));

                bmp_header.type[0] = 'B';
                bmp_header.type[1] = 'M';
                bmp_header.file_size = sizeof(bmp_header) + bitmap24_size;
                bmp_header.reserved_zero = 0;
                bmp_header.off_bits = sizeof(bmp_header);
                bmp_header.info_size = 40;
                bmp_header.width = width;
                bmp_header.height = height;
                bmp_header.planes = 1;
                bmp_header.bit_count = 24;
                bmp_header.compression = 0;
                bmp_header.size_image = bitmap24_size;
                bmp_header.xpels_per_meter = 0;
                bmp_header.ypels_per_meter = 0;
                bmp_header.clr_used = 0;
                bmp_header.clr_important = 0;

                fwrite(&bmp_header, 1, sizeof(bmp_header), bitmap_stream);
                for(i = 0; i < height; ++i){
                    void* bitmap_offset = bitmap24_data + (height - i - 1) * width * 3;
                    fwrite(bitmap_offset, 1, width * 3, bitmap_stream);
                }
                free(bitmap24_data);
                fclose(bitmap_stream);
                GADI_INFO("caputre frame success,at /opt/pda_888.bmp\n");
                return 0;
            }
        }
    }
    return -1;
}

static inline GADI_S32 delete_padding_from_strided_y
    (GADI_U8* output_y, const GADI_U8* input_y, GADI_S32 pitch, GADI_S32 width, GADI_S32 height)
{
    GADI_S32 row;
    for (row = 0; row < height; row++) {         //row
        memcpy(output_y, input_y, width);
        input_y = input_y + pitch;
        output_y = output_y + width ;
    }
    return 0;
}

static inline GADI_S32 delete_padding_and_deinterlace_from_strided_uv
    (GADI_U8* output_uv, const GADI_U8* input_uv, GADI_S32 pitch, GADI_S32 width, GADI_S32 height,GADI_U8 isPannel)
{
    GADI_S32 row, i;
    GADI_U8 * output_u = output_uv;
    GADI_U8 * output_v = output_uv + width * height;    //without padding

    for (row = 0; row < height; row++) {         //row
        for (i = 0; i < width; i++) {
            if (isPannel) {
                // U buffer and V buffer is plane buffer
                *output_u++ = *input_uv++;       //U Buffer
                *output_v++ =  *input_uv++;        //V buffer
            } else {
                // U buffer and V buffer is interlaced buffer
                *output_u++ = *input_uv++;
                *output_u++ = *input_uv++;
            }
        }
        input_uv += (pitch - width) * 2;        //skip padding
    }
    return 0;
}

int turing_pda_get_y_12(GADI_U32 channelIndex,char *yBuffer)
{
	GADI_PDA_Frame_InfoT info;

	GADI_U8 * uvBuffer = NULL;

	GADI_U8 fileName[64];
	GADI_S32 uv_width, uv_height,uv_pitch;
	GADI_PDA_CAP_Buffer_TypeT buffer = GADI_PDA_CAP_MAIN_BUFFER;

	if (0 == channelIndex)
	{
		buffer = GADI_PDA_CAP_MAIN_BUFFER;
	}
	else if (1 == channelIndex)
	{
		buffer = GADI_PDA_CAP_SECOND_BUFFER;
	}
	else if (3 == channelIndex)
	{
		buffer = GADI_PDA_CAP_FOURTH_BUFFER;
	}


	uvBuffer = malloc(MAX_CAP_FRAEM_SIZE);
	if (uvBuffer == NULL) {
		printf("Not enough memory for UV buffer:%dKB !\n",MAX_CAP_FRAEM_SIZE);
		goto error_exit;
	}

	if(gadi_pda_capture_buffer(buffer, &info) != GADI_OK)
	{
		GADI_ERROR("pda_capture_bmp: failed\n");

		goto error_exit;
	}

	if (info.stride== info.width)
	{
		memcpy(yBuffer, info.yAddr, info.width * info.height);
	} else if (info.stride > info.width)
	{
		delete_padding_from_strided_y(yBuffer,
		info.yAddr, info.stride, info.width, info.height);
	} else
	{
		printf("stride size smaller than width!\n");
		goto error_exit;
	}

	//convert uv data from interleaved into planar format
	if (FRAME_FORMAT_YUV_SEMIPLANAR_420 == info.frameFormat) {
		uv_pitch = info.stride/ 2;
		uv_width = info.width / 2;
		uv_height = info.height / 2;
	} else { // FRAME_FORMAT_YUV_SEMIPLANAR_422
		uv_pitch = info.stride/ 2;
		uv_width = info.width / 2;
		uv_height = info.height;
	}

	delete_padding_and_deinterlace_from_strided_uv(uvBuffer,
		info.uvAddr, uv_pitch, uv_width, uv_height,1);

	return 0;
error_exit:

	if(uvBuffer != NULL)
	{
		free(uvBuffer);
		uvBuffer = 0;
	}


	return -1;
}


 void pda_cap_yuv_12(GADI_U32 channelIndex)
{
    GADI_PDA_Frame_InfoT info;
    FILE *yuv_stream = NULL;
    GADI_U8 * uvBuffer = NULL;
    GADI_U8 * yBuffer = NULL;
    GADI_U8 fileName[64];
    GADI_S32 uv_width, uv_height,uv_pitch;
    GADI_PDA_CAP_Buffer_TypeT buffer = GADI_PDA_CAP_MAIN_BUFFER;

    if (0 == channelIndex)
    {
        buffer = GADI_PDA_CAP_MAIN_BUFFER;
    }
    else if (1 == channelIndex)
    {
        buffer = GADI_PDA_CAP_SECOND_BUFFER;
    }
    else if (3 == channelIndex)
    {
        buffer = GADI_PDA_CAP_FOURTH_BUFFER;
    }

    yBuffer = malloc(MAX_CAP_FRAEM_SIZE);
    if (yBuffer == NULL) {
        printf("Not enough memory for Y buffer:%dKB !\n",MAX_CAP_FRAEM_SIZE);
        goto error_exit;
    }
    uvBuffer = malloc(MAX_CAP_FRAEM_SIZE);
    if (uvBuffer == NULL) {
        printf("Not enough memory for UV buffer:%dKB !\n",MAX_CAP_FRAEM_SIZE);
        goto error_exit;
    }

    if(gadi_pda_capture_buffer(buffer, &info) != GADI_OK)
    {
        GADI_ERROR("pda_capture_bmp: failed\n");

        goto error_exit;
    }
    printf("gadi_pda_capture_buffer %d*%d stride:%d\n", info.width, info.height, info.stride);
    //sprintf((char*)fileName,"./pda_%d_%d_yv12.yuv", info.width, info.height);
    sprintf((char*)fileName,"%s","./pda_yv12.yuv");
    yuv_stream = fopen((char *)fileName,"wb");
    if(yuv_stream == NULL)
    {
        printf("pda_capture_bmp: open file error\n");
        goto error_exit;
    }

    if (info.stride== info.width)
    {
        memcpy(yBuffer, info.yAddr, info.width * info.height);
    } else if (info.stride > info.width)
    {
        delete_padding_from_strided_y(yBuffer,
        info.yAddr, info.stride, info.width, info.height);
    } else
    {
        printf("stride size smaller than width!\n");
        goto error_exit;
    }

    //convert uv data from interleaved into planar format
    if (FRAME_FORMAT_YUV_SEMIPLANAR_420 == info.frameFormat) {
        uv_pitch = info.stride/ 2;
        uv_width = info.width / 2;
        uv_height = info.height / 2;
    } else { // FRAME_FORMAT_YUV_SEMIPLANAR_422
        uv_pitch = info.stride/ 2;
        uv_width = info.width / 2;
        uv_height = info.height;
    }

    delete_padding_and_deinterlace_from_strided_uv(uvBuffer,
        info.uvAddr, uv_pitch, uv_width, uv_height,1);

    fwrite(yBuffer, 1, info.width*info.height, yuv_stream);
    //fwrite(uvBuffer, 1, uv_width*uv_height*2, yuv_stream);

    printf(" Cap ture yuv picture success: %s\n",fileName);

	unsigned int page_num;
	y_pictue_t y_pictue;
	y_pictue.width = info.width;
	y_pictue.height = info.height;
	y_pictue.buffer = yBuffer;
	page_num = tide_identify_books(&y_pictue);
	printf("---------------------------------->book page number: %d\n", page_num);
	
error_exit:
    if(yBuffer != NULL)
    {
        free(yBuffer);
        yBuffer = NULL;
    }
    if(uvBuffer != NULL)
    {
        free(uvBuffer);
        uvBuffer = 0;
    }
    if(yuv_stream != NULL)
    {
        fclose(yuv_stream);
        yuv_stream = 0;

    }

    return ;
}

void pda_draw_mark(GADI_U32 desX, GADI_U32 desY, GADI_U32 lenth, GADI_U8 area)
{
    OSD_SQUARE_AttrT attr;

    if((desX == 0 && desY == 0) || lenth == 0){
            textPar.enable = 0;
            desX = 50;
            desY = 50;
            lenth = 2;
            attr.offsetX = desX * formatPar.width / 100;
            attr.offsetY = desY * formatPar.height / 100;
    } else {
        GADI_U32 tmpX,tmpY;

        tmpX = desX  & 0xfffc;
        tmpY = desY  & 0xfffc;
        if (textPar.enable == 1 && (textPar.startX & 0xfffc) == tmpX
            && (textPar.startY & 0xfffc) == tmpY){
            return;
        } else {
            textPar.enable = 1;
            textPar.startX = desX;
            textPar.startY = desY;
            attr.offsetX = textPar.startX * formatPar.width / 100;
            attr.offsetY = textPar.startY * formatPar.height / 100;
        }
    }

    attr.realHeight = formatPar.height;
    attr.realWidth = formatPar.width;
    attr.lenth = lenth * formatPar.height / 100;
    attr.area = area;
    attr.alpha = 255;//no alpha
    attr.enable = textPar.enable;
    attr.streamId = formatPar.streamId;

    osd_draw_square(&attr);
    /*
    retVal = osd_function_show_text(osdHandle, formatPar.width, formatPar.height,
                                   formatPar.streamId , area, &textPar);
    if(retVal != 0)
    {
        printf("show text error.\n");
    }*/
}

GADI_U32 pda_sqrt(GADI_U32 n)
{
    GADI_U32 retVal = 0, i;

    n += 1;
    for(i = 0; i < 32; i++){
        if(n&(0x1<<i))
            retVal = i;
    }
    return retVal;
}

void pda_mark_object_16bit(GADI_PDA_DataT *pdaData)
{
    GADI_U32 i,j;
    GADI_U32 allWith = 0;
    GADI_U32 allHeight = 0;
    GADI_U32 vildNums = 0;
    GADI_U32 checkEnd = 0;
    GADI_U32 startFlag = 0;
    GADI_U32 desX = 0;
    GADI_U32 desY = 0;
    GADI_U8  area = 0;
    GADI_U8  interval = 0;
    GADI_U16* bufArry[pdaData->u32MbHeight];

    const GADI_U32 localthreshlodValue = (threshlodValue << 8);

    for(i = 0; i < pdaData->u32MbHeight; i++)
    {
        bufArry[i] =  (GADI_U16 *)(pdaData->unData.stMdData.pAddr+ (i*pdaData->unData.stMdData.stride));
    }

    for(i = 0; i < pdaData->u32MbHeight; i++)
    {
        checkEnd = 0;

        for(j = 0; j < pdaData->u32MbWidth; j++)
        {
            if (*(bufArry[i] + j) > localthreshlodValue){
                allWith += j;
                allHeight += i;
                vildNums ++;
                checkEnd++;
            }
        }

        if(checkEnd == 0) {
            interval++;
            if(interval > 2 && startFlag == 1){
                desX  = allWith * 100;
                if(desX)
                    desX /= vildNums;
                if(desX)
                    desX /= pdaData->u32MbWidth;

                desY  = allHeight * 100;
                if(desY)
                    desY /= vildNums;
                if(desY)
                    desY /= pdaData->u32MbHeight;
                if(area < 3 ) {
                    pda_draw_mark(desX, desY, pda_sqrt(vildNums)*100/pdaData->u32MbHeight, area);
                    area++;
                }
                allWith = 0;
                allHeight = 0;
                vildNums = 0;
                startFlag = 0;

            }
        }else {
            startFlag = 1;
            interval = 0;
        }
    }
    for(; area < 3;area++)
        pda_draw_mark(0, 0, 0, area);
}

void pda_mark_object_8bit(GADI_PDA_DataT *pdaData)
{
    GADI_U32 i,j;
    GADI_U32 allWith = 0;
    GADI_U32 allHeight = 0;
    GADI_U32 vildNums = 0;
    GADI_U32 checkEnd = 0;
    GADI_U32 startFlag = 0;
    GADI_U32 desX = 0;
    GADI_U32 desY = 0;
    GADI_U8  area = 0;
    GADI_U8  interval = 0;
    GADI_U8* bufArry[pdaData->u32MbHeight];



    for(i = 0; i < pdaData->u32MbHeight; i++)
    {
        bufArry[i] =  (GADI_U8 *)(pdaData->unData.stMdData.pAddr+ (i*pdaData->unData.stMdData.stride));
    }

    for(i = 0; i < pdaData->u32MbHeight; i++)
    {
        checkEnd = 0;

        for(j = 0; j < pdaData->u32MbWidth; j++)
        {
            if (*(bufArry[i] + j) > threshlodValue){
                allWith += j;
                allHeight += i;
                vildNums ++;
                checkEnd++;
            }
        }

        if(checkEnd == 0) {
            interval++;
            if(interval > 2 && startFlag == 1){
                desX  = allWith * 100;
                if(desX)
                    desX /= vildNums;
                if(desX)
                    desX /= pdaData->u32MbWidth;

                desY  = allHeight * 100;
                if(desY)
                    desY /= vildNums;
                if(desY)
                    desY /= pdaData->u32MbHeight;
                if(area < 3 ) {
                    pda_draw_mark(desX, desY, pda_sqrt(vildNums)*100/pdaData->u32MbHeight, area);
                    area++;
                }
                allWith = 0;
                allHeight = 0;
                vildNums = 0;
                startFlag = 0;

            }
        }else {
            startFlag = 1;
            interval = 0;
        }
    }
    for(; area < 3;area++)
        pda_draw_mark(0, 0, 0, area);
}

extern GADI_SYS_HandleT voHandle;
extern GADI_SYS_HandleT osdHandle;
static void pda_thread(void *data )
{
    GADI_PDA_DataT pdaData;
    GADI_U16 i,j;
    GADI_U8 *buf8;
    GADI_U8 firstFlag = 1;
    GADI_VOUT_SettingParamsT voutParams;
    GADI_U8 osd_use = 0;
    GADI_U16 *buf16;

    /* estimate  for OSD is available. */
    voutParams.voutChannel = GADI_VOUT_B;
    GADI_ERR retVal = gadi_vout_get_params(voHandle, &voutParams);
    if(retVal == GADI_OK && voutParams.deviceType != GADI_VOUT_DEVICE_CVBS){
        osd_use = 1;
    }
    while(threadExist)
    {
        if(pdaStart)
        {
            //display luma change in 8*8 or 16*16 micro block
            if(gadi_pda_get_data(handle,&pdaData, GADI_TRUE) == GADI_OK)
            {
                if(firstFlag)GADI_INFO("gadi_pda_get_data ok\n");
                GADI_INFO("\nRead pic Number:%d,mbH,mbW:(%d:%d)\n",dataNum++,pdaData.u32MbHeight,pdaData.u32MbWidth);
                if(pdaData.unData.stMdData.mbSadBits == GADI_PDA_MD_MB_SAD_8BIT)
                {
                    if((osdHandle != NULL)
                        && (osd_use != 0)
                        && (useOSD != GADI_FALSE)) {
                        /* mark activate object, use osd draw.  */
                        pda_mark_object_8bit(&pdaData);
                    } else {
                        for(i = 0; i < pdaData.u32MbHeight; i++)
                        {
                            buf8 =  (pdaData.unData.stMdData.pAddr+ (i*pdaData.unData.stMdData.stride));
                            for(j = 0; j < pdaData.u32MbWidth; j++)
                            {
                                printf("%02x ",*(buf8+j));
                                if(j == 16)
                                {
                                    printf("\n");
                                }
                            }
                            printf("\n");
                        }
                        printf("\n");
                    }
                }
                else
                {
                    if((osdHandle != NULL)
                        && (osd_use != 0)
                        && (useOSD != GADI_FALSE)) {
                        /* mark activate object, use osd draw.  */
                        pda_mark_object_16bit(&pdaData);
                    } else {
                        /* mark activate object, use serial print.  */
                        for(i = 0; i < pdaData.u32MbHeight; i++)
                        {
                            buf16 =  (GADI_U16 *)(pdaData.unData.stMdData.pAddr+ (i*pdaData.unData.stMdData.stride));
                            for(j = 0; j < pdaData.u32MbWidth; j++)
                            {
                                printf("%02x ",(*(buf16+j))>>8);
                            }
                            printf("\n");
                        }
                        printf("\n");
                    }
                }
                gadi_pda_release_data(handle,&pdaData);
                if(firstFlag){
                    GADI_INFO("gadi_pda_release_data ok\n");
                    firstFlag = 0;
                }

            }
            else
            {
                gadi_sys_thread_sleep(30);
            }
        }
        else
        {

            gadi_sys_thread_sleep(300);
        }
    }
    GADI_INFO("pda_thread exit.\n");
}

GADI_ERR pda_register_testcase(void)
{
    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "pda",
        pda_p_handle_cmd,
        "pda command",
        "---------------------------------------------------------------------\n"
        "pda -I [0/1]\n"
        "   brief : init pda module.\n"
        "   param : buffer_type      -- It selects preview buffer type,use first or second, we usually use first buffer\n"
        "   example: pda init 0"
        "\n"
        "---------------------------------------------------------------------\n"
        "pda -O\n"
        "   brief : open pda handle.\n"
        "   example: pda open\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "---------------------------------------------------------------------\n"
        "pda -l \n"
        "   brief : print pda attribute.\n"
        "   example: pda print \n"
        "\n"
        "---------------------------------------------------------------------\n"
        "pda -S \n"
        "   brief : start to anylysis picture.\n"
        "   example: pda start \n"
        "\n"
        "---------------------------------------------------------------------\n"
        "pda -P \n"
        "   brief : stop to anylysis picture.\n"
        "   example: pda stop "
        "\n"
        "---------------------------------------------------------------------\n"
        "pda -E\n"
        "   brief : exit motion detection.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "pda -o\n"
        "   brief : use display pda data(disable:0 enable:1)"
        "\n"
        "---------------------------------------------------------------------\n"
        "Please input:"
        "   pda -h\n"
        "   look for more help!\n"
        "---------------------------------------------------------------------\n"
        /******************************************************************/
    );
    handle = 0;
    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

/*
    md
*/
static void Usage(void)
{
    printf("\nusage: pda [OPTION]...\n");
    printf("\t-h, --help            help.\n"
           "\t-I, --init            init pda buff(0/1).\n"
           "\t-O, --open             open pda.\n"
           "\t-C, --close            close pda.\n"
           "\t-E, --exit            exit pda.\n"
           "\t-S, --start            start pda.\n"
           "\t-P, --stop            stop pda.\n"
           "\t-l, --look            look pda attributes.\n"
           "\t-y, --cap yuv         cap picture yuv(0/1/3).\n"
           "\t-Y, --cap             cap picture bmp(0/1/3).\n"
           "\t-t, --set             set pda attribute.\n"
           "\t-b, --bufnum             pda attr, bufnums(0~7).\n"
           "\t-i, --intreval         pda attr, interval frame.\n"
           "\t-v, --threshold         mark threshold value(default 0x4ff).\n"
           "\t-o <number>, --osd    use display pda data(disable:0 enable:1), need open OSD.\n");

    printf("example:\n");
    printf("\tpda \n"
           " --- start pda buff 0. \n"
           "\tpda -I 0 -O -S \n"
           " --- cap bmp pictrue. \n"
           "\tpda -c 0 \n");

    printf("\n");
}

static GADI_ERR pda_p_handle_cmd(int argc, char* argv[])
{
    int option_index, ch;
    int retVal;
    int capChannel = 0;

    /*change parameters when giving input options.*/
    optind = 1;
    while (1)
    {
        option_index = 0;
        ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index);
        if (ch == -1)
            break;

        switch (ch)
        {
        case 'h':
            Usage();
            break;
        case 'I':
        {
            GADI_U32 buffIndex;
            buffIndex = atoi(optarg);
            if(buffIndex >= GADI_PDA_MD_NUM_BUFFER)
            {
                GADI_ERROR("bad parameter,buffer type error");
                goto fun_end;
            }

            GADI_INFO("pda: buffer type is (%d)\n",buffIndex);
            if(gadi_pda_init(buffIndex) != GADI_OK)
            {
                GADI_ERROR("pda initilize error.");
                goto fun_end;
            }
            GADI_INFO("gadi_pda_init ok\n");
            formatPar.streamId = 0;
            retVal = gdm_venc_get_stream_format(&formatPar);
            if(retVal != GADI_OK)
            {
                printf("get stream format failed.\n");
                goto fun_end;
            }
        }
        break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        case 'O':
        if(pdaOpen == 0){
            if(useSet)useSet = 0;
            handle = gadi_pda_open(&attr);
            if(handle != NULL)
            {
                GADI_INFO("pda: open success");
            }
            else
            {
                GADI_INFO("pda: bad paramter");
                goto fun_end;
            }
            if(threadExist == 0)
            {
                threadExist = 1;
                gadi_sys_thread_create(pda_thread,(void *)handle, GADI_SYS_THREAD_PRIO_DEFAULT,
                                    8092,"pda_thread", &pdaThreadNum);
            }
            pdaOpen = 1;
            GADI_INFO("gadi_pda_open ok\n");
        }
        break;

        case 'S':
        if(pdaStart == 0){
            if(handle != NULL)
            {
                if(gadi_pda_start_recv_pic(handle) != GADI_OK)
                {
                    GADI_ERROR("gadi_pda_start_recv_pic error\n");
                }
                else
                {
                    pdaStart = 1;
                    GADI_INFO("gadi_pda_start_recv_pic ok\n");
                }
            }
        }
        break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        case 'P':
        if(pdaStart){
            GADI_ERR ret;

            if(handle == NULL)
            {
                GADI_ERROR("the index has closed");
                goto fun_end;
            }
            pdaStart = 0;
            ret = gadi_pda_stop_recv_pic(handle);
            if(ret != GADI_OK)
            {
                pdaStart = 1;
                GADI_ERROR("stop md error");
            }
            else
            {
                GADI_INFO("gadi_pda_stop_recv_pic ok\n");
            }
            ret = GADI_OK;
        }
            break;

        case 'b':
            if(useSet)
                attr.mdAttr.mdBufNum = atoi(optarg);
                GADI_INFO("set pda buffer number(1,8): [%04d]\n", attr.mdAttr.mdFrameIntvel);
            break;

        case 'i':
            if(useSet)
                attr.mdAttr.mdFrameIntvel = atoi(optarg);
            GADI_INFO("set pda frame intervel:[%04d]\n", attr.mdAttr.mdFrameIntvel);
            break;

        case 't':
            useSet = 1;
            break;

        case 'v':
            threshlodValue = atoi(optarg);
            GADI_INFO("set pda threshlodValue[0x%04x]\n", threshlodValue);
            break;

        case 'l':
            pda_print_attr();
              break;

        case 'y':
            capChannel = atoi(optarg);
            if ((capChannel >= 0) && (capChannel < 4))
            {
                pda_cap_yuv_12(capChannel);
            }
            else
            {
                GADI_INFO("error channel index\n");
            }
            break;
        case 'Y':
            capChannel = atoi(optarg);
            if ((capChannel >= 0) && (capChannel < 4))
            {
                pda_capture_bmp(capChannel);
            }
            else
            {
                GADI_INFO("error channel index\n");
            }
            break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        case 'C':
        if(pdaOpen){
            if(threadExist != 0) {
                threadExist = 0;//exit pthread
                sleep(1);
            }
             retVal = gadi_pda_close(handle);
             if(retVal != GADI_OK){
                 GADI_ERROR("close pda fail\n");
                goto fun_end;
             }
             handle = NULL;
             pdaOpen = 0;
             GADI_INFO("gadi_pda_close ok\n");
        }

             break;

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        case 'E':
            if(handle != 0)
            {
                if(pdaStart){
                    pdaStart = 0;
                    gadi_pda_stop_recv_pic(handle);
                    GADI_INFO("gadi_pda_stop_recv_pic ok\n");
                    gadi_sys_thread_sleep(50);
                }
                if(pdaOpen) {
                    if(threadExist != 0) {
                        threadExist = 0;//exit pthread
                        gadi_sys_wait_end_thread(pdaThreadNum);
                        pdaThreadNum = 0;
                    }
                    gadi_sys_thread_sleep(50);
                    gadi_pda_close(handle);
                    pdaOpen = 0;
                    GADI_INFO("gadi_pda_close ok\n");
                }
                handle = 0;
            }

            retVal = gadi_pda_exit();
            if(retVal != GADI_OK){
                 GADI_ERROR("close pda fail\n");
                goto fun_end;
            }
            GADI_INFO("gadi_pda_exit ok\n");
            break;
        case 'o':
            if (atoi(optarg) == 0){
                useOSD = GADI_FALSE;
             } else if (atoi(optarg) == 1) {
                useOSD = GADI_TRUE;
            } else {
                GADI_ERROR("no params; use display pda data(disable:0 enable:1)\n");
            }
            GADI_INFO("display OSD is %s\n", (useOSD == GADI_TRUE)?"ON":"OFF");
            break;
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        default:
            GADI_INFO("Params error or pda no start. \n");
        break;
        }
    }
    if(useSet){
        useSet = 0;
        if(handle != NULL){
            gadi_pda_set_attr(handle, &attr);
            GADI_INFO("gadi_pda_set_attr ok\n");
        }
    }
    optind = 1;
    return 0;

fun_end:
    optind = 1;
    return -1;
}
