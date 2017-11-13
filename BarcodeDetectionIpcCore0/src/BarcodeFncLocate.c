/*
 * BarcodeFncRotate.c
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#include "BarcodeFncLocate.h"

#define AUTOCONTRAST_THRESHOLD_RATIO_L	(0.05)

float gfLctGradHistRatio = 0;
int gnLctGradHistThre = 0;

int * gpnLctGradHistSlice[NTHREADS];

int gnLctImgMaxWid = 0, gnLctImgMaxHei = 0, gnLctImgMaxSize = 0;
int gnLctOGMaxWid = 0, gnLctOGMaxHei = 0, gnLctOGMaxSize = 0;
int gnLctClusMaxSize = 0;

unsigned char * gucLctAtanLUT512 = 0;

unsigned char *	gucLctGradient = 0;
unsigned char *	gucLctOrientation = 0;

short *	gpsLctOGLabelMat = 0;
short * gpsLctHoughArray = 0;

int * gpnLctPrimaryMarks = 0;

FastLocateClus * gflLctPrimaryClus = 0;

unsigned char * gucLctBlobMask = 0;		// 用来标识图像中的非零点(边缘)
RyuPoint * gptLctBlobSeq = 0;				// 记录所有非零点的坐标
int * gpnLctBlobSeqFeature = 0;
LocateClusLine * gclLctBlobClusLine = 0;
LocateClusArea * gcaLctBlobClusArea = 0;

int gnIsBarcodeLctInit = 0;


int  BarcodeLocate_init(IHeap_Handle heap_id, int img_max_wid, int img_max_hei, BarcodeGlobalPointers ** globalPtrs)
{
	int nRet = 0;
	unsigned char * pLUT = 0;
	int i = 0, j = 0;
	int dx = 0, dy = 0;

	if(gnIsBarcodeLctInit) {
		nRet = -11202000;
		goto nExit;
	}

	if(img_max_wid <= 0 || img_max_hei <= 0) {
		nRet = -11202001;
		goto nExit;
	}

	gnLctImgMaxWid = img_max_wid;
	gnLctImgMaxHei = img_max_hei;
	gnLctImgMaxSize = gnLctImgMaxWid * gnLctImgMaxHei;

	gnLctOGMaxWid = gnLctImgMaxWid >> 2;
	gnLctOGMaxHei = gnLctImgMaxHei >> 2;
	gnLctOGMaxSize = gnLctOGMaxWid * gnLctOGMaxHei;

	gnLctClusMaxSize = gnLctImgMaxSize >> 8;

	gfLctGradHistRatio = AUTOCONTRAST_THRESHOLD_RATIO_L;


	gucLctAtanLUT512 = (unsigned char*) Memory_alloc(heap_id, sizeof(unsigned char) * 512 * 512,
			MAX_CACHE_LINE, NULL);
	if(!gucLctAtanLUT512) {
		nRet = -11202002;
		goto nExit;
	}
	memset(gucLctAtanLUT512, 0, 512 * 512 * sizeof(unsigned char));

	dy = -256;
	pLUT = gucLctAtanLUT512;
	for(i = 512; i > 0; i--) {
		dx = -256;
		for(j = 512; j > 0; j--) {
			*pLUT = ryuArctan180Shift(dy, dx);
			dx++;
			pLUT++;
		}
		dy++;
	}

	Cache_wb(gucLctAtanLUT512, 512 * 512 * sizeof(unsigned char), Cache_Type_ALL, TRUE);


	gucLctGradient = (unsigned char*) Memory_alloc(heap_id,
			sizeof(unsigned char) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gucLctGradient) {
		nRet = -11202003;
		goto nExit;
	}


	gucLctOrientation = (unsigned char*) Memory_alloc(heap_id,
			sizeof(unsigned char) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gucLctOrientation) {
		nRet = -11202004;
		goto nExit;
	}


	for(i = 0; i < NTHREADS; i++) {
		gpnLctGradHistSlice[i] = (int *) Memory_alloc(heap_id, 256 * sizeof(int), MAX_CACHE_LINE, NULL);
		if(!gpnLctGradHistSlice[i]) {
			nRet = -11202015;
			goto nExit;
		}
	}

	/*
	gpsLctOGLabelMat = (short *) Memory_alloc(heap_id,
			sizeof(short) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gpsLctOGLabelMat) {
		nRet = -11202005;
		goto nExit;
	}
	memset(gpsLctOGLabelMat, 0, sizeof(short) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE));
	*/

	gpnLctPrimaryMarks = (int *) Memory_alloc(heap_id,
			sizeof(int) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gpnLctPrimaryMarks) {
		nRet = -11202007;
		goto nExit;
	}


#ifndef USE_NEW_LOCATE_ALGORITHM
	gflLctPrimaryClus = (FastLocateClus *) Memory_alloc(heap_id,
			sizeof(FastLocateClus) * ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE), NULL, NULL);
	if(!gflLctPrimaryClus) {
		nRet = -11202008;
		goto nExit;
	}
	memset(gflLctPrimaryClus, 0, sizeof(FastLocateClus) * ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE));

	status = ClassicCCA_Init(heap_id, gnLctOGMaxWid, gnLctOGMaxHei, gnLctClusMaxSize, globalPtrs);
	if((gnLctImgMaxSize>>8) != status) {
		nRet = status;
		goto nExit;
	}
