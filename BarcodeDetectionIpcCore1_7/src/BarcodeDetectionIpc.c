/*
 * BarcodeDetectionIpc.c
 *
 *  Created on: 2015-12-1
 *      Author: windlyu
 */


#include "BarcodeFncLocate.h"
#include "BarcodeFncSegment.h"
#include "BarcodeFncRotate.h"
#include "BarcodeFncImgproc.h"
#include "BarcodeFncDecode.h"
#include "WaybillSegmentation.h"
#include "BarcodeDetectionIpc.h"


/************************************************************************/
/* 全局变量																*/
/************************************************************************/
#define DECODE_AVGMIN_MODULEW_THRESHOLD	(1.8)
#define DECODE_IM_WIDTH_THRESHOLD		(400)

int nIsBarcodeDetectInit = 0;
int nBarcodeDetectInitImgWid = 0;
int nBarcodeDetectInitImgHei = 0;

unsigned char * ucBarcodeDetectResults = 0;
barcode_detect_proc_info_t ** stBarcodeDetectProcInfo;

int nScanInterval = 32;

/************************************************************************/
/* IPC交互全局变量															*/
/************************************************************************/
int nBarcodeDetectIsLearn = 0;
int nBarcodeDetectCodeCnt = 0;
int nBarcodeDetectRetVal = 0;
int nLctClusMaxGradVal = 0;

int nBarcodeDetectCandiCnt = 0, nBarcodeDetectCandiSeq = 0;

unsigned char * pBarcodeDetectInDataPtr = 0;
int nBarcodeDetectInDataWid = 0, nBarcodeDetectInDataHei = 0;

SegmentBarcodeArea * stSegmentBarcodeAreaCollection = 0;
int nSegmentBarcodeAreaCnt = 0;

AlgorithmResult ** stBarcodeDetectResultNode;

BarcodeGlobalPointers * stBarcodeGlobalPtrs;

#if _DEBUG_OUTPUT_IMAGE_
unsigned char * ucOutputTestImage = 0;
int nOutputTestImageW = 0, nOutputTestImageH = 0;
int nOutputTestImageFlag = 0;
#endif

/************************************************************************/
/* 设置参数																*/
/************************************************************************/
int nBarcodeDetectCodeSymbology = 0;
int nBarcodeDetectRglCodeCnt = 0;
int nBarcodeDetectCodeDgtNum = 0;
int nBarcodeDetectCodeValidity = 0;
int nBarcodeDetectCodeValidityExt = 0;
int nBarcodeDetectMultiPkgDetect = 0;

int nBarcodeDetectIpcLoopCnt = 0;
int nBarcodeDetectIpcMaxLoopCnt = 0;

