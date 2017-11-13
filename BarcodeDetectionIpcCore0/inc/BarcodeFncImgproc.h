#ifndef _BARCODE_IMPROVEMENT_H
#define _BARCODE_IMPROVEMENT_H

#include "DspCommon.h"
#include "RyuCore.h"

int BarcodeImgproc_init(IHeap_Handle heap_id, int max_wid, int max_hei, BarcodeGlobalPointers ** globalPtrs);

void BarcodeImgproc_release(IHeap_Handle heap_id);

int ryuImageContrastAnalyze(unsigned char * img, int width, int height, int widthstep, int * hist,
						float low_ratio, int * low_scale, float high_ratio, int * high_scale,
						int * min_scale, int * max_scale,
						int * avg_scale, int * mid_scale, int * grav_scale);

#endif	// of _BARCODE_IMPROVEMENT_H



