

#ifndef H264_FRAME_PROC_H
#define H264_FRAME_PROC_H


struct H264_FRAME_PROC_CACHE_S {
	unsigned char ucDataSPS[128];
	unsigned int uSizeSPS;
	unsigned char ucDataPPS[128];
	unsigned int uSizePPS;
	unsigned char* pucDataFrm;
	unsigned int uSizeFrm;
};

#define H264_FRAME_PROC_CACHE_INIT() {{0}, 0, {0}, 0, 0, 0}

void* H264FrameProcData(H264_FRAME_PROC_CACHE_S* lpCache, void* lpDataFrm, unsigned int* lpuDataSize);
void H264FrameProcClean(H264_FRAME_PROC_CACHE_S* lpCache);


#endif //H264_FRAME_PROC_H