#else
	gucLctBlobMask = (unsigned char *) Memory_alloc(heap_id,
			ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(unsigned char), MAX_CACHE_LINE, NULL);
	if(!gucLctBlobMask) {
		nRet = -11202031;
	}


	gptLctBlobSeq = (RyuPoint *) Memory_alloc(heap_id,
			ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(RyuPoint), MAX_CACHE_LINE, NULL);
	if(!gptLctBlobSeq) {
		nRet = -11202032;
	}


	gclLctBlobClusLine = (LocateClusLine *) Memory_alloc(heap_id,
			ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(LocateClusLine), MAX_CACHE_LINE, NULL);
	if(!gclLctBlobClusLine) {
		nRet = -11202033;
	}


	gcaLctBlobClusArea = (LocateClusArea *) Memory_alloc(heap_id,
			ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(LocateClusArea), MAX_CACHE_LINE, NULL);
	if(!gcaLctBlobClusArea) {
		nRet = -11202034;
	}

#endif

	for(i = 0; i < NTHREADS; i++) {
		globalPtrs[i]->pnLctGradHistSlice = gpnLctGradHistSlice[i];
		globalPtrs[i]->nLctGradHistThre = gnLctGradHistThre;
		globalPtrs[i]->ucLctAtanLUT512 = gucLctAtanLUT512;
		globalPtrs[i]->ucLctGradient = gucLctGradient;
		globalPtrs[i]->ucLctOrientation = gucLctOrientation;
		globalPtrs[i]->pnLctPrimaryMarks = gpnLctPrimaryMarks;
		globalPtrs[i]->flLctPrimaryClus = (int *)gflLctPrimaryClus;
		globalPtrs[i]->ucLctBlobMask = gucLctBlobMask;
		globalPtrs[i]->ptLctBlobSeq = (int *)gptLctBlobSeq;
		globalPtrs[i]->clLctBlobClusLine = (int *)gclLctBlobClusLine;
		globalPtrs[i]->caLctBlobClusArea = (int *)gcaLctBlobClusArea;
		globalPtrs[i]->nLctClusMaxSize = gnLctClusMaxSize;
	}

	gnIsBarcodeLctInit = 1;
	nRet = 1;

nExit:
	return nRet;
}


void BarcodeLocate_release(IHeap_Handle heap_id)
{
	int i = 0;

	gnIsBarcodeLctInit = 0;

	if(gucLctAtanLUT512) {
		Memory_free(heap_id, gucLctAtanLUT512, sizeof(unsigned char) * 512 * 512);
		gucLctAtanLUT512 = 0;
	}

	if(gucLctGradient) {
		Memory_free(heap_id, gucLctGradient,
				sizeof(unsigned char) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE));
		gucLctGradient = 0;
	}

	if(gucLctOrientation) {
		Memory_free(heap_id, gucLctOrientation,
				sizeof(unsigned char) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE));
		gucLctOrientation = 0;
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gpnLctGradHistSlice[i]) {
			Memory_free(heap_id, gpnLctGradHistSlice[i], 256 * sizeof(int));
			gpnLctGradHistSlice[i] = 0;
		}
	}

	/*
	if(gpsLctOGLabelMat) {
		Memory_free(heap_id, gpsLctOGLabelMat, sizeof(short) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE));
		gpsLctOGLabelMat = 0;
	}*/

	if(gpnLctPrimaryMarks) {
		Memory_free(heap_id, gpnLctPrimaryMarks, sizeof(int) * ROUNDUP(gnLctOGMaxSize, MAX_CACHE_LINE));
		gpnLctPrimaryMarks = 0;
	}

#ifndef USE_NEW_LOCATE_ALGORITHM
	if(gflLctPrimaryClus) {
		Memory_free(heap_id, gflLctPrimaryClus, sizeof(FastLocateClus) * ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE));
		gflLctPrimaryClus = 0;
	}

	ClassicCCA_Release(heap_id);
#else
	if(gucLctBlobMask) {
		Memory_free(heap_id, gucLctBlobMask, ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(unsigned char));
		gucLctBlobMask = 0;
	}

	if(gptLctBlobSeq) {
		Memory_free(heap_id, gptLctBlobSeq, ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(RyuPoint));
		gptLctBlobSeq = 0;
	}

	if(gclLctBlobClusLine) {
		Memory_free(heap_id, gclLctBlobClusLine, ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(LocateClusLine));
		gclLctBlobClusLine = 0;
	}

	if(gcaLctBlobClusArea) {
		Memory_free(heap_id, gcaLctBlobClusArea, ROUNDUP(gnLctClusMaxSize, MAX_CACHE_LINE) * sizeof(LocateClusArea));
		gcaLctBlobClusArea = 0;
	}
#endif

	return;
}


