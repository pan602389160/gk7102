#ifndef __TIDESTOP_BOOK_H__
#define __TIDESTOP_BOOK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PR 0

typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t  *buffer;
}y_pictue_t;

/*
 * 加载黑白图片（bmp格式）
 * file_name：图片名称，包含路径
 * pictue：用来存储图片数据
 * 成功返回0，其他表示失败
 */
int32_t tide_ybmp_loader(int8_t *file_name, y_pictue_t *pictue);

/*
 * 保存黑白图片（bmp格式）
 * file_name：要保存的图片名称，包含路径
 * pictue：用来存储图片数据
 * 成功返回0，其他表示失败
 */
int32_t tide_ybmp_save(int8_t *file_name, y_pictue_t *pictue);

/*
 * 书本识别
 */
int32_t tide_identify_books(y_pictue_t *pictue);

#ifdef __cplusplus
}
#endif

#endif