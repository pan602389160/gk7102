#ifndef __SLOT_H_
#define __SLOT_H_
#include <json-c/json.h>

#include "slot_sds.h"
#include "slot_sem.h"

/**
 * @brief slot_free 释放解析语音分配的内存.
 * @param asr_info [in] 结果缓存结构体.
 */


extern void slot_free(asr_info_t *asr_info);
/**
 * @brief slot_resolve 提取语音识别的返回结果.
 * @param sem_json [in] 输入的原始json.
 * @param asr_info [out] 结果缓存结构体.
 *
 * @return 返回结果： 0 成功， -1：异常失败.
 *
 * @details 使用此函数会分配内存，需在后面使用slot_free（）释放分配的空间
 */

extern int slot_resolve(json_object *sem_json, asr_info_t *asr_info);


#endif

