/* =======================================================================================
*  Project          Keyword Search (KWS)
*  (c) Copyright    2014-2018
*  Company          Shanghai UVoice Technology CO., LTD
*                   All rights reserved
*  Secrecy Level    STRICTLY CONFIDENTIAL
* --------------------------------------------------------------------------------------*/
/**
 *  @internal
 *  @file kws.h
 *
 *  Prototypes of the Keyword Search (KWS)  API functions.
 *
 *  This header file contains all prototypes of the API functions of the Keyword Search (KWS) module.
 */
/*======================================================================================*/
/** @addtogroup DEF
*  @{*/
#ifndef _WAKEUP_API_
#define _WAKEUP_API_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------------------*/
/*   Error codes - success must ALWAYS equate to 0     */
/*-----------------------------------------------------*/
#define KWS_CODE_ERROR -1
#define KWS_CODE_NORMAL 0

/*--------------------------------------------------------------------------------------*/
/*      Define identification strings                                                   */
/*--------------------------------------------------------------------------------------*/
typedef uint16_t kws_uint16_t;
typedef float kws_float32_t;

/*-----------------------------------------------------------------------------*/
/*       Function prototypes                                                   */
/*-----------------------------------------------------------------------------*/
/**
*  This function get keyword name.
*
* @param
*      kwsInst    -IO : Pointer to the KWS instance
*      index      -I  : keyword index ( > 0)
*      name       -IO : Pointer pointer to keyword name (no copy)
* @return
*      Returns 0 if success or other error code
*/
int32_t kwsGetName(uint8_t index, const char** name);

/**
*  This function get version.
*
* @return
*      Returns version name
*/
const char* kwsGetVersion();

/**
 * Allocates the memory needed by the KWS.
 *
 * @param
 *      kwsInst -IO: Pointer to the created object or NULL with error
 *
 */
void kwsCreate(void** kwsInst);

/**
 *  Initializes the KWS instance.
 *
 * @param
 *      kwsInst    -IO : Pointer to the KWS instance
 * @return
 *      Returns 0 if success or other error code
 */
int32_t kwsInit(void* kwsInst);

/**
 *   This function enables the user to set certain parameters on-the-fly.
 *
 * @param
 *      kwsInst    -IO : Pointer to the KWS instance
 *      parm_id    -I  : Parameter id
*       data       -I  : Pointer to the parameter data
 * @return
 *      Returns 0 if success or other error code
 */

int32_t kwsReset(void* kwsInst);

/**
* Search keyword on one frame of data.
*
* @param
*      kwsInst        -IO : Pointer to the KWS instance
*      audio          -I  : In buffer containing one frame of recording signal (Number of samples must be 160)
*      samples        -I  : Number of samples in audio buffer, must be 160 now
* @return
*      Returns -1 means error, 0 means normal, > 0 means keyword index (level 1: 1, level 2: 2/3/4....)
*/
int32_t kwsProcess(void* kwsInst, const int16_t* audio, uint16_t samples);


/**
 * This function releases the memory allocated by UVoiceKWS_Create().
 *
 * @param
 *     uvoiceKWS -IO: Pointer to the KWS instance
 */

void kwsDesrtoy(void** kwsInst);

#ifdef __cplusplus
}
#endif

#endif
/**@}*/
