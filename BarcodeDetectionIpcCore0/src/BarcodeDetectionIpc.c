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
IHeap_Handle gHeapID = 0;

char * gFileVersion = "2.6.1";
int gnBarcodeDetectRetVal = 0;			// 全局返回值

int gnIsBarcodeDetectInit = 0;
int gnBarcodeDetectInitImgWid = 0;
int gnBarcodeDetectInitImgHei = 0;

unsigned char * gBarcodeDetectResults = 0;
barcode_detect_proc_info_t ** gBarcodeDetectProcInfo;

AlgorithmResult ** gBarcodeDetectResultNode;
BarcodeGlobalPointers * gstGlobalPtrs[NTHREADS];

// 相机标定
//const int nConstDetectDia = 200;
#define nConstDetectDia (200)
const int nConstDetectThk = 50;
unsigned char gcCalibrationRegister[32];

#if _DEBUG_OUTPUT_IMAGE_
unsigned char * gOutputTestImage = 0;
#endif

/************************************************************************/
/* 设置参数										*/
/************************************************************************/
int gnIsLearnSymbol = 0;
int gnCodeSymbology = 0;
int gnRglCodeCnt = 0;
int gnCodeDgtNum = 0;
int gnCodeDgtNumExt = 0;
int gnCodeValidity = 0;
int gnCodeValidityExt = 0;
int gnMultiPkgDetect = 0;


