/*!
*****************************************************************************
** \file        ./adi/test/src/vout.h
**
** \brief       adi test vout module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _VOUT_H_
#define _VOUT_H_

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
#include "adi_vout.h"

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif


GADI_ERR gdm_vout_init(void);
GADI_ERR gdm_vout_exit(void);
GADI_ERR gdm_vout_open(void);
GADI_ERR gdm_vout_close(void);
GADI_ERR gdm_vout_parse_config_file(char *path);
GADI_ERR gdm_vout_setup(void);
GADI_ERR gdm_vout_setup_after(void);

#ifdef __cplusplus
    }
#endif


#endif /* _VOUT_H_ */

