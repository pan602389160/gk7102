/*!
*****************************************************************************
** \file        ./adi/test/src/vi.h
**
** \brief       adi test vi module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _VI_H_
#define _VI_H_

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/queue.h>

#include "stdio.h"
#include "adi_types.h"

#include "adi_sys.h"
#include "adi_vi.h"
#include "adi_isp.h"

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif


GADI_ERR gdm_vi_init(void);
GADI_ERR gdm_vi_exit(void);
GADI_ERR gdm_vi_open(void);
GADI_ERR gdm_vi_close(void);
GADI_ERR gdm_vi_parse_config_file(char *path);
GADI_ERR gdm_vi_setup(void);
GADI_ERR gdm_vi_set_flip(GADI_VI_MirrorPatternEnumT rotate);
GADI_ERR gdm_vi_set_framerate(GADI_U32 framerate);
GADI_ERR gdm_vi_set_operation_mode(GADI_U8 mode);
GADI_ERR gdm_vi_set_slowshutter_framerate(GADI_U32* framerate);
GADI_ERR gdm_vi_set_ircut_control(GADI_VI_IRModeEnumT mode);
GADI_ERR gdm_vi_register_testcase(void);

#ifdef __cplusplus
    }
#endif


#endif /* _VI_H_ */