/************************************************************************/
/* 函数声明																*/
/************************************************************************/
void BarcodeDetect_mapGlobalPtrs(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcMainProc0(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcSlaveProc0(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcMainProc1();
void BarcodeDetect_ipcSlaveProc1(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcMainProc2();
void BarcodeDetect_ipcSlaveProc2(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcMainProc3();
void BarcodeDetect_ipcSlaveProc3(barcode_detect_proc_info_t * proc_info);
void BarcodeDetect_ipcMainProc4();


void BarcodeDetect_run_ipcSlaveProc(barcode_detect_proc_info_t * proc_info)
{
	if(!proc_info) {
		return;
	}

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	switch(proc_info->proc_type) {
	case null_proc:
		return;

	case mapBarcodeGlobalPtrs:
		BarcodeDetect_mapGlobalPtrs(proc_info);
		break;

	case main_proc0:
		BarcodeDetect_ipcMainProc0(proc_info);
		break;

	case slave_proc0:
		BarcodeDetect_ipcSlaveProc0(proc_info);
		break;

	case main_proc1:
		BarcodeDetect_ipcMainProc1();
		break;

	case slave_proc1:
		BarcodeDetect_ipcSlaveProc1(proc_info);
		break;

	case main_proc2:
		BarcodeDetect_ipcMainProc2();
		break;

	case slave_proc2:
		BarcodeDetect_ipcSlaveProc2(proc_info);
		break;

	case main_proc3:
		BarcodeDetect_ipcMainProc3();
		break;

	case slave_proc3:
		BarcodeDetect_ipcSlaveProc3(proc_info);
		break;

	case main_proc4:
		BarcodeDetect_ipcMainProc4();
		break;

	default:	return;
	}
}


void BarcodeDetect_mapGlobalPtrs(barcode_detect_proc_info_t * proc_info)
{
	BarcodeGlobalPointers * globalPtr = 0;
	int status = 0;

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	globalPtr = (BarcodeGlobalPointers *) proc_info->nData1;

	if(!globalPtr) {
		proc_info->nFlag = -1;
		goto nExit;
	}

	nBarcodeDetectInitImgWid = globalPtr->nBarcodeDetectInitImgWid;
	if(nBarcodeDetectInitImgWid <= 0 || nBarcodeDetectInitImgWid > 6000) {
		proc_info->nFlag = -2;
		goto nExit;
	}
	nBarcodeDetectInitImgHei = globalPtr->nBarcodeDetectInitImgHei;
	if(nBarcodeDetectInitImgHei <= 0 || nBarcodeDetectInitImgHei > 6000) {
		proc_info->nFlag = -3;
		goto nExit;
	}
	ucBarcodeDetectResults = globalPtr->ucBarcodeDetectResults;
	if(!ucBarcodeDetectResults) {
		proc_info->nFlag = -4;
		goto nExit;
	}
	stBarcodeDetectProcInfo = (barcode_detect_proc_info_t **)globalPtr->stBarcodeDetectProcInfo;
	if(!stBarcodeDetectProcInfo) {
		proc_info->nFlag = -5;
		goto nExit;
	}
	stBarcodeDetectResultNode = (AlgorithmResult **)globalPtr->stBarcodeDetectResultNode;
	if(!stBarcodeDetectResultNode) {
		proc_info->nFlag = -6;
		goto nExit;
	}
	stSegmentBarcodeAreaCollection = (SegmentBarcodeArea *)globalPtr->stSegmentBarcodeAreaCollection;
	if(!stSegmentBarcodeAreaCollection) {
		proc_info->nFlag = -12;
		goto nExit;
	}

	status = mapBarcodeLocateGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -7;
		goto nExit;
	}
	status = mapBarcodeSegmentGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -9;
		goto nExit;
	}
	status = mapBarcodeRotateGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -8;
		goto nExit;
	}
	status = mapBarcodeImgprocGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -11;
		goto nExit;
	}
	status = mapBarcodeDecodeGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -10;
		goto nExit;
	}
	status = mapWaybillSegmentGlobalPtrs(globalPtr);
	if(1 != status) {
		proc_info->nFlag = -11;
		goto nExit;
	}

#if _DEBUG_OUTPUT_IMAGE_
	ucOutputTestImage = globalPtr->ucOutputTestImage;
#endif

	Cache_inv(globalPtr, sizeof(BarcodeGlobalPointers), Cache_Type_ALL, FALSE);
	proc_info->nFlag = 1;

nExit:
	Cache_wb(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	return;
}


void BarcodeDetect_ipcMainProc0(barcode_detect_proc_info_t * proc_info)
{
	int ret_val = 0, status = 0;
	int i = 0;

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	nBarcodeDetectRetVal = 0;
	nBarcodeDetectCodeCnt = 0;
	nLctClusMaxGradVal = 0;
	nBarcodeDetectCandiCnt = nBarcodeDetectCandiSeq = 0;
	nSegmentBarcodeAreaCnt = 0;

	memset(ucBarcodeDetectResults, 0, sizeof(AlgorithmResult) * MAX_BARCODE_COUNT + sizeof(int));
	Cache_wb(ucBarcodeDetectResults, sizeof(AlgorithmResult) * MAX_BARCODE_COUNT + sizeof(int), Cache_Type_ALL, FALSE);

#if _DEBUG_OUTPUT_IMAGE_
		nOutputTestImageFlag = 0;
#endif

	pBarcodeDetectInDataPtr = proc_info->uData1;
	nBarcodeDetectIsLearn = proc_info->nVari1;
	nBarcodeDetectInDataWid = proc_info->nVari2;
	nBarcodeDetectInDataHei = proc_info->nVari3;

	nBarcodeDetectRglCodeCnt = proc_info->nVari4;
	nBarcodeDetectCodeSymbology = proc_info->nVari5;
	nBarcodeDetectCodeDgtNum = proc_info->nVari6;
	nBarcodeDetectCodeValidity = proc_info->nVari7;
	nBarcodeDetectCodeValidityExt = proc_info->nVari8;
	nBarcodeDetectMultiPkgDetect = proc_info->nVari9;

	if(nBarcodeDetectIsLearn)
		nBarcodeDetectIpcMaxLoopCnt = MAX_BARCODE_COUNT / 4 + 1;
	else
		nBarcodeDetectIpcMaxLoopCnt = nBarcodeDetectRglCodeCnt / 4 + 1;

	if(0 == pBarcodeDetectInDataPtr) {
		ret_val = -10112019;
		nBarcodeDetectRetVal = ret_val;
		//stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		printf("\n--Ryu--Invalid inputs of Algorithm Learning/Running operation, %d\n", ret_val);
		goto nExit;
	}

	if(nBarcodeDetectInDataWid > nBarcodeDetectInitImgWid || nBarcodeDetectInDataHei > nBarcodeDetectInitImgHei) {
		ret_val = -10112020;
		nBarcodeDetectRetVal = ret_val;
		//stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		printf("\n--Ryu--Invalid inputs of Algorithm Learning/Running operation, %d\n", ret_val);
		goto nExit;
	}

	status = BarcodeLocation_ipcMainProc0(pBarcodeDetectInDataPtr, nBarcodeDetectInDataWid, nBarcodeDetectInDataHei,
				&nScanInterval, stBarcodeDetectProcInfo);
	if(1 != status) {
		ret_val = status;
		nBarcodeDetectRetVal = ret_val;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	for(i = 1; i < NCORENUM; i++) {
		stBarcodeDetectProcInfo[i]->nFlag = 1;
	}

	ret_val = 1;

nExit:

	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return;
}


void BarcodeDetect_ipcSlaveProc0(barcode_detect_proc_info_t * proc_info)
{
	int status = 0;

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	status = BarcodeLocation_ipcSlaveProc0(proc_info);

	proc_info->nFlag = status;
	Cache_wb(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);

	return;
}

void BarcodeDetect_ipcMainProc1()
{
	int ret_val = 0, status = 0;
	int i = 0;

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		stBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	if(0 > nBarcodeDetectRetVal) {
		ret_val = nBarcodeDetectRetVal;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	status = BarcodeLocation_ipcMainProc1(stBarcodeDetectProcInfo);
	if(1 != status) {
		ret_val = status;
		nBarcodeDetectRetVal = ret_val;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	for(i = 1; i < NCORENUM; i++) {
		stBarcodeDetectProcInfo[i]->nFlag = 1;
	}

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return;
}

void BarcodeDetect_ipcSlaveProc1(barcode_detect_proc_info_t * proc_info)
{
	int status = 0;

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	status = BarcodeLocation_ipcSlaveProc1(proc_info);

	proc_info->nFlag = status;
	Cache_wb(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);

	return;
}

void BarcodeDetect_ipcMainProc2()
{
	int ret_val = 0;

	int i = 0, j = 0, candi_cnt = 0, effec_cnt = 0, index = 0;

#ifndef USE_NEW_LOCATE_ALGORITHM
	FastLocateClus * pLocateClus = 0;
#else
	LocateClusArea * pLocateClus = 0;
#endif

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		stBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	if(0 > nBarcodeDetectRetVal) {
		ret_val = nBarcodeDetectRetVal;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	// Location: BlobCluster2Line & LineCluster2Area
	candi_cnt = BarcodeLocation_ipcMainProc2(stBarcodeDetectProcInfo);
	if(0 > candi_cnt) {
		ret_val = nBarcodeDetectRetVal = candi_cnt;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	} else if(0 == candi_cnt) {
		ret_val = nBarcodeDetectRetVal = -1;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

#ifndef USE_NEW_LOCATE_ALGORITHM
	pLocateClus = getLocateBarCodePrimary();
#else
	pLocateClus = getLocateFeatureAreas();
#endif

	nBarcodeDetectIpcLoopCnt = (candi_cnt - 1) / NTHREADS + 1;
	nBarcodeDetectIpcLoopCnt = RYUMIN(nBarcodeDetectIpcLoopCnt, nBarcodeDetectIpcMaxLoopCnt);
	index = 1;
	for(j = 0; j < nBarcodeDetectIpcLoopCnt; j++) {
		for(i = 1; i < NCORENUM; i++) {
			if(0 >= candi_cnt) {
				break;
			}
			if(2 > pLocateClus[index].linecnt) {
				candi_cnt = 0;
				break;
			}
			if(j == 0) {
				stBarcodeDetectProcInfo[i]->nData1 = (int *) &pLocateClus[index];
				stBarcodeDetectProcInfo[i]->uData1 = pBarcodeDetectInDataPtr;
				stBarcodeDetectProcInfo[i]->nVari1 = nBarcodeDetectInDataWid;
				stBarcodeDetectProcInfo[i]->nVari2 = nBarcodeDetectInDataHei;
				stBarcodeDetectProcInfo[i]->nVari3 = 0;
//				stBarcodeDetectProcInfo[i]->nVari3 = nBarcodeDetectIsLearn;
//				stBarcodeDetectProcInfo[i]->nVari4 = nBarcodeDetectCodeSymbology;
				stBarcodeDetectProcInfo[i]->nVari5 = 0;
			}

			stBarcodeDetectProcInfo[i]->nVari5++;
			effec_cnt++;
			index++;
			candi_cnt--;
		}
		if(0 >= candi_cnt) {
			break;
		}
	}

	if(0 == effec_cnt) {
		ret_val = nBarcodeDetectRetVal = -1;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	stBarcodeDetectProcInfo[1]->nFlag = effec_cnt;
	ret_val = effec_cnt;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return;
}

// 分核做分割处理
void BarcodeDetect_ipcSlaveProc2(barcode_detect_proc_info_t * proc_info)
{
	int i = 0, status = 0, nSegmCnt = 0, nLoopCnt = 0;

#ifndef USE_NEW_LOCATE_ALGORITHM
	FastLocateClus * pLocateClus = 0;
#else
	LocateClusArea * pLocateClus = 0;
#endif


	unsigned char * img_data = 0;
	int img_wid = 0, img_hei = 0;
	SegmentBarcodeArea * pSegmentBarcode = 0;

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	img_data = proc_info->uData1;
	img_wid = proc_info->nVari1;
	img_hei = proc_info->nVari2;

	Cache_inv(img_data, img_wid * img_hei * sizeof(unsigned char), Cache_Type_ALL, FALSE);

	nLoopCnt = proc_info->nVari5;

	for(i = 0; i < nLoopCnt; i++) {
		pLocateClus  = (LocateClusArea *)proc_info->nData1 + i * 7;
		Cache_inv(pLocateClus, sizeof(LocateClusArea), Cache_Type_ALL, FALSE);

		// 条码分割
		status = SegmentBarcode(img_data, img_wid, img_hei,  pLocateClus->corner, (RyuPoint*)&pLocateClus->min_intcpt,
			(RyuPoint*)&pLocateClus->min_ontcpt, &pLocateClus->angle, pLocateClus->grad, nSegmCnt);
		if(0 < status)
			nSegmCnt += status;
		if(nSegmCnt >= MAX_SEGMENT_SLAVE_COUNT) {
			nSegmCnt = MAX_SEGMENT_SLAVE_COUNT;
			break;
		}
	}

	if(0 < nSegmCnt) {
		proc_info->nVari3 = nSegmCnt;
		pSegmentBarcode = GetSegmentBarcodeAreaPtr();
		proc_info->nData3 = (int *)pSegmentBarcode;
		proc_info->nFlag = 1;
	} else {
		proc_info->nFlag = 0;
	}

	Cache_wb(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	Cache_wb(pLocateClus, sizeof(LocateClusArea), Cache_Type_ALL, TRUE);
}

// 整合分割条码，排序，并装入容器准备分发至从核处理
void BarcodeDetect_ipcMainProc3()
{
	int ret_val = 0;

	int i = 0, j = 0, candi_cnt = 0, effec_cnt = 0;
	SegmentBarcodeArea tmpSegment;
	int nTmp1 = 0, nTmp2 = 0;

	SegmentBarcodeArea * pSegmentBarcodeCollect = stSegmentBarcodeAreaCollection;
	SegmentBarcodeArea * pSegmentBarcode = 0;

	LocateClusArea * pLocateClus = 0;

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		stBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	if(0 > nBarcodeDetectRetVal) {
		ret_val = nBarcodeDetectRetVal;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	// 整合
	for(i = 1; i < NCORENUM; i++) {
		if(0 < stBarcodeDetectProcInfo[i]->nVari3 && 1 == stBarcodeDetectProcInfo[i]->nFlag) {
			pSegmentBarcode = (SegmentBarcodeArea *)stBarcodeDetectProcInfo[i]->nData3;
			Cache_inv(pSegmentBarcode, stBarcodeDetectProcInfo[i]->nVari3 * sizeof(SegmentBarcodeArea), Cache_Type_ALL, FALSE);
			memcpy(&pSegmentBarcodeCollect[candi_cnt], pSegmentBarcode,
					stBarcodeDetectProcInfo[i]->nVari3 * sizeof(SegmentBarcodeArea));
			candi_cnt += stBarcodeDetectProcInfo[i]->nVari3;

			// 版本2.1.2修改(20160913)
			// 统计获取候选区域最大grad值，若其小于阈值，且无识别结果，则判定为无条码
			// 此处主要解决无条码时误判定为NOREAD的情况
			pLocateClus = (LocateClusArea *)stBarcodeDetectProcInfo[i]->nData1;
			Cache_inv(pLocateClus, sizeof(LocateClusArea), Cache_Type_ALL, FALSE);
			nLctClusMaxGradVal = (pLocateClus->grad > nLctClusMaxGradVal)
					? pLocateClus->grad : nLctClusMaxGradVal;
		}
	}

	if(0 > candi_cnt) {
		ret_val = nBarcodeDetectRetVal = candi_cnt;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	} else if(0 == candi_cnt) {
		ret_val = nBarcodeDetectRetVal = -1;
		stBarcodeDetectProcInfo[1]->nFlag = ret_val;
		goto nExit;
	}

	// 按照面积排序
	for(i = 0; i < candi_cnt; i++) {
		for(j = i+1; j < candi_cnt; j++) {
			nTmp1 = (pSegmentBarcodeCollect[i].max_ontcpt-pSegmentBarcodeCollect[i].min_ontcpt)
				* (pSegmentBarcodeCollect[i].max_intcpt-pSegmentBarcodeCollect[i].min_intcpt);
			nTmp2 = (pSegmentBarcodeCollect[j].max_ontcpt-pSegmentBarcodeCollect[j].min_ontcpt)
				* (pSegmentBarcodeCollect[j].max_intcpt-pSegmentBarcodeCollect[j].min_intcpt);
			if(nTmp1 < nTmp2) {
				tmpSegment = pSegmentBarcodeCollect[i];
				pSegmentBarcodeCollect[i] = pSegmentBarcodeCollect[j];
				pSegmentBarcodeCollect[j] = tmpSegment;
			}
		}
	}

	Cache_wb(pSegmentBarcodeCollect, candi_cnt * sizeof(SegmentBarcodeArea), Cache_Type_ALL, TRUE);

	nBarcodeDetectIpcLoopCnt = (candi_cnt - 1) / NTHREADS + 1;
	nBarcodeDetectIpcLoopCnt = RYUMIN(nBarcodeDetectIpcLoopCnt, nBarcodeDetectIpcMaxLoopCnt);

	for(j = 0; j < nBarcodeDetectIpcLoopCnt; j++) {
		for(i = 1; i < NCORENUM; i++) {
			if(0 >= candi_cnt) {
				break;
			}
			if(j == 0) {
				stBarcodeDetectProcInfo[i]->nData1 = (int *) &pSegmentBarcodeCollect[effec_cnt];
				stBarcodeDetectProcInfo[i]->nData2 = (int *) stBarcodeDetectResultNode[i-1];
				stBarcodeDetectProcInfo[i]->uData1 = pBarcodeDetectInDataPtr;
				stBarcodeDetectProcInfo[i]->nVari1 = nBarcodeDetectInDataWid;
				stBarcodeDetectProcInfo[i]->nVari2 = nBarcodeDetectInDataHei;
				stBarcodeDetectProcInfo[i]->nVari3 = nBarcodeDetectIsLearn;
				stBarcodeDetectProcInfo[i]->nVari4 = nBarcodeDetectCodeSymbology;
				stBarcodeDetectProcInfo[i]->nVari5 = 0;
				stBarcodeDetectProcInfo[i]->nVari6 = nBarcodeDetectCodeDgtNum;
				stBarcodeDetectProcInfo[i]->nVari7 = nBarcodeDetectCodeValidity;
				stBarcodeDetectProcInfo[i]->nVari8 = nBarcodeDetectCodeValidityExt;
			}
			stBarcodeDetectProcInfo[i]->nVari5++;
			effec_cnt++;
			candi_cnt--;
		}
		if(0 >= candi_cnt) {
			break;
		}
	}

	nSegmentBarcodeAreaCnt = effec_cnt;
	stBarcodeDetectProcInfo[1]->nFlag = effec_cnt;
	ret_val = effec_cnt;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return;
}

void BarcodeDetect_ipcSlaveProc3(barcode_detect_proc_info_t * proc_info)
{
	int status = 0, i = 0, j = 0, k = 0;
	int nCodeCnt = 0, nLoopCnt = 0;

	SegmentBarcodeArea * pSegmentBarcode = 0;
	AlgorithmResult * result_node = 0;

	unsigned char * img_data = 0;
	int img_wid = 0, img_hei = 0;

	// 旋转
	int rtt_width = 0, rtt_height = 0;
	unsigned char * ucBarcodeImage = 0, * ucDecodeImage = 0;

	// 读码
	int sliceH = 0, nDecodeFlag = 0, sliceHs[32] = {0}, sliceCnt = 0;
	int codeDigit = 0, codeType = 0, codeValid = 0, codeDirect = 0, codeModule = 0;
	char code_result[CODE_RESULT_ARR_LENGTH];
	int codeOffsetL = 0, codeOffsetR = 0;

	float minModuleW = 0.0, accModuleW = 0.0;

	RyuPoint ptCenter;

	int isLearn = 0, setType = 0, setDigit = 0, setValid = 0;

	int exSymbol = 0, exStartbit = 0, exCheckbit = 0, exDerection = 0, exCount = 0;

	Cache_inv(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);

	img_data = proc_info->uData1;
	img_wid = proc_info->nVari1;
	img_hei = proc_info->nVari2;

	nLoopCnt = proc_info->nVari5;

	isLearn = proc_info->nVari3;
	setType = proc_info->nVari4;
	setDigit = proc_info->nVari6;
	setValid = proc_info->nVari7;

	for(i = 0; i < nLoopCnt; i++) {
		pSegmentBarcode  = (SegmentBarcodeArea *)proc_info->nData1 + i * 7;
		result_node = (AlgorithmResult *)proc_info->nData2 + nCodeCnt;

		Cache_inv(pSegmentBarcode, sizeof(SegmentBarcodeArea), Cache_Type_ALL, FALSE);

		memset(result_node, 0, sizeof(AlgorithmResult));
		memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);

		ResetOcrParams();

		for(j = 0; j < 2; j++) {
			// 根据标志flag对条码区域图像进行缩放、旋转矫正
			status = RotateImage(img_data, img_wid, img_hei, pSegmentBarcode->corner_ext, pSegmentBarcode->angle,
				j+1, (short *) &rtt_width, (short *) &rtt_height);

			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of RotateImage, slice%d ret_val=%d. --algorithm_run\n", i, status );
#endif
				continue;
			} else if(rtt_width <= 5 || rtt_height <= 5) {
				continue;
			}

			ucBarcodeImage = GetRotateImage();
			if( !ucBarcodeImage ) {
#ifdef	_PRINT_PROMPT
				printf( "Error! Unexpected return of GetRotateImage, slice%d ucBarcodeImage=0x%x\n", DNUM, ucBarcodeImage );
#endif
				continue;
			}

			// 条码区域矫正图像预处理(调整对比度、锐化、二值化)
			status = BarcodeImgProcessIntegrogram(ucBarcodeImage, rtt_width, rtt_height);
			if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
				printf( "Warning! Unexpected return of BarcodeImageProcessing, slice%d ret_val=%d\n", DNUM, status );
#endif
				continue;
			}

			ucDecodeImage = getBarcodeImgProcOutput();
			if( !ucDecodeImage ) {
		#ifdef	_PRINT_PROMPT
				printf( "Error! Unexpected return of getBarcodeImgProcOutput, slice%d ucDecodeImage=0x%x\n", DNUM, ucDecodeImage );
		#endif
				continue;
			}

			// 添加修改部分
			sliceCnt = 0;
			sliceH = rtt_height;
			while(sliceH >= 24) {
				sliceHs[sliceCnt++] = sliceH;
				sliceH = rtt_height / (sliceCnt + 1);
			}
			sliceHs[sliceCnt++] = 16;
			sliceHs[sliceCnt++] = 12;
			sliceHs[sliceCnt++] = 8;
			sliceHs[sliceCnt++] = 4;
			accModuleW = 0.0;
			for(k = 0; k < sliceCnt; k++) {
				codeType = isLearn ? 0 : setType;
				codeDigit = setDigit;
				codeValid = setValid;
				memset(code_result, 0, sizeof(char) * CODE_RESULT_ARR_LENGTH);
				codeModule = codeDirect = codeOffsetL = codeOffsetR = 0;
				nDecodeFlag = BarcodeDecoding_run(ucBarcodeImage, (int *)ucDecodeImage,
						rtt_width, rtt_height, sliceHs[k], &codeType, &codeDigit, &codeValid,
						&codeModule, &codeDirect, &codeOffsetL, &codeOffsetR, &minModuleW, code_result);
				accModuleW = accModuleW + minModuleW;
				if( 1 == nDecodeFlag )
					break;
			}

			accModuleW = accModuleW / sliceCnt;
			if(1 == nDecodeFlag) {
				codeOffsetL /= (j+1);
				codeOffsetR /= (j+1);
				break;
			}
			else if(1 != nDecodeFlag
				&& accModuleW < DECODE_AVGMIN_MODULEW_THRESHOLD
				&& rtt_width < DECODE_IM_WIDTH_THRESHOLD) {
				continue;
			} else {
				break;
			}
		}

		if(1 != nDecodeFlag) {
			exCount = GetOcrParams(&exSymbol, &exStartbit, &exCheckbit, &exDerection);
		}

		// XXX TESTING
#if _DEBUG_OUTPUT_IMAGE_
		if(1 == DNUM && 0 == i) {
			memcpy(ucOutputTestImage, ucDecodeImage, rtt_width * rtt_height);
			nOutputTestImageW = rtt_width;
			nOutputTestImageH = rtt_height;
			nOutputTestImageFlag = 1;
			Cache_wb(ucOutputTestImage, rtt_width * rtt_height, Cache_Type_ALL, TRUE);
			printf("A candidate output image is set!\n");
		}
#endif
		// XXX ENDING

		if( 1 == nDecodeFlag ) {
			UpdateCodeCorner(pSegmentBarcode, codeOffsetL, codeOffsetR);
			memcpy(result_node->strCodeData, code_result, CODE_RESULT_ARR_LENGTH);
			result_node->nFlag = 1;
			result_node->nCodeSymbology = codeType;
			result_node->nCodeCharNum = codeDigit;
			result_node->nCodeOrient = (codeDirect > 0) ? pSegmentBarcode->angle : (pSegmentBarcode->angle + 180);
			result_node->nCodeWidth = abs(pSegmentBarcode->max_ontcpt - pSegmentBarcode->min_ontcpt + 1);
			result_node->nCodeHeight = abs(pSegmentBarcode->max_intcpt - pSegmentBarcode->min_intcpt + 1);
			result_node->nCodeModuleWid = result_node->nCodeWidth * 1000 / codeModule;
			result_node->ptCodeBound1 = (pSegmentBarcode->corner[0].x << 16) | (pSegmentBarcode->corner[0].y & 0xffff);
			result_node->ptCodeBound2 = (pSegmentBarcode->corner[1].x << 16) | (pSegmentBarcode->corner[1].y & 0xffff);
			result_node->ptCodeBound3 = (pSegmentBarcode->corner[2].x << 16) | (pSegmentBarcode->corner[2].y & 0xffff);
			result_node->ptCodeBound4 = (pSegmentBarcode->corner[3].x << 16) | (pSegmentBarcode->corner[3].y & 0xffff);
			ptCenter.x = ptCenter.y = 0;
			for( j = 0; j < 4; j++) {
				ptCenter.x += pSegmentBarcode->corner[j].x;
				ptCenter.y += pSegmentBarcode->corner[j].y;
			}
			ptCenter.x >>= 2;
			ptCenter.y >>= 2;
			result_node->ptCodeCenter = (ptCenter.x << 16) | (ptCenter.y & 0xffff);
			result_node->nCodeSeqNum = i * 7 + proc_info->core_id;
			nCodeCnt++;
			Cache_wb(result_node, sizeof(AlgorithmResult), Cache_Type_ALL, TRUE);
		} else if(0 < exCount) {
			pSegmentBarcode->ex_symbol = exSymbol;
			pSegmentBarcode->ex_startbit = exStartbit;
			pSegmentBarcode->ex_checkbit = exCheckbit;
			pSegmentBarcode->angle = (exDerection > 0) ? pSegmentBarcode->angle : (pSegmentBarcode->angle + 180);
			Cache_wb(pSegmentBarcode, sizeof(SegmentBarcodeArea), Cache_Type_ALL, TRUE);
		}
	}

	proc_info->nFlag = nCodeCnt;

nExit:
	Cache_wb(proc_info, sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
//	Cache_wb(pSegmentBarcode, sizeof(SegmentBarcodeArea), Cache_Type_ALL, TRUE);

	return;
}


void BarcodeDetect_ipcMainProc4()
{
	int ret_val = 0, i = 0, j = 0;

	int waybill_count = 0;
	RyuPoint corners[4];

	AlgorithmResult * pSlaveRsltNode = 0, * pRsltNode = 0;
	AlgorithmResult tmpResult;
	int * first_4char = 0;

	SegmentBarcodeArea * pSegmentBarcodeCollect = stSegmentBarcodeAreaCollection;
	int status = 0, ex_index = -1;
	RyuPoint ptIncptOCR, ptOncptOCR, ptCornerOCR[4];

	int rtt_width = 0, rtt_height = 0, rtt_size = 0;
	unsigned char * ucBarcodeImage = 0;
	unsigned char ucTmp = 0;
	int codeHeight = 0, ocrHeight = 0;

	// 包裹检测
	RyuPoint pkgCentre = ryuPoint(-1, -1);
	RyuRect  pkgBoundBox = ryuRect(-1, -1, 0, 0);

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	if(0 > nBarcodeDetectRetVal) {
		nBarcodeDetectCodeCnt = nBarcodeDetectRetVal;
		goto nExit;
	}

	// 重置包裹检测算法
	WaybillSegm_resetCodeAreaStack();

	// 拷贝结果
	nBarcodeDetectCodeCnt = 0;
	pRsltNode = (AlgorithmResult *)(ucBarcodeDetectResults + sizeof(int));
	for(i = 1; i < NCORENUM; i++) {
		if(1 <= stBarcodeDetectProcInfo[i]->nFlag) {
			pSlaveRsltNode = (AlgorithmResult *) stBarcodeDetectProcInfo[i]->nData2;
			Cache_inv(pSlaveRsltNode, sizeof(AlgorithmResult) * stBarcodeDetectProcInfo[i]->nFlag, Cache_Type_ALL, FALSE);

			for(j = 0; j < stBarcodeDetectProcInfo[i]->nFlag; j++) {
				if(nBarcodeDetectIsLearn) {
					nBarcodeDetectCodeSymbology |= pSlaveRsltNode->nCodeSymbology;
				}

				// 将已识别条码信息给入包裹检测算法
				corners[0] = ryuPoint((pSlaveRsltNode->ptCodeBound1>>16)&0xffff, pSlaveRsltNode->ptCodeBound1&0xffff);
				corners[1] = ryuPoint((pSlaveRsltNode->ptCodeBound2>>16)&0xffff, pSlaveRsltNode->ptCodeBound2&0xffff);
				corners[2] = ryuPoint((pSlaveRsltNode->ptCodeBound3>>16)&0xffff, pSlaveRsltNode->ptCodeBound3&0xffff);
				corners[3] = ryuPoint((pSlaveRsltNode->ptCodeBound4>>16)&0xffff, pSlaveRsltNode->ptCodeBound4&0xffff);
				WaybillSegm_pushCodeAreaStack(corners, pSlaveRsltNode->nCodeOrient, 1, pSlaveRsltNode->strCodeData);

				pRsltNode[nBarcodeDetectCodeCnt++] = pSlaveRsltNode[0];
				pSlaveRsltNode++;
			}
		}
	}

	// 结果按照序号排序
	for(i = 0; i < nBarcodeDetectCodeCnt; i++) {
		for(j = i + 1; j < nBarcodeDetectCodeCnt; j++) {
			if(pRsltNode[i].nCodeSeqNum > pRsltNode[j].nCodeSeqNum) {
				tmpResult = pRsltNode[i];
				pRsltNode[i] = pRsltNode[j];
				pRsltNode[j] = tmpResult;
			}
		}
	}

	// 版本2.1.2修改(20160913)
	// 统计获取候选区域最大grad值，若其小于阈值，且无识别结果，则判定为无条码
	// 此处主要解决无条码时误判定为NOREAD的情况
	if(0 >= nBarcodeDetectCodeCnt && 24 > nLctClusMaxGradVal) {
		nBarcodeDetectCodeCnt = nBarcodeDetectRetVal = -1;
	}

	// 添加包裹检测功能
	if(nBarcodeDetectMultiPkgDetect) {
		waybill_count = WaybillSegment(pBarcodeDetectInDataPtr, nBarcodeDetectInDataWid, nBarcodeDetectInDataHei, &pkgBoundBox);
		// 非法返回值
		if(0 > waybill_count || 3 < waybill_count) {
#ifdef	_PRINT_PROMPT
			printf("Warning, unexpected return of WaybillSegment, waybill_count=%d\n", waybill_count);
#endif
			goto nExit;
		}
		// 没有找到包裹区域
		else if(0 == waybill_count) {
			if(0 == nBarcodeDetectCodeCnt) {
				nBarcodeDetectCodeCnt = -1;
			}
		}
		// 找到一个包裹区域
		else if(1 == waybill_count) {
			if(0 > nBarcodeDetectCodeCnt) {
				nBarcodeDetectCodeCnt = 0;
			}
			// 写入包裹位置信息
			pkgCentre.x = pkgBoundBox.x + pkgBoundBox.width / 2;
			pkgCentre.y = pkgBoundBox.y + pkgBoundBox.height / 2;
			if(0 < pkgCentre.x && 0 < pkgCentre.y) {
				memset(&tmpResult, 0, sizeof(AlgorithmResult));
				tmpResult.nFlag = 0x4;
				tmpResult.ptCodeCenter = (pkgCentre.x << 16) | pkgCentre.y;
				tmpResult.ptCodeBound1 = pkgBoundBox.x;
				tmpResult.ptCodeBound2 = pkgBoundBox.y;
				tmpResult.ptCodeBound3 = pkgBoundBox.x + pkgBoundBox.width;
				tmpResult.ptCodeBound4 = pkgBoundBox.y + pkgBoundBox.height;
				tmpResult.nCodeWidth = pkgBoundBox.width;
				tmpResult.nCodeHeight = pkgBoundBox.height;
				pRsltNode[nBarcodeDetectCodeCnt] = tmpResult;
			}
		}
		// 找到疑似多个包裹区域
		else if(2 == waybill_count) {
			if(0 >= nBarcodeDetectCodeCnt) {
				nBarcodeDetectCodeCnt = 1 << 16;
			} else {
				nBarcodeDetectCodeCnt = (1 << 16) | nBarcodeDetectCodeCnt;
			}
		}
		// 找到多个包裹区域
		else if(3 == waybill_count) {
			if(0 >= nBarcodeDetectCodeCnt) {
				nBarcodeDetectCodeCnt = 2 << 16;
			} else {
				nBarcodeDetectCodeCnt = (2 << 16) | nBarcodeDetectCodeCnt;
			}
		}
	} else if(0 >= nBarcodeDetectCodeCnt) {
		waybill_count = WaybillSegment(pBarcodeDetectInDataPtr, nBarcodeDetectInDataWid, nBarcodeDetectInDataHei, &pkgBoundBox);
		if(0 > waybill_count || 3 < waybill_count) {
#ifdef	_PRINT_PROMPT
			printf("Warning, unexpected return of WaybillSegment, waybill_count=%d\n", waybill_count);
#endif
			goto nExit;
		}
		// 没有找到包裹区域
		else if(0 == waybill_count) {
			nBarcodeDetectCodeCnt = -1;
		}
		// 找到包裹区域
		else if(0 < waybill_count) {
			nBarcodeDetectCodeCnt = 0;
		}
	}

	// XXX TESTING
#if _DEBUG_OUTPUT_IMAGE_
	if(1 == nOutputTestImageFlag) {
		pRsltNode[0].pOcrIm = (char *)ucOutputTestImage;
		pRsltNode[0].nOcrImWid = nOutputTestImageW;
		pRsltNode[0].nOcrImHei = nOutputTestImageH;
	}
#endif
	// XXX ENDING

	// 截取供OCR识别的图像
	if(0 == nBarcodeDetectCodeCnt) {
		for(i = 0; i < nSegmentBarcodeAreaCnt; i++) {
			if(0 < pSegmentBarcodeCollect[i].ex_symbol) {
				ex_index = i;
				break;
			}
		}

		if(0 <= ex_index) {
			codeHeight = abs(pSegmentBarcodeCollect[ex_index].max_intcpt - pSegmentBarcodeCollect[ex_index].min_intcpt + 1);
			ocrHeight = RYUMIN(60, RYUMAX(codeHeight>>1, 36));

			if(pSegmentBarcodeCollect[ex_index].angle < 180) {
				ptIncptOCR.x = pSegmentBarcodeCollect[ex_index].max_intcpt;
				//ptIncptOCR.y = pSegmentBarcodeCollect[ex_index].max_intcpt + 36;
				ptIncptOCR.y = pSegmentBarcodeCollect[ex_index].max_intcpt + ocrHeight;
				ptOncptOCR.x = pSegmentBarcodeCollect[ex_index].min_ontcpt;
				ptOncptOCR.y = pSegmentBarcodeCollect[ex_index].max_ontcpt;
				InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, pSegmentBarcodeCollect[ex_index].angle, ptCornerOCR);
				status = RotateImage(pBarcodeDetectInDataPtr, nBarcodeDetectInDataWid, nBarcodeDetectInDataHei,
						ptCornerOCR, pSegmentBarcodeCollect[ex_index].angle,
						1, (short *) &rtt_width, (short *) &rtt_height);

				if( status <= 0 ) {
		#ifdef	_PRINT_PROMPT
						printf( "Warning! Unexpected return of RotateImage, slice%d ret_val=%d\n", DNUM, status );
		#endif
						goto nExit;
				}

				ucBarcodeImage = GetRotateImage();
				if( !ucBarcodeImage ) {
		#ifdef	_PRINT_PROMPT
					printf( "Error! Unexpected return of GetRotateImage, slice%d ucBarcodeImage=0x%x\n", DNUM, ucBarcodeImage );
			#endif
					goto nExit;
				}
				Cache_wb(ucBarcodeImage, rtt_width * rtt_height * sizeof(char), Cache_Type_ALL, TRUE);
			} else {
				//ptIncptOCR.x = pSegmentBarcodeCollect[ex_index].min_intcpt - 36;
				ptIncptOCR.x = pSegmentBarcodeCollect[ex_index].min_intcpt - ocrHeight;
				ptIncptOCR.y = pSegmentBarcodeCollect[ex_index].min_intcpt;
				ptOncptOCR.x = pSegmentBarcodeCollect[ex_index].min_ontcpt;
				ptOncptOCR.y = pSegmentBarcodeCollect[ex_index].max_ontcpt;
				InterceptCvt2Corners(ptIncptOCR, ptOncptOCR, pSegmentBarcodeCollect[ex_index].angle-180, ptCornerOCR);
				status = RotateImage(pBarcodeDetectInDataPtr, nBarcodeDetectInDataWid, nBarcodeDetectInDataHei,
						ptCornerOCR, pSegmentBarcodeCollect[ex_index].angle-180,
						1, (short *) &rtt_width, (short *) &rtt_height);

				if( status <= 0 ) {
		#ifdef	_PRINT_PROMPT
						printf( "Warning! Unexpected return of RotateImage, slice%d ret_val=%d\n", DNUM, status );
		#endif
						goto nExit;
				}

				ucBarcodeImage = GetRotateImage();
				if( !ucBarcodeImage ) {
		#ifdef	_PRINT_PROMPT
					printf( "Error! Unexpected return of GetRotateImage, slice%d ucBarcodeImage=0x%x\n", DNUM, ucBarcodeImage );
			#endif
					goto nExit;
				}

				rtt_size = rtt_width * rtt_height;
				for(i = 0; i < (rtt_size>>1); i++) {
					ucTmp = ucBarcodeImage[i];
					ucBarcodeImage[i] = ucBarcodeImage[rtt_size-i-1];
					ucBarcodeImage[rtt_size-i-1] = ucTmp;
				}
				Cache_wb(ucBarcodeImage, rtt_size * sizeof(char), Cache_Type_ALL, TRUE);
			}
			pRsltNode[0].nFlag |= 0x8;
			pRsltNode[0].pOcrIm = (char *)ucBarcodeImage;
			pRsltNode[0].nOcrImWid = rtt_width;
			pRsltNode[0].nOcrImHei = rtt_height;
			pRsltNode[0].nOcrExtra = (codeHeight << 16)
						| (pSegmentBarcodeCollect[ex_index].ex_startbit << 8)
						| pSegmentBarcodeCollect[ex_index].ex_checkbit;
		}
	}


nExit:
	first_4char = (int *) ucBarcodeDetectResults;
	first_4char[0] = nBarcodeDetectCodeCnt;

	Cache_wb(ucBarcodeDetectResults, sizeof(AlgorithmResult) * MAX_BARCODE_COUNT + sizeof(int), Cache_Type_ALL, TRUE);

	ret_val = nBarcodeDetectCodeCnt;
	stBarcodeDetectProcInfo[1]->nFlag = ret_val;
	stBarcodeDetectProcInfo[1]->nVari1 = nBarcodeDetectCodeSymbology;

	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(stBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return;
}


