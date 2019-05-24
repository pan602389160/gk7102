/*
****************************************************************************
** \file      /applications/adidemo/demo/src/ircut.h
**
** \version   $Id: ircut.h du: cannot access ‘ircut.h’: No such file or directory 2016-09-19 10:14:43Z dengbiao $
**
** \brief     videc abstraction layer header file.
**
** \attention THIS SAMPLE CODE IS PROVIDED AS IS. GOFORTUNE SEMICONDUCTOR
**            ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**            OMMISSIONS.
**
** (C) Copyright 2015-2016 by GOKE MICROELECTRONICS CO.,LTD
**
****************************************************************************
*/
#include "adi_sys.h"
#include "adi_pwm.h"
#include "adi_gpio.h"

#ifndef __IRCUT_H__
#define __IRCUT_H__
//**************************************************************************
//**************************************************************************
//** Defines and Macros
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Enumerated types
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Data Structures
//**************************************************************************
//**************************************************************************
typedef enum {
    GADI_IRCUT_OverShresholdTrigger = 0,
    GADI_IRCUT_BelowShresholdTrigger = 1,
} GADI_IRCUT_TriggerModeEnumT;

//**************************************************************************
//**************************************************************************
//** Global Data
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** API Functions
//**************************************************************************
//**************************************************************************
#ifdef __cplusplus
extern "C" {
#endif
GADI_ERR ircut_register_testcase(void);
GADI_ERR ircut_register_daytonight(GADI_U32 channel, void (*daytonight)(GADI_U32 value));
GADI_ERR ircut_register_nighttoday(GADI_U32 channel, void (*nighttoday)(GADI_U32 value));
GADI_ERR ircut_configurate(GADI_U32 channel, GADI_U32 shreshold,
        GADI_U32 redundancy, GADI_IRCUT_TriggerModeEnumT trgrmode);
void ircut_switch_manual(GADI_U32 isday);
#ifdef __cplusplus
}
#endif
#endif /* IRCUT_H */

