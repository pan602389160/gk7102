/*************************************************************************
  copyright   : Copyright (C) 2014-2017, www.peergine.com, All rights reserved.
              :
  filename    : pgLiveMultiError.h
  discription : 
  modify      : create, chenbichao, 2017/05/10

*************************************************************************/
#ifndef _PG_LIB_LIVE_MULTI_ERROR_H
#define _PG_LIB_LIVE_MULTI_ERROR_H


#ifdef __cplusplus
extern "C" {
#endif


///
// Error code
typedef enum {
	PG_LIVE_MULTI_ERR_Normal    = 0,
	PG_LIVE_MULTI_ERR_System    = 1,
	PG_LIVE_MULTI_ERR_BadParam  = 2,
	PG_LIVE_MULTI_ERR_BadClass  = 3,
	PG_LIVE_MULTI_ERR_BadMethod = 4,
	PG_LIVE_MULTI_ERR_BadObject = 5,
	PG_LIVE_MULTI_ERR_BadStatus = 6,
	PG_LIVE_MULTI_ERR_BadFile   = 7,
	PG_LIVE_MULTI_ERR_BadUser   = 8,
	PG_LIVE_MULTI_ERR_BadPass   = 9,
	PG_LIVE_MULTI_ERR_NoLogin   = 10,
	PG_LIVE_MULTI_ERR_Network   = 11,
	PG_LIVE_MULTI_ERR_Timeout   = 12,
	PG_LIVE_MULTI_ERR_Reject    = 13,
	PG_LIVE_MULTI_ERR_Busy      = 14,
	PG_LIVE_MULTI_ERR_Opened    = 15,
	PG_LIVE_MULTI_ERR_Closed    = 16,
	PG_LIVE_MULTI_ERR_Exist     = 17,
	PG_LIVE_MULTI_ERR_NoExist   = 18,
	PG_LIVE_MULTI_ERR_NoSpace   = 19,
	PG_LIVE_MULTI_ERR_BadType   = 20,
	PG_LIVE_MULTI_ERR_CheckErr  = 21,
	PG_LIVE_MULTI_ERR_BadServer = 22,
	PG_LIVE_MULTI_ERR_BadDomain = 23,
	PG_LIVE_MULTI_ERR_NoData    = 24,
	PG_LIVE_MULTI_ERR_Unknown   = 255,
	PG_LIVE_MULTI_ERR_Extend    = 256,  // Begin of user extend error code.
} PG_LIVE_MULTI_ERR_E;


#ifdef __cplusplus
}
#endif

#endif
