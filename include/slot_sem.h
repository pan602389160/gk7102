#ifndef __SLOT_SEM_H_
#define __SLOT_SEM_H_
#include <json-c/json.h>

/**
 * @brief free_sem 释放缓存的 sem(语义解析).
 * @param  sem [in] 缓存的 sem 结构体. 
 */
extern void free_sem(sem_info_t *sem);

/**
 * @brief get_sds 从包含 sem 的json， 提取 sem 内容.
 * @param sem_j [in] 包含 sem 的json.
 * @param sem [out] sem 信息结构体，用于缓存提取到的 sem 内容.
 */
extern void get_sem(json_object *sem_j, sem_info_t *sem);

#endif
