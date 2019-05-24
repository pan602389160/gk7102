// callbackVideo.cpp 
//

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "h264_frame_proc.h"

			
enum H264_NAL_TYPE_E { 
	H264_NAL_TYPE_NAL = 0, 
	H264_NAL_TYPE_SLICE, 
	H264_NAL_TYPE_SLICE_DPA, 
	H264_NAL_TYPE_SLICE_DPB, 
	H264_NAL_TYPE_SLICE_DPC, 
	H264_NAL_TYPE_SLICE_IDR, 
	H264_NAL_TYPE_SEI, 
	H264_NAL_TYPE_SPS, 
	H264_NAL_TYPE_PPS, 
};


static unsigned int H264GetNALBlock(unsigned char* lpucData, unsigned int uSize,
	unsigned int* lpuNalType, unsigned char* lpucNalData, unsigned int* lpuNalSize, unsigned int* lpuNextPos)
{
	unsigned char* pucData = lpucData;

	// Parse first NAL head.
	unsigned int uResult = 0;
	unsigned int uInd0 = 0;
	while ((uInd0 + 4) <= uSize) {
		if (pucData[uInd0 + 0] == 0x0
			&& pucData[uInd0 + 1] == 0x0
			&& pucData[uInd0 + 2] == 0x1)
		{
			uInd0 += 3;
			uResult = 1;
			break;
		}
		if (pucData[uInd0 + 0] == 0x0
			&& pucData[uInd0 + 1] == 0x0
			&& pucData[uInd0 + 2] == 0x0
			&& pucData[uInd0 + 3] == 0x1)
		{
			uInd0 += 4;
			uResult = 1;
			break;
		}
		uInd0 += 1;
	}
	if (!uResult) {
		return 0;
	}

	// If SPS or PPS, parse the next NAL head.
	unsigned int uInd1 = uInd0;
	unsigned int uNalType = (pucData[uInd0] & 0x1F);
	if ((lpucNalData != 0 && lpuNalSize != 0)
		&& (uNalType == H264_NAL_TYPE_SPS || uNalType == H264_NAL_TYPE_PPS))
	{
		uResult = 0;
		while ((uInd1 + 4) <= uSize) {
			if (pucData[uInd1 + 0] == 0x0
				&& pucData[uInd1 + 1] == 0x0
				&& pucData[uInd1 + 2] == 0x1)
			{
				uResult = 1;
				break;
			}
			if (pucData[uInd1 + 0] == 0x0
				&& pucData[uInd1 + 1] == 0x0
				&& pucData[uInd1 + 2] == 0x0
				&& pucData[uInd1 + 3] == 0x1)
			{
				uResult = 1;
				break;
			}
			uInd1 += 1;
		}

		unsigned int uSizeTemp;
		if (uResult) {
			uSizeTemp = uInd1;
		}
		else {
			uSizeTemp = uSize;
		}

		if (uSizeTemp > (*lpuNalSize)) {
			printf("H264GetNALBlock: Buffer size not enough, NalType=%x, DataSize=%u\n", pucData[uInd0], uSizeTemp);
			return 0;
		}

		memcpy(lpucNalData, pucData, uSizeTemp);
		*lpuNalSize = uSizeTemp;
	}

	if (lpuNalType != 0) {
		*lpuNalType = uNalType;
	}

	*lpuNextPos = uResult ? uInd1 : 0;
	return 1;
}

