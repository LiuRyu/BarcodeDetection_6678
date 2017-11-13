/*
 * BarcodeFncSegment.c
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#include "BarcodeFncSegment.h"

static int gnSgmImgMaxWidth = 0, gnSgmImgMaxHeight = 0;
const  int gnSgmNumangle = 19;
static int gnSgmMaxNumrho = 0;

static int gnSgmHoughSeqMaxSize = 0;

static RyuPoint * gpptSgmImgEffcRowRanges[NTHREADS] = {0};

static RyuPoint * gpptSgmVerticalSeq[NTHREADS] = {0};
static RyuPoint * gpptSgmParallelSeq[NTHREADS] = {0};

static int * gnSgmHoughAccum[NTHREADS] = {0};

static int * gpnSgmCastAccumVerti[NTHREADS] = {0};
static int * gpnSgmCastAccumParal[NTHREADS] = {0};

static int * gpnSgmMedianArray1[NTHREADS] = {0}, * gpnSgmMedianArray2[NTHREADS] = {0};

SegmentBarcodeArea * gstSegmentBarcodeArea[NTHREADS] = {0};

SegmentBarcodeArea * gstSegmentBarcodeAreaCollection = {0};

static int gnSgmInitFlag = 0;


int BarcodeSegment_init(IHeap_Handle heap_id, int img_max_wid, int img_max_hei, int seq_size,
		BarcodeGlobalPointers ** globalPtrs)
{
	int ret_val = 0;
	int i = 0;

	if(gnSgmInitFlag) {
		ret_val = -10219000;
		goto nExit;
	}

	if(0 >= img_max_wid || 0 >= img_max_hei) {
		ret_val = -10219001;
		goto nExit;
	}

	gnSgmImgMaxWidth = img_max_wid;
	gnSgmImgMaxHeight = img_max_hei;
	gnSgmHoughSeqMaxSize = seq_size;
	gnSgmMaxNumrho = (img_max_wid + img_max_hei) * 2 + 1;

	for(i = 0; i < NTHREADS; i++) {
		gpptSgmImgEffcRowRanges[i] = (RyuPoint*) Memory_alloc(heap_id,
				sizeof(RyuPoint) * ROUNDUP(gnSgmImgMaxHeight, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpptSgmImgEffcRowRanges[i]) {
			ret_val = -10219002;
			goto nExit;
		}

		gpptSgmVerticalSeq[i] = (RyuPoint*) Memory_alloc(heap_id,
				sizeof(RyuPoint) * ROUNDUP(gnSgmHoughSeqMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpptSgmVerticalSeq[i]) {
			ret_val = -10219003;
			goto nExit;
		}

		gpptSgmParallelSeq[i] = (RyuPoint*) Memory_alloc(heap_id,
				sizeof(RyuPoint) * ROUNDUP(gnSgmHoughSeqMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpptSgmParallelSeq[i]) {
			ret_val = -10219003;
			goto nExit;
		}

		gnSgmHoughAccum[i] = (int*) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnSgmMaxNumrho * gnSgmNumangle, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gnSgmHoughAccum[i]) {
			ret_val = -10219006;
			goto nExit;
		}

		gpnSgmCastAccumVerti[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnSgmCastAccumVerti[i]) {
			ret_val = -10219007;
			goto nExit;
		}

		gpnSgmCastAccumParal[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnSgmCastAccumParal[i]) {
			ret_val = -10219008;
			goto nExit;
		}

		gpnSgmMedianArray1[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnSgmMedianArray1[i]) {
			ret_val = -10219007;
			goto nExit;
		}

		gpnSgmMedianArray2[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnSgmMedianArray2[i]) {
			ret_val = -10219008;
			goto nExit;
		}

		gstSegmentBarcodeArea[i] = (SegmentBarcodeArea *) Memory_alloc(heap_id,
				sizeof(SegmentBarcodeArea) * MAX_SEGMENT_SLAVE_COUNT, NULL, NULL);
		if(!gstSegmentBarcodeArea[i]) {
			ret_val = -10219008;
			goto nExit;
		}
	}

	gstSegmentBarcodeAreaCollection = (SegmentBarcodeArea *) Memory_alloc(heap_id,
			sizeof(SegmentBarcodeArea) * MAX_SEGMENT_COUNT, NULL, NULL);
	if(!gstSegmentBarcodeAreaCollection) {
		ret_val = -10219008;
		goto nExit;
	}

	for(i = 0; i < NTHREADS; i++) {
		globalPtrs[i]->nSgmHoughSeqMaxSize 		= gnSgmHoughSeqMaxSize;
		globalPtrs[i]->pptSgmImgEffcRowRanges 	= (int *)gpptSgmImgEffcRowRanges[i];
		globalPtrs[i]->pptSgmVerticalSeq 		= (int *)gpptSgmVerticalSeq[i];
		globalPtrs[i]->pptSgmParallelSeq 		= (int *)gpptSgmParallelSeq[i];
		globalPtrs[i]->nSgmHoughAccum 			= gnSgmHoughAccum[i];
		globalPtrs[i]->pnSgmCastAccumVerti 		= gpnSgmCastAccumVerti[i];
		globalPtrs[i]->pnSgmCastAccumParal 		= gpnSgmCastAccumParal[i];
		globalPtrs[i]->pnSgmMedianArray1 		= gpnSgmMedianArray1[i];
		globalPtrs[i]->pnSgmMedianArray2 		= gpnSgmMedianArray2[i];
		globalPtrs[i]->stSegmentBarcodeArea 	= (int *)gstSegmentBarcodeArea[i];
		globalPtrs[i]->stSegmentBarcodeAreaCollection = (int *)gstSegmentBarcodeAreaCollection;
	}

	gnSgmInitFlag = 1;
	ret_val = 1;

nExit:
	return ret_val;
}

void BarcodeSegment_release(IHeap_Handle heap_id)
{
	int i = 0;

	for(i = 0; i < NTHREADS; i++) {
		if(gpptSgmImgEffcRowRanges[i]) {
			Memory_free(heap_id, gpptSgmImgEffcRowRanges[i], sizeof(RyuPoint) * ROUNDUP(gnSgmImgMaxHeight, MAX_CACHE_LINE));
			gpptSgmImgEffcRowRanges[i] = 0;
		}

		if(gpptSgmVerticalSeq[i]) {
			Memory_free(heap_id, gpptSgmVerticalSeq[i], sizeof(RyuPoint) * ROUNDUP(gnSgmHoughSeqMaxSize, MAX_CACHE_LINE));
			gpptSgmVerticalSeq[i] = 0;
		}

		if(gpptSgmParallelSeq[i]) {
			Memory_free(heap_id, gpptSgmParallelSeq[i], sizeof(RyuPoint) * ROUNDUP(gnSgmHoughSeqMaxSize, MAX_CACHE_LINE));
			gpptSgmParallelSeq[i] = 0;
		}

		if(gnSgmHoughAccum[i]) {
			Memory_free(heap_id, gnSgmHoughAccum[i], sizeof(int) * ROUNDUP(gnSgmMaxNumrho * gnSgmNumangle, MAX_CACHE_LINE));
			gnSgmHoughAccum[i] = 0;
		}

		if(gpnSgmCastAccumVerti[i]) {
			Memory_free(heap_id, gpnSgmCastAccumVerti[i], sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE));
			gpnSgmCastAccumVerti[i] = 0;
		}

		if(gpnSgmCastAccumParal[i]) {
			Memory_free(heap_id, gpnSgmCastAccumParal[i], sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE));
			gpnSgmCastAccumParal[i] = 0;
		}

		if(gpnSgmMedianArray1[i]) {
			Memory_free(heap_id, gpnSgmMedianArray1[i], sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE));
			gpnSgmMedianArray1[i] = 0;
		}

		if(gpnSgmMedianArray2[i]) {
			Memory_free(heap_id, gpnSgmMedianArray2[i], sizeof(int) * ROUNDUP(gnSgmMaxNumrho, MAX_CACHE_LINE));
			gpnSgmMedianArray2[i] = 0;
		}

		if(gstSegmentBarcodeArea[i]) {
			Memory_free(heap_id, gstSegmentBarcodeArea[i], sizeof(SegmentBarcodeArea) * MAX_SEGMENT_SLAVE_COUNT);
			gstSegmentBarcodeArea[i] = 0;
		}
	}

	if(gstSegmentBarcodeAreaCollection) {
		Memory_free(heap_id, gstSegmentBarcodeAreaCollection, sizeof(SegmentBarcodeArea) * MAX_SEGMENT_COUNT);
		gstSegmentBarcodeAreaCollection = 0;
	}

	gnSgmInitFlag = 0;

	return;
}


