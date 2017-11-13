/*
 * BarcodeFncRotate.c
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#include "BarcodeFncRotate.h"

static int gnImgRttMaxWidth = 0;
static int gnImgRttMaxHeight = 0;
static int gnImgRttMaxLineSize = 0;
static int gnImgRttMaxMemScale = 0;

static int * gpnImgRttBideL[NTHREADS] = {0};
static int * gpnImgRttBideR[NTHREADS] = {0};
static int ** gpnImgRttLinePts[NTHREADS] = {0};

unsigned char * gucImgRttZoom[NTHREADS] = {0};
unsigned char * gucImgRttImage[NTHREADS] = {0};
unsigned char * gucImgRttImage2[NTHREADS] = {0};

static int gnImgRttInitFlag = 0;


int BarcodeRotate_init(IHeap_Handle heap_id, int width, int height, BarcodeGlobalPointers ** globalPtrs)
{
	int nRet = 0, i = 0;

	if(gnImgRttInitFlag) {
		nRet = -11821000;
		goto nExit;
	}

	gnImgRttMaxWidth = width;
	gnImgRttMaxHeight = height;
	gnImgRttMaxLineSize = (int)sqrt(width*width*1.0 + height*height*1.0) + 1;
	gnImgRttMaxMemScale = (gnImgRttMaxLineSize * gnImgRttMaxLineSize) >> 1;

	if(gnImgRttMaxWidth <= 0 || gnImgRttMaxHeight <= 0 || gnImgRttMaxLineSize <= 0) {
		nRet = -11821002;
		goto nExit;
	}

	for(i = 0; i < NTHREADS; i++) {
		gpnImgRttBideL[i] = (int*) Memory_alloc(heap_id,
				ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	}

	for(i = 0; i < NTHREADS; i++) {
		gpnImgRttBideR[i] = (int*) Memory_alloc(heap_id,
				ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	}

	for(i = 0; i < NTHREADS; i++) {
		gpnImgRttLinePts[i] = (int **) Memory_alloc(heap_id, 4 * sizeof(int*), NULL, NULL);
	}

	for(i = 0; i < NTHREADS; i++) {
		gpnImgRttLinePts[i][0] = (int*) Memory_alloc(heap_id, ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
		gpnImgRttLinePts[i][1] = (int*) Memory_alloc(heap_id, ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
		gpnImgRttLinePts[i][2] = (int*) Memory_alloc(heap_id, ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
		gpnImgRttLinePts[i][3] = (int*) Memory_alloc(heap_id, ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	}

	for(i = 0; i < NTHREADS; i++) {
		gucImgRttZoom[i] = (unsigned char *) Memory_alloc(heap_id,
			ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char), MAX_CACHE_LINE, NULL);
		gucImgRttImage[i] = (unsigned char *) Memory_alloc(heap_id,
			ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char), MAX_CACHE_LINE, NULL);
		gucImgRttImage2[i] = (unsigned char *) Memory_alloc(heap_id,
			ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char), MAX_CACHE_LINE, NULL);
	}

	for(i = 0; i < NTHREADS; i++) {
		globalPtrs[i]->nImgRttMaxLineSize = gnImgRttMaxLineSize;
		globalPtrs[i]->nImgRttMaxMemScale = gnImgRttMaxMemScale;
		globalPtrs[i]->pnImgRttBideL = gpnImgRttBideL[i];
		globalPtrs[i]->pnImgRttBideR = gpnImgRttBideR[i];
		globalPtrs[i]->pnImgRttLinePts = gpnImgRttLinePts[i];
		globalPtrs[i]->ucImgRttZoom = gucImgRttZoom[i];
		globalPtrs[i]->ucImgRttImage = gucImgRttImage[i];
		globalPtrs[i]->ucImgRttImage2 = gucImgRttImage2[i];
	}

	gnImgRttInitFlag=1;
	nRet = 1;

nExit:
	return nRet;
}

void BarcodeRotate_release(IHeap_Handle heap_id)
{
	int i = 0;

	for(i = 0; i < NTHREADS; i++) {
		if(gpnImgRttBideL[i]) {
			Memory_free(heap_id, gpnImgRttBideL[i], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttBideL[i] = 0;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gpnImgRttBideR[i]) {
			Memory_free(heap_id, gpnImgRttBideR[i], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttBideR[i] = 0;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gpnImgRttLinePts[i][0]) {
			Memory_free(heap_id, gpnImgRttLinePts[i][0], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttLinePts[i][0] = 0;
		}

		if(gpnImgRttLinePts[i][1]) {
			Memory_free(heap_id, gpnImgRttLinePts[i][1], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttLinePts[i][1] = 0;
		}

		if(gpnImgRttLinePts[i][2]) {
			Memory_free(heap_id, gpnImgRttLinePts[i][2], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttLinePts[i][2] = 0;
		}

		if(gpnImgRttLinePts[i][3]) {
			Memory_free(heap_id, gpnImgRttLinePts[i][3], ROUNDUP(gnImgRttMaxLineSize, MAX_CACHE_LINE) * sizeof(int));
			gpnImgRttLinePts[i][3] = 0;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gpnImgRttLinePts[i]) {
			Memory_free(heap_id, gpnImgRttLinePts[i], 4 * sizeof(int*));
			gpnImgRttLinePts[i] = 0;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gucImgRttZoom[i]) {
			Memory_free(heap_id, gucImgRttZoom[i],
					ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char));
			gucImgRttZoom[i] = 0;
		}
		if(gucImgRttImage[i]) {
			Memory_free(heap_id, gucImgRttImage[i],
					ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char));
			gucImgRttImage[i] = 0;
		}
		if(gucImgRttImage2[i]) {
			Memory_free(heap_id, gucImgRttImage2[i],
					ROUNDUP(gnImgRttMaxMemScale, MAX_CACHE_LINE) * sizeof(unsigned char));
			gucImgRttImage2[i] = 0;
		}
	}

	gnImgRttInitFlag=0;
}



