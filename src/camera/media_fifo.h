/*!
*****************************************************************************
** FileName     : media_fifo.h
**
** Description  : media fifo
**
** Author       : hyb <heyongbin@gokemicro.com>
** Date         : 2015-7-29 create it
** Author       : bruce <zhaoquanfeng@gokemicro.com>
** Date         : 2015-8-9 modify it
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _MEDIA_FIFO_H_
#define _MEDIA_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VENC_MAX_STREAM_NUM 4


typedef void* MEDIABUF_HANDLE;

typedef struct _ZFIFO_NODE_ {
    void *base;
    int len;
} ZFIFO_NODE;

typedef struct _ZFIFO_NODE_P_ {
    int base;
    int len;
} ZFIFO_NODE_P;

typedef enum {
    GK_NET_STREAM_TYPE_STREAM0 = 0,
    GK_NET_STREAM_TYPE_STREAM1,
    GK_NET_STREAM_TYPE_STREAM2,
    GK_NET_STREAM_TYPE_STREAM3,
} GK_NET_STREAM_TYPE;

typedef enum {
    GK_NET_FRAME_TYPE_I = 0,
    GK_NET_FRAME_TYPE_P = 1,
    GK_NET_FRAME_TYPE_A = 2,
    GK_NET_FRAME_TYPE_JPEG = 3,
} GK_NET_FRAME_TYPE;

#define MAGIC_TEST  0XF3B0A4B8

typedef struct {
    unsigned int    magic;
    unsigned int device_type;
    unsigned int frame_size;//u
    unsigned int frame_no;
    unsigned int video_reso;   /* Video resolution. */
    unsigned char reserved;//U
    unsigned char frame_type;//U
    unsigned char frame_rate;
    unsigned char video_standard;//U
    unsigned int sec;//U
    unsigned int usec;//U
    unsigned int pts;

} GK_NET_FRAME_HEADER;



int mediabuf_init(int fifo_id, int size);
int mediabuf_uninit(int fifo_id);
MEDIABUF_HANDLE mediabuf_add_reader(GK_NET_STREAM_TYPE type);
void mediabuf_del_reader(MEDIABUF_HANDLE readerid);

//read data ,timeout 1 second,  <= 0 failed , > 0 success
// data: pointer of frame,
//      attation: if *data is not NULL, use buffer of caller
//                if *data is NULL, use buffer of mediabuf
// size: length of frame if success
//      attation: if *size is not equal 0, buffer length of caller
//                if *data is equal 0,     use buffer of mediabuf
// header: frame info if success
int mediabuf_read_frame(MEDIABUF_HANDLE readerid, void **data, int *size, GK_NET_FRAME_HEADER *header);

//read I frame ,no timeout, <=0 failed , >0 success
// data: pointer of frame,
//      attation: if *data is not NULL, use buffer of caller
//                if *data is NULL, use buffer of mediabuf
// size: length of frame if success
// header: frame info if success
int mediabuf_read_I_frame(MEDIABUF_HANDLE readerid, void **data, int *size, GK_NET_FRAME_HEADER *header);
int mediabuf_get_leave_frame(MEDIABUF_HANDLE handle);

MEDIABUF_HANDLE mediabuf_add_writer(GK_NET_STREAM_TYPE type);
void mediabuf_del_writer(MEDIABUF_HANDLE writerid);
int mediabuf_write_frame(MEDIABUF_HANDLE writerid, void *data, int size, GK_NET_FRAME_HEADER *header);
int mediabuf_set_newest_frame(MEDIABUF_HANDLE handle);
int mediabuf_read_frame_delay(MEDIABUF_HANDLE readerid, void **data, int *size, GK_NET_FRAME_HEADER *header,int delayFrame,int frameLimit);

#ifdef __cplusplus
};
#endif
#endif


