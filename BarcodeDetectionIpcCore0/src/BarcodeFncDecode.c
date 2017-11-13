
#include "BarcodeFncDecode.h"


int gnDcdMaxWidth = 0, gnDcdMaxHeight = 0;
int gnDcdMaxLineSize = 0;

int * gpnDcdColumnscanArr[NTHREADS] = {0};
int * gpnDcdPartitionArr[NTHREADS] = {0};
int * gpnDcdDecodeArr[NTHREADS] = {0};
int * gpnDcdDecodeArrProc[NTHREADS] = {0};

RyuPoint * gptDcdStartstop[NTHREADS] = {0};

DecodeDemarcateNode * gpDDNOrig_arr[NTHREADS] = {0};
DecodeDemarcateNode * gpDDNEffc_arr[NTHREADS] = {0};

float * gpfDcdCoor_basic[NTHREADS] = {0};
float * gpfDcdDecodeArr_basic[NTHREADS] = {0};

float * gpfDcdCoor_strict[NTHREADS] = {0};
float * gpfDcdDecodeArr_strict[NTHREADS] = {0};

int * gpnDcdDecodeArr2[NTHREADS] = {0};

int gnDcdInitFlag = 0;

int BarcodeDecode_init(IHeap_Handle heap_id, int maxImgWid, int maxImgHei, BarcodeGlobalPointers ** globalPtrs)
{
	int i = 0, nRet = 0;
	
	if(gnDcdInitFlag) {
		nRet = -11804000;
		goto nExit;
	}

	if(maxImgWid <= 0 || maxImgHei <= 0) {
		nRet = -11804001;
		goto nExit;
	}

	gnDcdMaxWidth = maxImgWid;
	gnDcdMaxHeight = maxImgHei;
	gnDcdMaxLineSize = (int)sqrt( maxImgWid*maxImgHei*1.0 + maxImgWid*maxImgHei*1.0) + 1;

	for(i = 0; i < NTHREADS; i++) {
		gpnDcdColumnscanArr[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnDcdColumnscanArr[i]) {
			nRet = -11804002;
			goto nExit;
		}

		gpnDcdPartitionArr[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnDcdPartitionArr[i]) {
			nRet = -11804003;
			goto nExit;
		}

		gpnDcdDecodeArr[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnDcdDecodeArr[i]) {
			nRet = -11804004;
			goto nExit;
		}

		gpnDcdDecodeArrProc[i] = (int *) Memory_alloc(heap_id,
				sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gpnDcdDecodeArrProc[i]) {
			nRet = -11804005;
			goto nExit;
		}

		gptDcdStartstop[i] = (RyuPoint *) Memory_alloc(heap_id,
				sizeof(RyuPoint) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gptDcdStartstop[i]) {
			nRet = -11804006;
			goto nExit;
		}

		gpDDNOrig_arr[i] = (DecodeDemarcateNode *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(DecodeDemarcateNode));
		gpDDNEffc_arr[i] = (DecodeDemarcateNode *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(DecodeDemarcateNode));
		if( !gpDDNOrig_arr[i] || !gpDDNEffc_arr[i]) {
			nRet = -11804008;
			goto nExit;
		}

		gpfDcdCoor_basic[i] = (float *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(float));
		gpfDcdDecodeArr_basic[i] = (float *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(float));
		if( !gpfDcdCoor_basic[i] || !gpfDcdDecodeArr_basic[i]) {
			nRet = -111804009;
			goto nExit;
		}

		gpfDcdCoor_strict[i] = (float *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(float));
		gpfDcdDecodeArr_strict[i] = (float *)malloc(ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(float));
		if( !gpfDcdCoor_strict[i] || !gpfDcdDecodeArr_strict[i]) {
			nRet = -11804010;
			goto nExit;
		}

		gpnDcdDecodeArr2[i] = (int *) malloc( ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE) * sizeof(int) );
		if( !gpnDcdDecodeArr2[i] ) {
			nRet = -11804011;
			goto nExit;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		globalPtrs[i]->pnDcdColumnscanArr = gpnDcdColumnscanArr[i];
		globalPtrs[i]->pnDcdPartitionArr = gpnDcdPartitionArr[i];
		globalPtrs[i]->pnDcdDecodeArr = gpnDcdDecodeArr[i];
		globalPtrs[i]->pnDcdDecodeArrProc = gpnDcdDecodeArrProc[i];
		globalPtrs[i]->ptDcdStartstop = (int *)gptDcdStartstop[i];

		globalPtrs[i]->pDDNOrig_arr = (int *)gpDDNOrig_arr[i];
		globalPtrs[i]->pDDNEffc_arr = (int *)gpDDNEffc_arr[i];
		globalPtrs[i]->pfDcdCoor_basic = gpfDcdCoor_basic[i];
		globalPtrs[i]->pfDcdDecodeArr_basic = gpfDcdDecodeArr_basic[i];
		globalPtrs[i]->pfDcdCoor_strict = gpfDcdCoor_strict[i];
		globalPtrs[i]->pfDcdDecodeArr_strict = gpfDcdDecodeArr_strict[i];
		globalPtrs[i]->pnDcdDecodeArr2 = gpnDcdDecodeArr2[i];
	}

	gnDcdInitFlag = 1;
	nRet = 1;

nExit:
	return nRet;
}