void* H264FrameProcData(struct H264_FRAME_PROC_CACHE_S* lpCache, void* lpDataFrm, unsigned int* lpuDataSize)
{
	unsigned char* pucDataTemp = (unsigned char*)lpDataFrm;
	unsigned int uDataSizeTemp = *lpuDataSize;

	// Parse the first NAL head.
	unsigned int uSizeProc = 0;
	unsigned int uHalType = 0, uNextPos = 0;
	if (!H264GetNALBlock(&(pucDataTemp[uSizeProc]),
		(uDataSizeTemp - uSizeProc), &uHalType, 0, 0, &uNextPos))
	{
		return pucDataTemp;
	}

	if (uHalType == H264_NAL_TYPE_SPS) {
		
		// It is SPS head, continue to parse PPS head.
		uSizeProc += uNextPos;
		uHalType = 0, uNextPos = 0;
		if (!H264GetNALBlock(&(pucDataTemp[uSizeProc]),
			(uDataSizeTemp - uSizeProc), &uHalType, 0, 0, &uNextPos))
		{
			unsigned int uSizeTemp = sizeof(lpCache->ucDataSPS);
			H264GetNALBlock(&(pucDataTemp[uSizeProc]), (uDataSizeTemp - uSizeProc),
				0, lpCache->ucDataSPS, &uSizeTemp, 0);
			lpCache->uSizeSPS = uSizeTemp;
			return 0;
		}

		if (uHalType == H264_NAL_TYPE_PPS) {
			
			// It is PPS head, continue to parse IDR slice.
			uSizeProc += uNextPos;
			uHalType = 0, uNextPos = 0;
			if (!H264GetNALBlock(&(pucDataTemp[uSizeProc]),
				(uDataSizeTemp - uSizeProc), &uHalType, 0, 0, &uNextPos))
			{
				unsigned int uSizeTemp = sizeof(lpCache->ucDataPPS);
				H264GetNALBlock(&(pucDataTemp[uSizeProc]), (uDataSizeTemp - uSizeProc), 
					0, lpCache->ucDataPPS, &uSizeTemp, 0);
				lpCache->uSizePPS = uSizeTemp;
				return 0;
			}

			if (uHalType == H264_NAL_TYPE_SLICE_IDR) {
				// It is IDR slice.
				return pucDataTemp;
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	else if (uHalType == H264_NAL_TYPE_SLICE_IDR) {

		// The first NAL head is IDR slice, combine the SPS and PPS ahead.
		if (lpCache->uSizeSPS != 0 && lpCache->uSizePPS != 0) {
			
			unsigned int uSizeCache = lpCache->uSizeSPS + lpCache->uSizePPS + uDataSizeTemp;
			if ((uSizeCache % 1024) != 0) {
				uSizeCache = ((uSizeCache / 1024) + 1) * 1024;
			}

			if (uSizeCache > lpCache->uSizeFrm) {
				if (lpCache->pucDataFrm != 0) {
					free(lpCache->pucDataFrm);
					lpCache->pucDataFrm = 0;
					lpCache->uSizeFrm = 0;
				}
			}

			if (lpCache->pucDataFrm == 0) {
				lpCache->pucDataFrm = (unsigned char*)malloc(uSizeCache);
				if (lpCache->pucDataFrm == 0) {
					return 0;
				}
				lpCache->uSizeFrm = uSizeCache;
			}

			unsigned int uSizeTemp = 0;
			memcpy(lpCache->pucDataFrm, lpCache->ucDataSPS, lpCache->uSizeSPS);
			uSizeTemp += lpCache->uSizeSPS;
			
			memcpy(&(lpCache->pucDataFrm[uSizeTemp]), lpCache->ucDataPPS, lpCache->uSizePPS);
			uSizeTemp += lpCache->uSizePPS;
			
			memcpy(&(lpCache->pucDataFrm[uSizeTemp]), pucDataTemp, uDataSizeTemp);
			uSizeTemp += uDataSizeTemp;
			
			*lpuDataSize = uSizeTemp;
			return lpCache->pucDataFrm;
		}
		else {
			return 0;
		}
	}
	else {
		return pucDataTemp;
	}
}

void H264FrameProcClean(struct H264_FRAME_PROC_CACHE_S* lpCache)
{
	if (lpCache->pucDataFrm != 0) {
		free(lpCache->pucDataFrm);
		lpCache->pucDataFrm = 0;
	}
	lpCache->uSizeSPS = 0;
	lpCache->uSizePPS = 0;
	lpCache->uSizeFrm = 0;
}
