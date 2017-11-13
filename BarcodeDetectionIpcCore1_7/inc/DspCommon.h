/*
 * algo_common.h
 *
 *  Created on: 2015-9-21
 *  Author: Liu Ryu
 */
#ifndef _DSP_COMMOM
#define _DSP_COMMOM

// 运行平台
#define _RUN_IN_6678

// debug调试开关
//#define _DEBUG_FLAG
#define  _DEBUG_OUTPUT_IMAGE_	(0)

// 打印信息开关
#define _PRINT_PROMPT

// 计时调试开关
//#define _PRINT_TIME

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _RUN_IN_6678
#include <c6x.h>
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Cache.h>
#include <ti/platform/platform.h>
#include "BarcodeDetectionIpc.h"

#define DDR_HEAP (HeapMem_Handle_to_xdc_runtime_IHeap(ddr_heap))

#define ROUNDUP(n,w) 		(((n) + (w) - 1) & ~((w) - 1))
#define MAX_CACHE_LINE 		(128)
#define NTHREADS 			(7)
#define NCORENUM 			(NTHREADS+1)

typedef struct tagBarcodeGlobalPointers {

	// BarcodeDetection
	int nBarcodeDetectInitImgWid;
	int nBarcodeDetectInitImgHei;
	unsigned char * ucBarcodeDetectResults;
//	barcode_detect_proc_info_t ** stBarcodeDetectProcInfo;
//	AlgorithmResult ** stBarcodeDetectResultNode;
	int ** stBarcodeDetectProcInfo;
	int ** stBarcodeDetectResultNode;

	// BarcodeFncLocate
	int nLctGradHistThre;
	unsigned char * ucLctAtanLUT512;
	unsigned char * ucLctGradient;
	unsigned char * ucLctOrientation;
	int * pnLctPrimaryMarks;
	int * pnLctGradHistSlice;
	int * flLctPrimaryClus;
	unsigned char * ucLctBlobMask;
	int * ptLctBlobSeq;
	int * clLctBlobClusLine;
	int * caLctBlobClusArea;
	int nLctClusMaxSize;

	// BarcodeFncSegment
	int nSgmHoughSeqMaxSize;
	int * pptSgmImgEffcRowRanges;
	int * pptSgmVerticalSeq;
	int * pptSgmParallelSeq;
	int * nSgmHoughAccum;
	int * pnSgmCastAccumVerti;
	int * pnSgmCastAccumParal;
	int * pnSgmMedianArray1;
	int * pnSgmMedianArray2;
	int * stSegmentBarcodeArea;
	int * stSegmentBarcodeAreaCollection;

	// BarcodeFncRotate
	int nImgRttMaxLineSize;
	int nImgRttMaxMemScale;
	int *  pnImgRttBideL;
	int *  pnImgRttBideR;
	int ** pnImgRttLinePts;
	unsigned char * ucImgRttZoom;
	unsigned char * ucImgRttImage;
	unsigned char * ucImgRttImage2;

	// BarcodeFncImgproc
	unsigned char * ucImgprocBuff1;
	unsigned char * ucImgprocOutput;

	// BarcodeFncDecode
	int * pnDcdColumnscanArr;
	int * pnDcdPartitionArr;
	int * pnDcdDecodeArr;
	int * pnDcdDecodeArrProc;
	int * ptDcdStartstop;

	int * pDDNOrig_arr;
	int * pDDNEffc_arr;
	float * pfDcdCoor_basic;
	float * pfDcdDecodeArr_basic;
	float * pfDcdCoor_strict;
	float * pfDcdDecodeArr_strict;
	int * pnDcdDecodeArr2;

	// WaybillSegment
	int nFloodFillMatMaxSize;
	int nFloodFillStatMaxCount;
	unsigned char * ucFloodFillImage;
	int * ptFloodFillSeeds;
	int * nFloodFillLabelMat;
	int * tFloodFillStatistics;
	int * nFloodFillContourMat;
	int * nFloodFillContourFlag;
	int * ptFloodFillContourPts;
	int * nFloodFillClassSeeds;
	int * tFloodFillClassClusters;
	int * tFloodFillCodeAreas;

#if _DEBUG_OUTPUT_IMAGE_
	unsigned char * ucOutputTestImage;
#endif
} BarcodeGlobalPointers;

#endif

#endif		// end of _DSP_COMMOM