void BarcodeDecode_release(IHeap_Handle heap_id)
{
	int i = 0;

	for(i = 0; i < NTHREADS; i++) {
		if(gpnDcdColumnscanArr[i]) {
			Memory_free(heap_id, gpnDcdColumnscanArr[i], sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpnDcdColumnscanArr[i] = 0;
		}

		if(gpnDcdPartitionArr[i]) {
			Memory_free(heap_id, gpnDcdPartitionArr[i], sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpnDcdPartitionArr[i] = 0;
		}

		if(gpnDcdDecodeArr[i]) {
			Memory_free(heap_id, gpnDcdDecodeArr[i], sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpnDcdDecodeArr[i] = 0;
		}

		if(gpnDcdDecodeArrProc[i]) {
			Memory_free(heap_id, gpnDcdDecodeArrProc[i], sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpnDcdDecodeArrProc[i] = 0;
		}

		if(gptDcdStartstop[i]) {
			Memory_free(heap_id, gptDcdStartstop[i], sizeof(RyuPoint) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gptDcdStartstop[i] = 0;
		}

		if(gpDDNOrig_arr[i]) {
			Memory_free(heap_id, gpDDNOrig_arr[i], sizeof(DecodeDemarcateNode) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpDDNOrig_arr[i] = 0;
		}

		if(gpDDNEffc_arr[i]) {
			Memory_free(heap_id, gpDDNEffc_arr[i], sizeof(DecodeDemarcateNode) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpDDNEffc_arr[i] = 0;
		}

		if(gpfDcdCoor_basic[i]) {
			Memory_free(heap_id, gpfDcdCoor_basic[i], sizeof(float) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpfDcdCoor_basic[i] = 0;
		}

		if(gpfDcdDecodeArr_basic[i]) {
			Memory_free(heap_id, gpfDcdDecodeArr_basic[i], sizeof(float) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpfDcdDecodeArr_basic[i] = 0;
		}

		if(gpfDcdCoor_strict[i]) {
			Memory_free(heap_id, gpfDcdCoor_strict[i], sizeof(float) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpfDcdCoor_strict[i] = 0;
		}

		if(gpfDcdDecodeArr_strict[i]) {
			Memory_free(heap_id, gpfDcdDecodeArr_strict[i], sizeof(float) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpfDcdDecodeArr_strict[i] = 0;
		}

		if(gpnDcdDecodeArr2[i]) {
			Memory_free(heap_id, gpnDcdDecodeArr2[i], sizeof(int) * ROUNDUP(gnDcdMaxLineSize, MAX_CACHE_LINE));
			gpnDcdDecodeArr2[i] = 0;
		}
	}

	gnDcdInitFlag = 0;
}

