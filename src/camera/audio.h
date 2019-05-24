/*!
*****************************************************************************
** \file        ./adi/test/src/audio.h
**
** \brief       adi test audio module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "stdio.h"
#include "adi_types.h"

#include "adi_sys.h"
#include "adi_audio.h"

//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
typedef enum {
    /** Playback stream */
    GADI_AUDIO_STREAM_PLAYBACK = 0,
    /** Capture stream */
    GADI_AUDIO_STREAM_CAPTURE,
    /**Loopback stream*/
    GADI_AUDIO_STREAM_LOOPBACK,
    GADI_AUDIO_STREAM_LAST
} GADI_AUDIO_StreamDirectEnumT;

//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif
GADI_ERR gdm_audio_init(GADI_VOID);
GADI_ERR gdm_audio_register_testcase(GADI_VOID);
GADI_ERR gdm_audio_ai_set_gain(GADI_S32 value);
GADI_ERR gdm_audio_ai_get_gain(GADI_S32 *value);
GADI_ERR gdm_audio_ao_set_volume(GADI_S32 value);
GADI_ERR gdm_audio_ao_get_volume(GADI_S32 *value);

#ifdef __cplusplus
    }
#endif


#endif /* _AUDIO_H_ */
