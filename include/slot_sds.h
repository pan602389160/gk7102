#ifndef __SLOT_SDS_H_
#define __SLOT_SDS_H_
#include <json-c/json.h>


/**
 * @brief free_sds 释放缓存的 sds(对话&内容).
 * @param sds [in] 缓存的 sds 结构体. 
 */
extern void free_sds(sds_info_t *sds);

/**
 * @brief get_sds 从包含 sds 的json， 提取 sds 内容.
 * @param sds_j [in] 包含 sds 的json.
 * @param sds [out] sds 信息结构体，用于缓存提取到的 sds 内容.
 */
extern void get_sds(json_object *sds_j, sds_info_t *sds);



#endif