/************************************************************************/
/* 算法初始化函数									*/
/************************************************************************/
int BarcodeDetect_init_ipcMasterProc0(IHeap_Handle heap_id, int img_max_wid, int img_max_hei)
{
	int status= 0, ret_val = 0;
	int i = 0;

	if(1 == gnIsBarcodeDetectInit) {
		gnIsBarcodeDetectInit = 0;
		BarcodeDetect_release(gHeapID);
	}

	platform_write("\nImage-processing C Runtime Library loading... \n");

	gnIsLearnSymbol = gnCodeSymbology = gnRglCodeCnt = 0;
	gnCodeDgtNum = gnCodeDgtNumExt = gnCodeValidity = gnCodeValidityExt = 0;
	gnMultiPkgDetect = 0;
	gnBarcodeDetectInitImgWid = img_max_wid;
	gnBarcodeDetectInitImgHei = img_max_hei;

	if(gnBarcodeDetectInitImgWid <= 0 || gnBarcodeDetectInitImgHei <= 0) {
		ret_val = -10112002;
		goto nExit;
	}

	gHeapID = heap_id;

	gBarcodeDetectResults = (unsigned char *) Memory_alloc (gHeapID,
			ROUNDUP(sizeof(AlgorithmResult) * MAX_BARCODE_COUNT + sizeof(int), MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gBarcodeDetectResults) {
		ret_val = -10112001;
		printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
		goto nExit;
	}

	gBarcodeDetectProcInfo =  (barcode_detect_proc_info_t **) Memory_alloc (gHeapID,
			sizeof(barcode_detect_proc_info_t *) * NCORENUM, NULL, NULL);
	if(!gBarcodeDetectProcInfo) {
		ret_val = -10112004;
		printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
		goto nExit;
	}
	for(i = 0; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i] = (barcode_detect_proc_info_t *) Memory_alloc (gHeapID,
				ROUNDUP(sizeof(barcode_detect_proc_info_t), MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gBarcodeDetectProcInfo[i]) {
			ret_val = -10112004;
			printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
			goto nExit;
		}
		memset(gBarcodeDetectProcInfo[i], 0, sizeof(barcode_detect_proc_info_t));
		gBarcodeDetectProcInfo[i]->core_id 		= i;
		gBarcodeDetectProcInfo[i]->proc_type 	= null_proc;
	}

	gBarcodeDetectResultNode = (AlgorithmResult **) Memory_alloc (gHeapID,
			sizeof(AlgorithmResult *) * NTHREADS, NULL, NULL);
	if(!gBarcodeDetectResultNode) {
		ret_val = -10112004;
		printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
		goto nExit;
	}
	for(i = 0; i < NTHREADS; i++) {
		gBarcodeDetectResultNode[i] = (AlgorithmResult *) Memory_alloc (gHeapID,
				MAX_SLAVE_BARCODE_COUNT * sizeof(AlgorithmResult), MAX_CACHE_LINE, NULL);
		if(!gBarcodeDetectResultNode[i]) {
			ret_val = -10112004;
			printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
			goto nExit;
		}
	}

	for(i = 0; i < NTHREADS; i++) {
		gstGlobalPtrs[i] = (BarcodeGlobalPointers *) Memory_alloc (gHeapID,
				ROUNDUP(sizeof(BarcodeGlobalPointers), MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
		if(!gstGlobalPtrs[i]) {
			ret_val = -10112004;
			printf("\n--Ryu--BarcodeDetect_init results Alloc failed, %d\n", ret_val);
			goto nExit;
		}
		memset(gstGlobalPtrs[i], 0, sizeof(BarcodeGlobalPointers));
		gstGlobalPtrs[i]->nBarcodeDetectInitImgWid = gnBarcodeDetectInitImgWid;
		gstGlobalPtrs[i]->nBarcodeDetectInitImgHei = gnBarcodeDetectInitImgHei;
		gstGlobalPtrs[i]->ucBarcodeDetectResults = gBarcodeDetectResults;
		gstGlobalPtrs[i]->stBarcodeDetectProcInfo = (int **)gBarcodeDetectProcInfo;
		gstGlobalPtrs[i]->stBarcodeDetectResultNode = (int **)gBarcodeDetectResultNode;
	}

	status = BarcodeLocate_init(gHeapID, gnBarcodeDetectInitImgWid, gnBarcodeDetectInitImgHei, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

	status = BarcodeSegment_init(gHeapID, gnBarcodeDetectInitImgWid, gnBarcodeDetectInitImgHei, 128000, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

	status = BarcodeRotate_init(gHeapID, gnBarcodeDetectInitImgWid, gnBarcodeDetectInitImgHei, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

	status = BarcodeImgproc_init(gHeapID, gnBarcodeDetectInitImgWid, gnBarcodeDetectInitImgHei, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

	status = BarcodeDecode_init(gHeapID, gnBarcodeDetectInitImgWid, gnBarcodeDetectInitImgHei, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

	status = WaybillSegment_init(gHeapID, ryuSize(WAYBILLSEG_RESIZE_SCALE, WAYBILLSEG_RESIZE_SCALE), 1024, gstGlobalPtrs);
	if(1 != status) {
		ret_val = status;
		goto nExit;
	}

#if  _DEBUG_OUTPUT_IMAGE_
	gOutputTestImage = (unsigned char *) Memory_alloc (gHeapID,
			ROUNDUP(gnBarcodeDetectInitImgWid * gnBarcodeDetectInitImgHei, MAX_CACHE_LINE), MAX_CACHE_LINE, NULL);
	if(!gOutputTestImage) {
		ret_val = -10112009;
		printf("\n--Ryu--gOutputTestImage Alloc failed, %d\n", ret_val);
		goto nExit;
	}
	for(i = 0; i < NTHREADS; i++) {
		gstGlobalPtrs[i]->ucOutputTestImage = gOutputTestImage;
	}
#endif

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = mapBarcodeGlobalPtrs;
		gBarcodeDetectProcInfo[i]->nData1 = (int *) gstGlobalPtrs[i-1];
		Cache_wb(gBarcodeDetectProcInfo[i]->nData1, sizeof(BarcodeGlobalPointers), Cache_Type_ALL, TRUE);
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}

	ret_val = 1;

nExit:

	return ret_val;
}

int BarcodeDetect_init_ipcMasterProc1()
{
	int i = 0, status = 0, ret_val = 0;

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
		status = gBarcodeDetectProcInfo[i]->nFlag;
		if(1 != status) {
			ret_val = -10112000 - i * 100 + status;
			printf("---Ryu--- Core%d mapBarcodeGlobalPtrs ERROR, BarcodeDetect init failed, %d\n", i, ret_val);
			goto nExit;
		}
	}

	platform_write("Image-processing C Runtime Library loaded successfully, \n");
	platform_write("    version: %s, Copyright (C) 中科贯微(Govoauto) \n\n", gFileVersion);

	gnIsBarcodeDetectInit = 1;
	ret_val = 1;

nExit:
	return ret_val;
}

void BarcodeDetect_release()
{
	int i = 0;

	gnIsBarcodeDetectInit = 0;

	gnIsLearnSymbol = gnCodeSymbology = gnRglCodeCnt = 0;
	gnCodeDgtNum = gnCodeDgtNumExt = gnCodeValidity = gnCodeValidityExt = 0;
	gnMultiPkgDetect = 0;

	if(gBarcodeDetectResults) {
		Memory_free(gHeapID, gBarcodeDetectResults,
				ROUNDUP(sizeof(AlgorithmResult) * MAX_BARCODE_COUNT + sizeof(int), MAX_CACHE_LINE));
		gBarcodeDetectResults = 0;
	}

	for(i = 0; i < NCORENUM; i++) {
		if(gBarcodeDetectProcInfo[i]) {
			Memory_free(gHeapID, gBarcodeDetectProcInfo[i], ROUNDUP(sizeof(barcode_detect_proc_info_t), MAX_CACHE_LINE));
			gBarcodeDetectProcInfo[i] = 0;
		}
	}
	if(gBarcodeDetectProcInfo) {
		Memory_free(gHeapID, gBarcodeDetectProcInfo, sizeof(barcode_detect_proc_info_t *) * NCORENUM);
		gBarcodeDetectProcInfo = 0;
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gBarcodeDetectResultNode[i]) {
			Memory_free(gHeapID, gBarcodeDetectResultNode[i], MAX_SLAVE_BARCODE_COUNT * sizeof(AlgorithmResult));
			gBarcodeDetectResultNode[i] = 0;
		}
	}
	if(gBarcodeDetectResultNode) {
		Memory_free(gHeapID, gBarcodeDetectResultNode, sizeof(AlgorithmResult *) * NTHREADS);
		gBarcodeDetectResultNode = 0;
	}

	for(i = 0; i < NTHREADS; i++) {
		if(gstGlobalPtrs[i]) {
			Memory_free(gHeapID, gstGlobalPtrs[i], ROUNDUP(sizeof(BarcodeGlobalPointers), MAX_CACHE_LINE));
			gstGlobalPtrs[i] = 0;
		}
	}

	BarcodeLocate_release(gHeapID);

	BarcodeSegment_release(gHeapID);

	BarcodeRotate_release(gHeapID);

	BarcodeImgproc_release(gHeapID);

	BarcodeDecode_release(gHeapID);

	WaybillSegment_release(gHeapID);

#if  _DEBUG_OUTPUT_IMAGE_
	Memory_free(gHeapID, gOutputTestImage, ROUNDUP(gnBarcodeDetectInitImgWid * gnBarcodeDetectInitImgHei, MAX_CACHE_LINE));
	gOutputTestImage = 0;
#endif
}

/************************************************************************/
/* 相机标定算法																*/
/************************************************************************/
void BarcodeDetect_calibration_reset()
{
	memset(gcCalibrationRegister, 0, 32 * sizeof(char));
	gcCalibrationRegister[31] = 0x1;
}

int BarcodeDetect_calibration(unsigned char * img_data, int img_wid, int img_hei,
		CalibrateResult * results)
{
	int ret_val = 0;
	int i = 0, sum = 0, nTmp = 0;
	int lineArr[nConstDetectDia] = {0};

	unsigned char * pIm = 0;
	RyuPoint code_cent;

	const float low_ratio = 0.15, high_ratio = 0.15;
	int low_scale, high_scale, min_scale, max_scale, avg_scale, mid_scale, grav_scale;

	int sum1 = 0, sum2 = 0;
	int gradState = 0, currState = 0, gradMax = 0, gradIdx = 0, gradThresh = 0, gradEffc = 0;

	if(NULL == img_data || NULL == results) {
		ret_val = -1;
		goto nExit;
	}

	results->nFlag = 0;
	code_cent.x = img_wid >> 1;
	code_cent.y = img_hei >> 1;

	if(0 > code_cent.x - nConstDetectDia || img_wid < code_cent.x + nConstDetectDia
		|| 0 > code_cent.y - nConstDetectThk || img_hei < code_cent.y + nConstDetectThk) {
		ret_val = -1;
		goto nExit;
	}

	// 获取图像中心区域局部指标
	pIm = img_data + (code_cent.y-(nConstDetectThk>>1)) * img_wid
			+ code_cent.x - (nConstDetectDia>>1);
	ryuImageContrastAnalyze(pIm, nConstDetectDia, nConstDetectThk, img_wid, 0,
		low_ratio, &low_scale, high_ratio, &high_scale,
		&min_scale, &max_scale, &avg_scale, &mid_scale, &grav_scale);

	results->nMaxGreyLevel = RYUMAX(0, RYUMIN(255, max_scale));
	results->nMinGreyLevel = RYUMAX(0, RYUMIN(255, min_scale));
	results->nWhiteLevel = RYUMAX(0, RYUMIN(255, high_scale));
	results->nBlackLevel = RYUMAX(0, RYUMIN(255, low_scale));

	results->nCurrContrast = RYUMAX(0, RYUMIN(255, high_scale - low_scale));
	results->nCurrLuminance = RYUMAX(0, RYUMIN(255, (high_scale + low_scale) >> 1));

	// 计算中线中心局部偏差值，梯度值
	nTmp = (high_scale + low_scale) >> 1;
	sum = 0;
	pIm = img_data + code_cent.y * img_wid + code_cent.x - (nConstDetectDia>>1);
	for(i = 0; i < nConstDetectDia; i++) {
 		lineArr[i] = (int)pIm[i];
		sum += (lineArr[i]-nTmp) * (lineArr[i]-nTmp);

		lineArr[i] = (int)pIm[i] - (int)pIm[i+1];
	}

	sum = (int)(sqrt(1.0 * sum / nConstDetectDia) + 0.5);

	// 寻找梯度数组每一个正负区间的最大值
	gradThresh = (high_scale - low_scale) >> 2;
	nTmp = RYUMAX(10, gradThresh);
	gradState = (lineArr[0] == 0) ? 0 : (lineArr[0] / abs(lineArr[0]));
	gradMax = abs(lineArr[0]);
	gradIdx = 0;
	lineArr[0] = (lineArr[0] > 0) ? (0 - lineArr[0]) : lineArr[0];
	for(i = 1; i < nConstDetectDia-1; i++) {
		currState = (lineArr[i] == 0) ? 0 : (lineArr[i] / abs(lineArr[i]));
		if(currState != gradState) {
			if(gradMax >= nTmp) {
				lineArr[gradIdx] = 0 - lineArr[gradIdx];
				gradEffc++;
			}
			gradState = currState;
			gradMax = abs(lineArr[i]);
			gradIdx = i;
		} else {
			if(gradMax < abs(lineArr[i])) {
				gradMax = abs(lineArr[i]);
				gradIdx = i;
			}
		}
		lineArr[i] = (lineArr[i] > 0) ? (0 - lineArr[i]) : lineArr[i];
	}
	// 最后一个区间处理
	if(gradMax >= nTmp) {
		lineArr[gradIdx] = 0 - lineArr[gradIdx];
		gradEffc++;
	}

	// 获取有效梯度值和扰动梯度值
	for(i = 0; i < nConstDetectDia-1; i++) {
		if(lineArr[i] > 0) {
			sum1 += lineArr[i];
		} else {
			sum2 -= lineArr[i];
		}
	}

	if(sum1 > 0 && gradEffc > 0)
		sum1 /= gradEffc;
	if(sum2 > 0 && gradEffc < nConstDetectDia - 1)
		sum2 /= (nConstDetectDia - 1 - gradEffc);

	// 计算当前条码质量
	nTmp = (sum1 - sum2 + sum + gradThresh) * 255 / 446;
	nTmp = RYUMAX(1, RYUMIN(255, nTmp));
	results->nCurrQuality = nTmp;

	// 检查计算最佳质量
	gcCalibrationRegister[0] = RYUMAX(gcCalibrationRegister[0], nTmp);
	results->nLmaxQuality = gcCalibrationRegister[0];

	// 将当前值加入"队列"，更新"队列"
	for(i = 30; i > 0; i--) {
		gcCalibrationRegister[i] = gcCalibrationRegister[i-1];
	}
	gcCalibrationRegister[1] = nTmp;

	// 计算条码质量变化趋势
	sum = nTmp = 0;
	for(i = 1; i < 11; i++) {
		sum += (gcCalibrationRegister[i] - gcCalibrationRegister[i+1]);
		if(5 < abs(sum)) {
			nTmp = 1;
			break;
		}
	}
	if(nTmp) {
		gcCalibrationRegister[31] = (sum > 0) ? 0x1 : 0xff;
	}
	results->nSignQuality = (0xff == gcCalibrationRegister[31]) ? -1 : 1;

	ret_val = results->nFlag = 1;

nExit:
	return ret_val;
}


/************************************************************************/
/* 算法参数设置函数													*/
/************************************************************************/
int BarcodeDetect_setparams(AlgorithmParamSet * paramset)
{
	int ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = -10112013;
		printf("\n--Ryu--Algorithm Setparams without initialization, DO Init first of all, %d\n", ret_val);
		goto nExit;
	}

	if(!paramset) {
		ret_val = -10112011;
		printf("\n--Ryu--Invalid input parameters in Algorithm Getparams operation, %d\n", ret_val);
		goto nExit;
	}

	gnRglCodeCnt = paramset->nCodeCount;
	if(gnRglCodeCnt < 0) {
		gnRglCodeCnt = 0;	// 还原初始值
		ret_val = -10112014;
		printf("\n--Ryu--Invalid input parameters in Algorithm Setparams operation, %d\n", ret_val);
		goto nExit;
	}

	// XXX--存在设置无法生效的隐患
	gnCodeSymbology = paramset->nCodeSymbology;
//	if(gnCodeSymbology < 0) {
//		gnCodeSymbology = 0;	// 还原初始值
//		ret_val = -10112015;
//		printf("\n--Ryu--Invalid input parameters in Algorithm Setparams operation, %d\n", ret_val);
//		goto nExit;
//	}

	gnCodeDgtNum = paramset->nCodeDgtNum;
	gnCodeDgtNumExt = paramset->nCodeDgtNumExt;

	gnCodeValidity = paramset->nCodeValidity;
	gnCodeValidityExt = paramset->nCodeValidityExt;

	gnMultiPkgDetect = paramset->nMultiPkgDetect;

	ret_val = 9;

nExit:

	return ret_val;
}

int BarcodeDetect_getparams(AlgorithmParamSet * paramset)
{
	int ret_val = 0;

	if(!paramset) {
		ret_val = -10112011;
		printf("\n--Ryu--Invalid input parameters in Algorithm Getparams operation, %d\n", ret_val);
		goto nExit;
	}

	paramset->nCodeCount = gnRglCodeCnt;
	paramset->nCodeSymbology = gnCodeSymbology;
	paramset->nCodeDgtNum = gnCodeDgtNum;
	paramset->nCodeDgtNum = gnCodeDgtNumExt;
	paramset->nCodeValidity = gnCodeValidity;
	paramset->nCodeValidityExt = gnCodeValidityExt;
	paramset->nMultiPkgDetect = gnMultiPkgDetect;

	ret_val = 9;

nExit:

	return ret_val;
}

barcode_detect_proc_info_t ** BarcodeDetect_getIpcProcInfo()
{
	return gBarcodeDetectProcInfo;
}

void BarcodeDetect_run_ipcSlaveProc(barcode_detect_proc_info_t * proc_info)
{
	return;
}

// 入口参数分发传递给CORE1进行【串行算法0】，为【并行算法0】做分块准备，CORE2-7空置
int BarcodeDetect_run_ipcMasterProc0(int lrning_flag, unsigned char * img_data, int img_wid, int img_hei)
{
	int i = 0, ret_val = 0;

	gnBarcodeDetectRetVal = 0;
	gnIsLearnSymbol = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112018;
		printf("\n---Ryu--- Algorithm runs without initialization, DO Init before run it, %d\n", ret_val);
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;

#ifdef DEBUG_OUTPUT_IMAGE
		gBarcodeDetectProcInfo[i]->nVari8 = 0;
#endif
	}

	gnIsLearnSymbol = lrning_flag;

	// Core0向Core1传参
	gBarcodeDetectProcInfo[1]->proc_type = main_proc0;
	gBarcodeDetectProcInfo[1]->uData1 = img_data;
	gBarcodeDetectProcInfo[1]->nVari1 = lrning_flag;
	gBarcodeDetectProcInfo[1]->nVari2 = img_wid;
	gBarcodeDetectProcInfo[1]->nVari3 = img_hei;
	gBarcodeDetectProcInfo[1]->nVari4 = gnRglCodeCnt;
	gBarcodeDetectProcInfo[1]->nVari5 = gnCodeSymbology;
	gBarcodeDetectProcInfo[1]->nVari6 = gnCodeDgtNum;
	gBarcodeDetectProcInfo[1]->nVari7 = gnCodeValidity;
	gBarcodeDetectProcInfo[1]->nVari8 = gnCodeValidityExt;
	gBarcodeDetectProcInfo[1]->nVari9 = gnMultiPkgDetect;

	ret_val = 1;

	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【串行算法0】返回标识，分发至CORE1-7运行【并行算法0】
int BarcodeDetect_run_ipcMasterProc1()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112025;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	// 每次验证都需要Cache_inv！！！
	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	for(i = 1; i < NCORENUM; i++) {
		status = gBarcodeDetectProcInfo[i]->nFlag;
		if(1 != status) {
			ret_val = gnBarcodeDetectRetVal = status;
			printf("---Ryu--- Core%d main_proc0 ERROR, %d\n", i, ret_val);
			goto nExit;
		}
	}

//	status = gBarcodeDetectProcInfo[1]->nFlag;
//	if(0 > status) {
//		ret_val = status;
//		printf("---Ryu--- Core1 main_proc2 ERROR, %d\n", ret_val);
//		goto nExit;
//	} else if(0 == status) {
//		ret_val = status;
//		goto nExit;
//	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = slave_proc0;
	}

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【并行算法0】返回标识，分发至CORE1运行【串行算法1】，为【并行算法1】做分块准备，CORE2-7空置
int BarcodeDetect_run_ipcMasterProc2()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112026;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	for(i = 1; i < NCORENUM; i++) {
		status = gBarcodeDetectProcInfo[i]->nFlag;
		if(1 != status) {
			ret_val = gnBarcodeDetectRetVal = status;
			printf("---Ryu--- Core%d slave_proc0 ERROR, %d\n", i, ret_val);
			goto nExit;
		}
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	gBarcodeDetectProcInfo[1]->proc_type = main_proc1;

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【串行算法1】返回标识，分发至CORE1-7运行【并行算法1】
int BarcodeDetect_run_ipcMasterProc3()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112027;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	for(i = 1; i < NCORENUM; i++) {
		status = gBarcodeDetectProcInfo[i]->nFlag;
		if(1 != status) {
			ret_val = gnBarcodeDetectRetVal = status;
			printf("---Ryu--- Core%d main_proc1 ERROR, %d\n", i, ret_val);
			goto nExit;
		}
	}

//	status = gBarcodeDetectProcInfo[1]->nFlag;
//	if(0 > status) {
//		ret_val = status;
//		printf("---Ryu--- Core1 main_proc2 ERROR, %d\n", ret_val);
//		goto nExit;
//	} else if(0 == status) {
//		ret_val = status;
//		goto nExit;
//	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = slave_proc1;
	}

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}


// 检验【并行算法1】返回标识，分发至CORE1运行【串行算法2】，为【并行算法2】做分块准备，CORE2-7空置
int BarcodeDetect_run_ipcMasterProc4()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112028;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	for(i = 1; i < NCORENUM; i++) {
		status = gBarcodeDetectProcInfo[i]->nFlag;
		if(1 != status) {
			ret_val = gnBarcodeDetectRetVal = status;
			printf("---Ryu--- Core%d slave_proc1 ERROR, %d\n", i, ret_val);
			goto nExit;
		}
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	gBarcodeDetectProcInfo[1]->proc_type = main_proc2;

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【串行算法2】返回标识，分发至CORE1-7运行【并行算法2】
int BarcodeDetect_run_ipcMasterProc5()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112029;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	status = gBarcodeDetectProcInfo[1]->nFlag;
	if(-100 > status) {
		ret_val = gnBarcodeDetectRetVal = status;
		printf("---Ryu--- Core1 main_proc2 ERROR, %d\n", ret_val);
		goto nExit;
	} else if(0 >= status) {	// 未找到条码，给出特定返回值
		ret_val = gnBarcodeDetectRetVal = status;
		goto nExit;
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;

		if(i <= status) {
			gBarcodeDetectProcInfo[i]->proc_type = slave_proc2;
		}
	}

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【并行算法2】返回标识，分发至CORE1运行【串行算法3】
int BarcodeDetect_run_ipcMasterProc6()
{
	int i = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112030;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	gBarcodeDetectProcInfo[1]->proc_type = main_proc3;

	ret_val = 1;

	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【串行算法3】返回标识，分发至CORE1-7运行【并行算法3】
int BarcodeDetect_run_ipcMasterProc7()
{
	int i = 0, status = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112031;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	status = gBarcodeDetectProcInfo[1]->nFlag;
	if(-100 > status) {
		ret_val = gnBarcodeDetectRetVal = status;
		printf("---Ryu--- Core1 main_proc3 ERROR, %d\n", ret_val);
		goto nExit;
	} else if(0 >= status) {	// 未找到条码，给出特定返回值
		ret_val = gnBarcodeDetectRetVal = status;
		goto nExit;
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->nFlag = 0;
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;

		if(i <= status) {
			gBarcodeDetectProcInfo[i]->proc_type = slave_proc3;
		}
	}

	ret_val = 1;

nExit:
	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 检验【并行算法3】返回标识，分发至CORE1运行【串行算法4】
int BarcodeDetect_run_ipcMasterProc8()
{
	int i = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112032;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->core_id = i;
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	gBarcodeDetectProcInfo[1]->proc_type = main_proc4;

	ret_val = 1;

	for(i = 1; i < NCORENUM; i++) {
		Cache_wb(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, TRUE);
	}
	return ret_val;
}

// 接收【串行算法4】返回标识，输出识别结果
int BarcodeDetect_run_ipcMasterProc9(unsigned char ** results)
{
	int i = 0, ret_val = 0;

	if(1 != gnIsBarcodeDetectInit) {
		ret_val = gnBarcodeDetectRetVal = -10112033;
		*results = 0;
		printf("\n--Ryu--Algorithm runs without initialization, DO Init before run it!\n");
		return ret_val;
	}

	for(i = 1; i < NCORENUM; i++) {
		Cache_inv(gBarcodeDetectProcInfo[i], sizeof(barcode_detect_proc_info_t), Cache_Type_ALL, FALSE);
	}

	for(i = 1; i < NCORENUM; i++) {
		gBarcodeDetectProcInfo[i]->proc_type = null_proc;
	}

	ret_val = gBarcodeDetectProcInfo[1]->nFlag;
	if(-100 > ret_val) {
		printf("---Ryu--- Core1 main_proc4 ERROR, %d\n", ret_val);
		goto nExit;
	}

	if(gnIsLearnSymbol)
		gnCodeSymbology = gBarcodeDetectProcInfo[1]->nVari1;


nExit:
	*results = gBarcodeDetectResults;
	return ret_val;
}


#ifdef DEBUG_OUTPUT_IMAGE
void getDebugOutputImage(unsigned char ** output_images, int * output_wid, int * output_hei, int * output_flags)
{
	int i = 0;

	for(i = 0; i < NTHREADS; i++) {
		output_images[i] = gstGlobalPtrs[i]->ucRotateImage;
		output_wid[i] = gBarcodeDetectProcInfo[i+1]->nVari6;
		output_hei[i] = gBarcodeDetectProcInfo[i+1]->nVari7;
		output_flags[i] = gBarcodeDetectProcInfo[i+1]->nVari8;
	}
}
#endif

