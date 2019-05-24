/*!
*****************************************************************************
** \file        ./adi/test/src/isp.h
**
** \brief       adi test isp module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _ISP_H_
#define _ISP_H_

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
#ifdef HIAPI
#include "adi_isp_hi.h"
#else
#include "adi_isp.h"
#endif

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
    GADI_ERR isp_init(void);

    GADI_ERR isp_exit(void);

    GADI_ERR isp_open(void);

    GADI_ERR isp_close(void);

    GADI_ERR isp_restart(void);

    GADI_ERR isp_start(void);

    GADI_ERR isp_register_testcase(void);

    GADI_ERR isp_custom_code(void);

#ifdef __cplusplus
    }
#endif


#endif /* _ISP_H_ */
