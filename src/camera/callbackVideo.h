#ifndef _CALLBACK_VIDEO_H
#define _CALLBACK_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

void RegisterVideoCallback(void);
void VideoCallbackSetCurrentMode(unsigned int uMode);
void VideoCallbackModifyModeSize(unsigned int uMode, unsigned int uWidth, unsigned int uHeight);

#ifdef __cplusplus
}
#endif
#endif