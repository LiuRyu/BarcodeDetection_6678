/*
 * BarcodeFncRotate.c
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#include "BarcodeFncLocate.h"

#define FNC_AVG_VALUE(v0, c0, v1, c1)	(((v0) * (c0) + (v1) * (c1)) / ((c0) + (c1)))

#define GRADHIST_THRE					(36)

//////////////////////////////////////////////////////////////////////////
// [v2.6.1] 图像中存在强反光时，条码提取不完整
// [v2.6.1] 注释为
//#define AUTOCONTRAST_THRESHOLD_RATIO_L	(0.05)
// [v2.6.1] 更改为
#define AUTOCONTRAST_THRESHOLD_RATIO_L	(0.1)
// [v2.6.1] 新增为
#define LOCATE_OUTPUT_MAXWIDTH			(1200)
#define LOCATE_OUTPUT_MAXHEIGHT			(600)
#define LOCATE_OUTPUT_MAXAREAS			(360000)
//////////////////////////////////////////////////////////////////////////

#define BARCODE_LOCATE_GRAD_THRESHOLD_L	(10)

float fLctGradHistRatio = AUTOCONTRAST_THRESHOLD_RATIO_L;
int nLctGradHistThre = 0;

int * pnLctGradHistSlice;
static short psGradCnt[64];
static short psGradAcc[64];

int nLctImgMaxWid = 0, nLctImgMaxHei = 0;

int nLctImgWid = 0, nLctImgHei = 0, nLctImgSize = 0;
int nLctOGWid = 0, nLctOGHei = 0, nLctOGSize = 0;
int nLctBlobsWid = 0, nLctBlobsHei = 0;
int nLctBlobOGSize = 0;

unsigned char * ucLctAtanLUT512 = 0;

unsigned char *	ucLctGradient = 0;
unsigned char *	ucLctOrientation = 0;

int * pnLctPrimaryMarks = 0;
FastLocateClus * flLctPrimaryClus = 0;

int nLctClusMaxSize = 0;

unsigned char * ucLctBlobMask = 0;		// 用来标识图像中的非零点(边缘)
RyuPoint * ptLctBlobSeq = 0;				// 记录所有非零点的坐标
LocateClusLine * clLctBlobClusLine = 0;
LocateClusArea * caLctBlobClusArea = 0;

int FNC_LBC02_FilterBorderBlob(int orin_thre, int thre1, int thre2);

int FNC_LBC03_GetRegionFeature(int * label_mat, int clus_cnt);

int FNC_LBC02_BlobCluster2Line();

int FNC_LBC03_LineCluster2Area(int line_cnt);

int mapBarcodeLocateGlobalPtrs(BarcodeGlobalPointers * globalPtr)
{
	nLctImgMaxWid = globalPtr->nBarcodeDetectInitImgWid;
	nLctImgMaxHei = globalPtr->nBarcodeDetectInitImgHei;

	nLctClusMaxSize = globalPtr->nLctClusMaxSize;

	nLctGradHistThre = globalPtr->nLctGradHistThre;
	pnLctGradHistSlice = globalPtr->pnLctGradHistSlice;
	if(!pnLctGradHistSlice) {
		return -1;
	}

	ucLctAtanLUT512 = globalPtr->ucLctAtanLUT512;
	if(!ucLctAtanLUT512) {
		return -1;
	}
	ucLctGradient = globalPtr->ucLctGradient;
	if(!ucLctGradient) {
		return -1;
	}
	ucLctOrientation = globalPtr->ucLctOrientation;
	if(!ucLctOrientation) {
		return -1;
	}

	pnLctPrimaryMarks = globalPtr->pnLctPrimaryMarks;
	if(!pnLctPrimaryMarks) {
		return -1;
	}

#ifndef USE_NEW_LOCATE_ALGORITHM
	flLctPrimaryClus = (FastLocateClus *) globalPtr->flLctPrimaryClus;
	if(!flLctPrimaryClus) {
		return -1;
	}

	status = mapClassicCCAGlobalPtrs(globalPtr);
	if(1 != status) {
		return -1;
	}
#else
	ucLctBlobMask = globalPtr->ucLctBlobMask;
	if(!ucLctBlobMask) {
		return -1;
	}
	ptLctBlobSeq = (RyuPoint *) globalPtr->ptLctBlobSeq;
	if(!ptLctBlobSeq) {
		return -1;
	}
	clLctBlobClusLine = (LocateClusLine *) globalPtr->clLctBlobClusLine;
	if(!clLctBlobClusLine) {
		return -1;
	}
	caLctBlobClusArea = (LocateClusArea *) globalPtr->caLctBlobClusArea;
	if(!caLctBlobClusArea) {
		return -1;
	}
#endif

	return 1;
}


FastLocateClus * getLocateBarCodePrimary()
{
	return flLctPrimaryClus;
}

LocateClusArea * getLocateFeatureAreas()
{
	return caLctBlobClusArea;
}

int BarcodeLocation_ipcMainProc0(unsigned char * img, int img_wid, int img_hei, int * bloc_size,
		barcode_detect_proc_info_t ** proc_infos)
{
	int ret_val = 0, i = 0;
	int sliceHei = 0, remainHei = 0, accHei = 0;

	if(img_wid > nLctImgMaxWid || img_hei > nLctImgMaxHei) {
		ret_val = -11202016;
		goto nExit;
	}

	nLctImgWid = img_wid;
	nLctImgHei = img_hei;
	nLctImgSize = nLctImgWid * nLctImgHei;

	nLctOGWid = nLctImgWid >> 2;
	nLctOGHei = nLctImgHei >> 2;
	nLctOGSize = nLctOGWid * nLctOGHei;

	nLctGradHistThre = (int)(nLctOGSize * (1 - fLctGradHistRatio));

	nLctBlobOGSize = (*bloc_size >> 2) + ((*bloc_size % 4) >> 1);
	*bloc_size = nLctBlobOGSize << 2;

	nLctBlobsWid = nLctOGWid / (nLctBlobOGSize >> 1) - 1;
	nLctBlobsHei = nLctOGHei / (nLctBlobOGSize >> 1) - 1;

	sliceHei = nLctOGHei / NTHREADS;
	remainHei = nLctOGHei % NTHREADS;
	for(i = 0; i < NTHREADS; i++) {
		proc_infos[i+1]->nVari1 = nLctOGWid;
		proc_infos[i+1]->nVari2 = (i < remainHei) ? (sliceHei + 1) : sliceHei;
		proc_infos[i+1]->nVari3 = nLctImgWid;
		proc_infos[i+1]->uData1 = img + nLctImgWid * (accHei<<2);
		proc_infos[i+1]->uData2 = ucLctGradient + nLctOGWid * accHei;
		proc_infos[i+1]->uData3 = ucLctOrientation + nLctOGWid * accHei;
		proc_infos[i+1]->nData1 = pnLctGradHistSlice;
		accHei += proc_infos[i+1]->nVari2;
	}

	ret_val = 1;

nExit:
	return ret_val;
}

int BarcodeLocation_ipcSlaveProc0(barcode_detect_proc_info_t * proc_info)
{
	int i = 0, j = 0, nstep = 0;

	int dx[4] = {0}, dy[4] = {0}, t[4] = {0};
	int val = 0, val1 = 0, val2 = 0, idx = 0, idx1 = 0, idx2 = 0;

	unsigned char * pOrient	= 0;
	unsigned char * pGrad	= 0;

	unsigned char * lOrient	= 0;
	unsigned char * lGrad	= 0;

	unsigned char * loffset_1, * loffset_2, * loffset_3, * loffset_4;
	unsigned char * poffset_1, * poffset_2, * poffset_3, * poffset_4;

	unsigned char * sliceImg = 0, * sliceGrad = 0, * sliceOrie = 0;
	int sliceW = 0, sliceH = 0, step = 0, thre = 0;
	int * sliceHist = 0;

	sliceImg  = proc_info->uData1;
	sliceGrad = proc_info->uData2;
	sliceOrie = proc_info->uData3;
	sliceHist = proc_info->nData1;
	sliceW	  = proc_info->nVari1;
	sliceH	  = proc_info->nVari2;
	step	  = proc_info->nVari3;
	nstep	  = step << 2;
	thre	  = BARCODE_LOCATE_GRAD_THRESHOLD_L;

	Cache_inv(sliceImg, (sliceW<<2) * (sliceH<<2) * sizeof(unsigned char), Cache_Type_ALL, FALSE);

	memset(sliceHist, 0, sizeof(int) * 256);

	loffset_1 = loffset_2 = loffset_3 = loffset_4 = 0;
	poffset_1 = poffset_2 = poffset_3 = poffset_4 = 0;

	lOrient	= sliceOrie;
	lGrad	= sliceGrad;

	loffset_1	= sliceImg;
	loffset_2	= loffset_1 + step;
	loffset_3	= loffset_2 + step;
	loffset_4	= loffset_3 + step;

	// 其实没有必要用倒序，正序并使用“数组名[循环变量]”的形式并配合cache效率更高
	for(i = sliceH; i > 0; i--) {
		poffset_1 = loffset_1;
		poffset_2 = loffset_2;
		poffset_3 = loffset_3;
		poffset_4 = loffset_4;

		pGrad = lGrad;
		pOrient = lOrient;

		for(j = sliceW; j > 0; j--) {
			dx[0] = poffset_2[2] - poffset_2[0];
			dy[0] = poffset_3[1] - poffset_1[1];
			dx[1] = poffset_2[3] - poffset_2[1];
			dy[1] = poffset_3[2] - poffset_1[2];
			dx[2] = poffset_3[2] - poffset_3[0];
			dy[2] = poffset_4[1] - poffset_2[1];
			dx[3] = poffset_3[3] - poffset_3[1];
			dy[3] = poffset_4[2] - poffset_2[2];

			t[0] = abs(dx[0]) > abs(dy[0]) ? abs(dx[0]) : abs(dy[0]);
			t[1] = abs(dx[1]) > abs(dy[1]) ? abs(dx[1]) : abs(dy[1]);
			t[2] = abs(dx[2]) > abs(dy[2]) ? abs(dx[2]) : abs(dy[2]);
			t[3] = abs(dx[3]) > abs(dy[3]) ? abs(dx[3]) : abs(dy[3]);

			val1 = (t[0] > t[1]) ? t[0] : t[1];
			idx1 = (t[0] > t[1]) ? 0 : 1;
			val2 = (t[2] > t[3]) ? t[2] : t[3];
			idx2 = (t[2] > t[3]) ? 2 : 3;

			idx = (val1 > val2) ? idx1 : idx2;
			val = (val1 > val2) ? val1 : val2;

			*pGrad = (val < thre) ? 0 : val;
			*pOrient = (*pGrad) ? ucLctAtanLUT512[(dx[idx]+256)|((dy[idx]+256)<<9)] : 0xff;	// 高频使用的小片内存，优化内存位置，可提升读取速度

			sliceHist[val]++;	// 高频使用的小片内存，优化内存位置，可提升读取速度

			poffset_1 += 4;
			poffset_2 += 4;
			poffset_3 += 4;
			poffset_4 += 4;
			pGrad++;
			pOrient++;
		}
		loffset_1 += nstep;
		loffset_2 += nstep;
		loffset_3 += nstep;
		loffset_4 += nstep;
		lOrient   += sliceW;
		lGrad 	  += sliceW;
	}

	// Ryu cache回写很重要!!
	Cache_wb(sliceHist, 256 * sizeof(int), Cache_Type_ALL, TRUE);
	Cache_wb(sliceGrad, sliceW * sliceH * sizeof(unsigned char), Cache_Type_ALL, TRUE);
	Cache_wb(sliceOrie, sliceW * sliceH * sizeof(unsigned char), Cache_Type_ALL, TRUE);

	return 1;
}

int BarcodeLocation_ipcMainProc1(barcode_detect_proc_info_t ** proc_infos)
{
	int ret_val = 0;
	int i = 0, j = 0;
	int nsum = 0, nthre = 0, grad_thre = 0;

	int * sliceHist[NTHREADS];

	int sliceBlobsHei = 0, remainHei = 0, accHei = 0;

	for(i = 0; i < NTHREADS; i++) {
		sliceHist[i] = proc_infos[i+1]->nData1;
		Cache_inv(sliceHist[i], 256 * sizeof(int), Cache_Type_ALL, FALSE);
	}

	for(i = 0; i < 256; i++) {
		for(j = 0; j < NTHREADS; j++) {
			nsum += sliceHist[j][i];
		}

		if(nsum >= nLctGradHistThre) {
			nthre = i - 1;
			break;
		}
		// 2016.9.4更改，对比VC平台，优化代码
//		if(nsum > nLctGradHistThre) {
//			nthre = i - 2;
//			break;
//		}
	}

	grad_thre = (nthre > 10) ? nthre : 10;
	grad_thre = (nthre < 36) ? nthre : 36;
	// 2016.9.4更改，对比VC平台，优化代码
//	grad_thre = (nthre > 10) ? nthre : 0;
//	grad_thre = (grad_thre < 54) ? grad_thre : 54;

	sliceBlobsHei = nLctBlobsHei / NTHREADS;
	remainHei = nLctBlobsHei % NTHREADS;
	for(i = 0; i < NTHREADS; i++) {
		proc_infos[i+1]->nVari1 = nLctBlobsWid;
		proc_infos[i+1]->nVari2 = (i < remainHei) ? (sliceBlobsHei + 1) : sliceBlobsHei;
		proc_infos[i+1]->uData1 = ucLctGradient + nLctOGWid * (nLctBlobOGSize >> 1) * accHei;
		proc_infos[i+1]->uData2 = ucLctOrientation + nLctOGWid * (nLctBlobOGSize >> 1) * accHei;
		proc_infos[i+1]->nData1 = pnLctPrimaryMarks + nLctBlobsWid * accHei;
		proc_infos[i+1]->nVari3 = nLctOGWid;
		proc_infos[i+1]->nVari4 = nLctBlobOGSize;
		proc_infos[i+1]->nVari5 = grad_thre;
		accHei += proc_infos[i+1]->nVari2;
	}

	ret_val = 1;

	return ret_val;
}

int BarcodeLocation_ipcSlaveProc1(barcode_detect_proc_info_t * proc_info)
{
	int ret_val = 0;
	int i = 0, j = 0, m = 0, n = 0;

	unsigned char * sliceGrad = proc_info->uData1;
	unsigned char * sliceOrie = proc_info->uData2;
	int nOGStep = proc_info->nVari3;
	int nBlkSize = proc_info->nVari4;
	int grad_thre = proc_info->nVari5;

	short * sliceGradCnt = psGradCnt;
	short * sliceGradAcc = psGradAcc;
//	short sliceGradCnt[64], sliceGradAcc[64];

	int * sliceStamps = proc_info->nData1;
	int sliceBlobsW = proc_info->nVari1;
	int sliceBlobsH = proc_info->nVari2;

	unsigned char * pGrad_H = sliceGrad, * pOrie_H = sliceOrie;
	unsigned char * pGrad, *pOrie, * pGrad_B, * pOrie_B, * pGrad_L, * pOrie_L;

	int offsetH = 0, offsetW = 0;
	int nGradCnt = 0, nGradAcc = 0, nGradAvg = 0;

	int * pBlockStamps = sliceStamps;
	int effectCnt = 0;

	short * pCnt1 = 0, * pCnt2 = 0, * pAcc1 = 0, * pAcc2 = 0;

	short gradVal = 0, gradSgn = 0;

	int index = 0, index_t = 0;
	int val_acc = 0, cnt_acc = 0, cnt_macc = 0, val_macc = 0;
	int val_rat = 0;
	int val = 0;

	const int aPI = 45;
	//const int Set0Cnt = sizeof(short) * aPI;
	const int acc_radius = 2;
	int acc_range = (acc_radius<<1) + 1;

	int nByteCnt = 0;

	offsetH = nOGStep * (nBlkSize >> 1);
	offsetW = nBlkSize >> 1;

	nByteCnt = nOGStep * (nBlkSize >> 1) * sliceBlobsH * sizeof(unsigned char);
	Cache_inv(sliceGrad, nByteCnt, Cache_Type_ALL, FALSE);
	Cache_inv(sliceOrie, nByteCnt, Cache_Type_ALL, FALSE);

	nByteCnt = sizeof(short) * aPI;

	for(m = sliceBlobsH; m > 0; m--) {
		pGrad_B = pGrad_H;
		pOrie_B = pOrie_H;
		for(n = sliceBlobsW; n > 0; n--) {
			pGrad_L = pGrad_B;
			pOrie_L = pOrie_B;
			nGradCnt = nGradAcc = nGradAvg = 0;
			memset(sliceGradCnt, 0, nByteCnt);
			memset(sliceGradAcc, 0, nByteCnt);
			for(i = nBlkSize; i > 0; i--) {
				pGrad = pGrad_L;
				pOrie = pOrie_L;
				for(j = nBlkSize; j > 0; j--) {
					// 20160923修改，尝试优化算法运行时间
					index = *pOrie >> 2;
					gradVal = (*pGrad < grad_thre) ? 0 : *pGrad;
					gradSgn = (gradVal && 1);
					sliceGradCnt[index] += gradSgn;	// ***
					sliceGradAcc[index] += gradVal;	// ***
					nGradCnt += gradSgn;
					nGradAcc += gradVal;

//					if(*pGrad >= grad_thre) {
//						index = *pOrie >> 2;
//						sliceGradCnt[index]++;	// ***
//						sliceGradAcc[index] += *pGrad;	// ***
//						nGradCnt++;
//						nGradAcc += *pGrad;
//					}

					pGrad++;
					pOrie++;
				}
				pGrad_L += nOGStep;
				pOrie_L += nOGStep;
			}
			pGrad_B += offsetW;
			pOrie_B += offsetW;

 			if(nGradCnt < nBlkSize || nGradAcc <= 0) {
 				*pBlockStamps = 0;
 				pBlockStamps++;
 				continue;
 			}

			// 后面这一部分耗时约10ms,可以通过降低角度分辨率提速
			val_acc = cnt_acc = 0;
			pAcc1 = sliceGradAcc;
			pAcc2 = sliceGradAcc + aPI;
			pCnt1 = sliceGradCnt;
			pCnt2 = sliceGradCnt + aPI;

			index = index_t = acc_radius;
			for(i = acc_range; i > 0; i--) {
				*(pAcc2++) = *pAcc1;
				*(pCnt2++) = *pCnt1;
				val_acc += *(pAcc1++);
				cnt_acc += *(pCnt1++);
			}
			// 中间值加成
			val_macc = val = val_acc + sliceGradAcc[index_t++];
			cnt_macc = cnt_acc;

			pAcc1 = sliceGradAcc;
			pCnt1 = sliceGradCnt;
			pAcc2 = sliceGradAcc + acc_range;
			pCnt2 = sliceGradCnt + acc_range;

			for(i = aPI; i > 0; i--) {
				val_acc += (*(pAcc2++) - *(pAcc1++));
				cnt_acc += (*(pCnt2++) - *(pCnt1++));
				val = val_acc + sliceGradAcc[index_t];
				cnt_macc = (val > val_macc) ? cnt_acc : cnt_macc;
				index = (val > val_macc) ? index_t : index;
				val_macc = (val > val_macc) ? val : val_macc;
				index_t++;
			}


			index = (index >= aPI) ? (index - aPI) : index;
			val_macc -= sliceGradAcc[index];

			index = (index << 2) + 2;

			cnt_acc = cnt_macc;
			val_acc = val_macc;

// 			// 考虑条码边界,对于垂直方向进行加和
// 			if(index - 9 < 90) {
// 				index_t = index + 90;
// 			} else {
// 				index_t = index - 90;
// 			}
// 			for(i = -9; i < 10; i++) {
// 				cnt_acc += sGradCnt[index_t+i];
// 				val_acc += sGradAcc[index_t+i];
// 			}

//			cnt_rat = cnt_acc * 100 / nGradCnt;
			val_rat = val_acc * 100 / nGradAcc;
			nGradAvg = nGradAcc / nGradCnt;
			nGradAvg = (nGradAvg < 0) ? 0 : nGradAvg;
			nGradAvg = (nGradAvg > 0xff) ? 0xff : nGradAvg;

			if(cnt_acc >= nBlkSize && val_rat > GRADHIST_THRE) {
#ifndef USE_NEW_LOCATE_ALGORITHM
				*pBlockStamps = (0xf<<24) | (cnt_acc<<16) | (val_rat<<8) | index;
#else
				*pBlockStamps = (0xf<<24) | (cnt_acc<<16) | (nGradAvg<<8) | index;
#endif
				effectCnt++;
			} else if(nGradCnt >= nBlkSize) {
				*pBlockStamps = (0x1<<24);
			} else {
				*pBlockStamps = 0;
			}

			pBlockStamps++;
		}
		pGrad_H += offsetH;
		pOrie_H += offsetH;
	}

	proc_info->nVari3 = effectCnt;

	// --Ryu-- ***Cache回写非常非常重要！！没有的话无法得到正确结果且速度很慢
	Cache_wb(sliceStamps, sizeof(int) * sliceBlobsW * sliceBlobsH, Cache_Type_ALL, TRUE);

	ret_val = 1;

	return ret_val;
}

int BarcodeLocation_ipcMainProc2(barcode_detect_proc_info_t ** proc_infos)
{
	int ret_val = 0;
	int i = 0;
	int effectCnt = 0;
	int line_cnt = 0, clus_cnt = 0;

	for(i = 0; i < NTHREADS; i++) {
		effectCnt += proc_infos[i+1]->nVari3;
	}

	if(0 == effectCnt) {
		ret_val = 0;
		goto nExit;
	}

#ifndef USE_NEW_LOCATE_ALGORITHM
	// --Ryu-- 【6678优化】此函数测时<1.5ms(约1.1ms),无需优化
	// 参数1为比重,2为个数
	FNC_LBC02_FilterBorderBlob(20, 20, 20);

	// --Ryu-- 【6678优化】此函数测时<2ms(约1.2ms),无需优化
	// 聚类
	clus_cnt = ClassicCCA_LabelImage(0, pnLctPrimaryMarks, nLctBlobsWid, nLctBlobsHei, nLctBlobsWid, 15, 18);
	label_mat = ClassicCCA_GetLabelMat();

	if(0 > clus_cnt) {
		ret_val = -11202011;
		goto nExit;
	} else if(0 == clus_cnt) {
		ret_val = 0;
		goto nExit;
	}

	// --Ryu-- 【6678优化】此函数测时<1ms(约0.7ms),无需优化
	// 聚类写入,融合
	status = FNC_LBC03_GetRegionFeature(label_mat, clus_cnt);

	if(0 < status)
		Cache_wb(flLctPrimaryClus, (status + 1) * sizeof(FastLocateClus), Cache_Type_ALL, TRUE);

	ret_val = status;
#else
	line_cnt = FNC_LBC02_BlobCluster2Line();
	if(0 == line_cnt) {
		ret_val = 0;
		goto nExit;
	} else if(0 > line_cnt) {
		ret_val = -10820013;
		goto nExit;
	}

	clus_cnt = FNC_LBC03_LineCluster2Area(line_cnt);
	if(0 == clus_cnt) {
		ret_val = 0;
		goto nExit;
	} else if(0 > clus_cnt) {
		ret_val = -10820014;
		goto nExit;
	} else {
		Cache_wb(caLctBlobClusArea, (clus_cnt + 1) * sizeof(LocateClusArea), Cache_Type_ALL, TRUE);
	}

	ret_val = clus_cnt;
#endif

nExit:
	return ret_val;
}

/*
// 精确角度,用到Hough变换
int updateLocateCodeClus(FastLocateClus * pClus)
{
	int nRet = 0;
	int i = 0, j = 0, k = 0;

	int wid = 0, hei = 0;
	int nTmp = 0;
	int index = 0, val = 0;
	int thre = 0, cnt = 0;

	int rMax = 0;
	int arr_size = 0;

	int ths[19] = {0};
	int iths[19] = {0};
	int cosThs[19] = {0};
	int sinThs[19] = {0};
	int midRsp[19] = {0};
	int nSum[19] = {0};

	short *	pOGLabelMat = 0, * pOG = 0, * pOG_L = 0;
	short *	pHoughArray = glcHoughArray, * pHA = 0, * pHA_L = 0;

	if(!pClus) {
		nRet = -10000009;
		goto nExit;
	}
	if(0 >= pClus->tgt_num) {
		nRet = 0;
		goto nExit;
	}

	wid = pClus->rect_loc[1] - pClus->rect_loc[0] + 1;
	hei = pClus->rect_loc[3] - pClus->rect_loc[2] + 1;
	if(1 >= wid || 1 >= hei) {
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	IplImage * update_ipl = cvCreateImage(cvSize(nLctOGWid, nLctOGHei), 8, 3);
	cvZero(update_ipl);
	for(int j_dbg = 0; j_dbg < nLctOGHei; j_dbg++) {
		short * pOG_dbg = glcOGLabelMat + j_dbg * nLctOGWid;
		unsigned char * pLbp_dbg = (unsigned char *)update_ipl->imageData + j_dbg * update_ipl->widthStep;
		for(i = 0; i < glcOGWid; i++) {
			if(*pOG_dbg > 0) {
				pLbp_dbg[0] = 255;
				pLbp_dbg[1] = 255;
				pLbp_dbg[2] = 255-*pOG_dbg;
			}
			pOG_dbg++;
			pLbp_dbg += 3;
		}
	}
	cvRectangle(update_ipl, cvPoint(pClus->rect_loc[0], pClus->rect_loc[2]), cvPoint(pClus->rect_loc[1], pClus->rect_loc[3]), 
		CV_RGB(255, 0, 0));
#endif
#endif
#endif

	pOGLabelMat = glcOGLabelMat + pClus->rect_loc[2] * nLctOGWid + pClus->rect_loc[0];
	rMax = wid + hei;
	arr_size = (rMax<<1) * 19;
	memset(pHoughArray, 0, sizeof(short) * arr_size);

	j = pClus->angle - 9;
	for(i = 0; i < 19; i++) {
		nTmp = (j < 0) ? (j + 180) : j;
		ths[i] = (nTmp >= 180) ? (nTmp - 180) : nTmp;
		iths[i] = ths[i] + 90;
		cosThs[i] = ryuCosShift(iths[i]);
		sinThs[i] = ryuSinShift(iths[i]);
		midRsp[i] = ((pClus->center_x/4-pClus->rect_loc[0]) * cosThs[i] 
			+ (pClus->center_y/4-pClus->rect_loc[2]) * sinThs[i])>>FLOAT2FIXED_SHIFT_DIGIT;
		j++;
	}

	if(pClus->angle < 45 || pClus->angle > 135) {
		pOG_L = pOGLabelMat;
		for(i = 0; i < wid; i++) {
			pOG = pOG_L;
			cnt = 0;
			for(j = 0; j < hei; j++) {
				if(pClus->clus_label == *pOG) {

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
					unsigned char * pIpl = (unsigned char *)update_ipl->imageData
						+ (pClus->rect_loc[2]+j) * update_ipl->widthStep + (pClus->rect_loc[0]+i) * 3;
					pIpl[0] = 0; pIpl[1] = 0; pIpl[2] = 255;
#endif
#endif
#endif
					pHA_L = pHoughArray + rMax;
					for(k = 0; k < 19; k++) {
						nTmp = (i * cosThs[k] + j * sinThs[k])>>FLOAT2FIXED_SHIFT_DIGIT;
						pHA = pHA_L + nTmp;
						//(*(pHA-1))++; (*pHA)++; (*(pHA+1))++;
						(*pHA)++;
						pHA_L += (rMax<<1);
					}
					cnt++;
					if(cnt >= 2)
						break;
				}
				pOG += nLctOGWid;
			}
			pOG_L++;
		}

		pOG_L = pOGLabelMat + (hei - 1) * nLctOGWid;
		for(i = 0; i < wid; i++) {
			pOG = pOG_L;
			cnt = 0;
			for(j = hei - 1; j >= 0; j--) {
				if(pClus->clus_label == *pOG) {
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
					unsigned char * pIpl = (unsigned char *)update_ipl->imageData
						+ (pClus->rect_loc[2]+j) * update_ipl->widthStep + (pClus->rect_loc[0]+i) * 3;
					pIpl[0] = 0; pIpl[1] = 0; pIpl[2] = 255;
#endif
#endif
#endif
					pHA_L = pHoughArray + rMax;
					for(k = 0; k < 19; k++) {
						nTmp = (i * cosThs[k] + j * sinThs[k])>>FLOAT2FIXED_SHIFT_DIGIT;
						pHA = pHA_L + nTmp;
						//(*(pHA-1))++; (*pHA)++; (*(pHA+1))++;
						(*pHA)++;
						pHA_L += (rMax<<1);
					}
					cnt++;
					if(cnt >= 2)
						break;
				}
				pOG -= glcOGWid;
			}
			pOG_L++;
		}
	} else {
		pOG_L = pOGLabelMat;
		for(i = 0; i < hei; i++) {
			pOG = pOG_L;
			cnt = 0;
			for(j = 0; j < wid; j++) {
				if(pClus->clus_label == *pOG) {
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
					unsigned char * pIpl = (unsigned char *)update_ipl->imageData
						+ (pClus->rect_loc[2]+i) * update_ipl->widthStep + (pClus->rect_loc[0]+j) * 3;
					pIpl[0] = 0; pIpl[1] = 0; pIpl[2] = 255;
#endif
#endif
#endif
					pHA_L = pHoughArray + rMax;
					for(k = 0; k < 19; k++) {
						nTmp = (j * cosThs[k] + i * sinThs[k])>>FLOAT2FIXED_SHIFT_DIGIT;
						pHA = pHA_L + nTmp;
						//(*(pHA-1))++; (*pHA)++; (*(pHA+1))++;
						(*pHA)++;
						pHA_L += (rMax<<1);
					}
					cnt++;
					if(cnt >= 2)
						break;
				}
				pOG++;
			}
			pOG_L += glcOGWid;
		}

		pOG_L = pOGLabelMat + wid - 1;
		for(i = 0; i < hei; i++) {
			pOG = pOG_L;
			cnt = 0;
			for(j = wid - 1; j >= 0; j--) {
				if(pClus->clus_label == *pOG) {
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
					unsigned char * pIpl = (unsigned char *)update_ipl->imageData
						+ (pClus->rect_loc[2]+i) * update_ipl->widthStep + (pClus->rect_loc[0]+j) * 3;
					pIpl[0] = 0; pIpl[1] = 0; pIpl[2] = 255;
#endif
#endif
#endif
					pHA_L = pHoughArray + rMax;
					for(k = 0; k < 19; k++) {
						nTmp = (j * cosThs[k] + i * sinThs[k])>>FLOAT2FIXED_SHIFT_DIGIT;
						pHA = pHA_L + nTmp;
						//(*(pHA-1))++; (*pHA)++; (*(pHA+1))++;
						(*pHA)++;
						pHA_L += (rMax<<1);
					}
					cnt++;
					if(cnt >= 2)
						break;
				}
				pOG--;
			}
			pOG_L += glcOGWid;
		}
	}

	pHA = pHoughArray;
	for(i = arr_size; i > 0; i--) {
		thre = (*pHA > thre) ? *pHA : thre;
		pHA++;
	}

	thre = ((thre>>1) > 5) ? (thre>>1) : 5;

	pHA_L = pHoughArray;
	val = 0;
	for(i = 0; i < 19; i++) {
		pHA = pHA_L;
		nTmp = 0;
		for(j = (rMax<<1); j > 0; j--) {
			nTmp += ((*pHA < thre) ? 0 : *pHA);
			pHA++;
		}
		nSum[i] = nTmp;
		index = (nSum[i] > val) ? i : index;
		val = (nSum[i] > val) ? nSum[i] : val;
		pHA_L += (rMax<<1);
	}

	if(val > thre) {
		nRet = ths[index];
	} else {
		nRet = -1;
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	printf("\correct = %d\n", nRet);
	cvNamedWindow("updateLocateCodeClus");
	cvShowImage("updateLocateCodeClus", update_ipl);
	cvWaitKey();
	cvReleaseImage(&update_ipl);
#endif
#endif
#endif

nExit:
	return nRet;
}*/

/*
// Ryu 多核并行优化
void FNC_LBC00_SlaveThreadProc0(unsigned char * sliceImg, int sliceW, int sliceH, int step, int thre,
		int * sliceHist, unsigned char * sliceGrad, unsigned char * sliceOrie)
{
	int i = 0, j = 0, nstep = 0;

	int dx[4] = {0}, dy[4] = {0}, t[4] = {0};
	int val = 0, val1 = 0, val2 = 0, idx = 0, idx1 = 0, idx2 = 0;
	//int nsum = 0;

	unsigned char * pOrient	= 0;
	unsigned char * pGrad	= 0;

	unsigned char * lOrient	= 0;
	unsigned char * lGrad	= 0;

	unsigned char * loffset_1, * loffset_2, * loffset_3, * loffset_4;
	unsigned char * poffset_1, * poffset_2, * poffset_3, * poffset_4;

	nstep = step << 2;

	Cache_inv(sliceImg, (sliceW<<2) * (sliceH<<2) * sizeof(unsigned char), Cache_Type_ALL, FALSE);

	memset(sliceHist, 0, sizeof(int) * 256);

	loffset_1 = loffset_2 = loffset_3 = loffset_4 = 0;
	poffset_1 = poffset_2 = poffset_3 = poffset_4 = 0;

	lOrient	= sliceOrie;
	lGrad	= sliceGrad;

	loffset_1	= sliceImg;
	loffset_2	= loffset_1 + step;
	loffset_3	= loffset_2 + step;
	loffset_4	= loffset_3 + step;

	// 其实没有必要用倒序，正序并使用“数组名[循环变量]”的形式并配合cache效率更高
	for(i = sliceH; i > 0; i--) {
		poffset_1 = loffset_1;
		poffset_2 = loffset_2;
		poffset_3 = loffset_3;
		poffset_4 = loffset_4;

		pGrad = lGrad;
		pOrient = lOrient;

		for(j = sliceW; j > 0; j--) {
			dx[0] = poffset_2[2] - poffset_2[0];
			dy[0] = poffset_3[1] - poffset_1[1];
			dx[1] = poffset_2[3] - poffset_2[1];
			dy[1] = poffset_3[2] - poffset_1[2];
			dx[2] = poffset_3[2] - poffset_3[0];
			dy[2] = poffset_4[1] - poffset_2[1];
			dx[3] = poffset_3[3] - poffset_3[1];
			dy[3] = poffset_4[2] - poffset_2[2];

			//nsum = (poffset_1[0] + poffset_1[1] + poffset_1[2] + poffset_1[3]
			//+  poffset_2[0] + poffset_2[1] + poffset_2[2] + poffset_2[3]
			//+  poffset_3[0] + poffset_3[1] + poffset_3[2] + poffset_3[3]
			//+  poffset_4[0] + poffset_4[1] + poffset_4[2] + poffset_4[3]) >> 6;

			// nthre = (grad_thre > nsum) ? grad_thre : nsum;

			t[0] = abs(dx[0]) > abs(dy[0]) ? abs(dx[0]) : abs(dy[0]);
			t[1] = abs(dx[1]) > abs(dy[1]) ? abs(dx[1]) : abs(dy[1]);
			t[2] = abs(dx[2]) > abs(dy[2]) ? abs(dx[2]) : abs(dy[2]);
			t[3] = abs(dx[3]) > abs(dy[3]) ? abs(dx[3]) : abs(dy[3]);

			val1 = (t[0] > t[1]) ? t[0] : t[1];
			idx1 = (t[0] > t[1]) ? 0 : 1;
			val2 = (t[2] > t[3]) ? t[2] : t[3];
			idx2 = (t[2] > t[3]) ? 2 : 3;

			idx = (val1 > val2) ? idx1 : idx2;
			val = (val1 > val2) ? val1 : val2;

			*pGrad = (val < thre) ? 0 : val;
			*pOrient = (*pGrad) ? glcAtanLUT512[(dx[idx]+256)|((dy[idx]+256)<<9)] : 0xff;

			sliceHist[val]++;

			poffset_1 += 4;
			poffset_2 += 4;
			poffset_3 += 4;
			poffset_4 += 4;
			pGrad++;
			pOrient++;
		}
		loffset_1 += nstep;
		loffset_2 += nstep;
		loffset_3 += nstep;
		loffset_4 += nstep;
		lOrient += glcOGWid;
		lGrad += glcOGWid;
	}

	// Ryu cache回写很重要!!
	Cache_wb(sliceHist, 256 * sizeof(int), Cache_Type_ALL, TRUE);
	Cache_wb(sliceGrad, sliceW * sliceH * sizeof(unsigned char), Cache_Type_ALL, TRUE);
	Cache_wb(sliceOrie, sliceW * sliceH * sizeof(unsigned char), Cache_Type_ALL, TRUE);

	return;
}

int FNC_LBC00_GetSampleGradOrient(unsigned char * const srcimg, int min_thre, int max_thre, float ratio, int * grad_thre)
{
	int nRet = 0;
	int i = 0, j = 0;
	int nsum = 0, nthre = 0;

	unsigned char * sliceImg[NTHREADS];
	unsigned char * sliceGrad[NTHREADS];
	unsigned char * sliceOrie[NTHREADS];
	int * sliceHist[NTHREADS];
	int sliceH[NTHREADS];
	int sliceW = 0, sliceHei = 0, remainHei = 0, accHei = 0;

	if(1 != gIsLBCInit) {
		nRet = -11202012;
		goto nExit;
	}

	if(!srcimg || !grad_thre) {
		nRet = -11202013;
		goto nExit;
	}

	if(glcGradHistRatio != ratio) {
		glcGradHistRatio = ratio;
		glcGradHistThre = (int)(glcOGWid * glcOGHei * (1 - glcGradHistRatio));
	}

	omp_set_num_threads(NTHREADS);

	sliceW = glcOGWid;
	sliceHei = glcOGHei / NTHREADS;
	remainHei = glcOGHei % NTHREADS;
	for(i = 0; i < NTHREADS; i++) {
		sliceH[i] = (i < remainHei) ? (sliceHei + 1) : sliceHei;
		sliceImg[i] = srcimg + glcImgWid * (accHei<<2);
		sliceGrad[i] = glcGradient + sliceW * accHei;
		sliceOrie[i] = glcOrientation + sliceW * accHei;
		sliceHist[i] = glcGradHistSlice[i];
		accHei += sliceH[i];
	}

#pragma omp parallel for shared(sliceImg, sliceW, sliceH, glcImgWid, min_thre, sliceHist, sliceGrad, sliceOrie, glcAtanLUT512) private(i)
	for(i = 0; i < NTHREADS; i++) {
		FNC_LBC00_SlaveThreadProc0(sliceImg[i], sliceW, sliceH[i], glcImgWid, min_thre, sliceHist[i],
				sliceGrad[i], sliceOrie[i]);
	}

	nsum = nthre = 0;
	for(i = 0; i < 256; i++) {
		for(j = 0; j < NTHREADS; j++) {
			nsum += sliceHist[j][i];
		}

		if(nsum >= glcGradHistThre) {
			nthre = i - 1;
			break;
		}
	}

#ifdef _DEBUG_FLAG
#ifdef _RUN_IN_6678
	platform_write("--Ryu-- 灰度级%d, 累积个数%d, 累积比例%f, 阈值%f\n",
			nthre, nsum, 1.0 * nsum / glcOGWid / glcOGHei, 1 - glcGradHistRatio);
#endif
#endif

	*grad_thre = (nthre > min_thre) ? nthre : min_thre;
	*grad_thre = (nthre < max_thre) ? nthre : max_thre;

	nRet = 1;
nExit:
	return nRet;
}

// Ryu 多核并行优化
void FNC_LBC01_SlaveThreadProc0(unsigned char * sliceGrad, unsigned char * sliceOrie, int nOGStep, int nBlkSize, int grad_thre,
		short * sliceGradCnt, short * sliceGradAcc, int * sliceStamps, int sliceBlobsW, int sliceBlobsH, int * sliceEffcnt)
{
	int i = 0, j = 0, m = 0, n = 0;

	unsigned char * pGrad_H = sliceGrad, * pOrie_H = sliceOrie;
	unsigned char * pGrad, *pOrie, * pGrad_B, * pOrie_B, * pGrad_L, * pOrie_L;

	int offsetH = 0, offsetW = 0;
	int nGradCnt = 0, nGradAcc = 0;

	int * pBlockStamps = sliceStamps;
	int effectCnt = 0;

	short * pCnt1 = 0, * pCnt2 = 0, * pAcc1 = 0, * pAcc2 = 0;

	short gradVal = 0, gradSgn = 0;

	int index = 0, index_t = 0;
	int val_acc = 0, cnt_acc = 0, cnt_macc = 0, val_macc = 0;
	int val_rat = 0;
	int val = 0;

	const int aPI = 45;
	//const int Set0Cnt = sizeof(short) * aPI;
	const int acc_radius = 2;
	int acc_range = (acc_radius<<1) + 1;

	int nByteCnt = 0;

	offsetH = nOGStep * (nBlkSize >> 1);
	offsetW = nBlkSize >> 1;

	nByteCnt = nOGStep * (nBlkSize >> 1) * sliceBlobsH * sizeof(unsigned char);
	Cache_inv(sliceGrad, nByteCnt, Cache_Type_ALL, FALSE);
	Cache_inv(sliceOrie, nByteCnt, Cache_Type_ALL, FALSE);

	nByteCnt = sizeof(short) * aPI;

	for(m = sliceBlobsH; m > 0; m--) {
		pGrad_B = pGrad_H;
		pOrie_B = pOrie_H;
		for(n = sliceBlobsW; n > 0; n--) {
			pGrad_L = pGrad_B;
			pOrie_L = pOrie_B;
			nGradCnt = 0;
			nGradAcc = 0;
			memset(sliceGradCnt, 0, nByteCnt);
			memset(sliceGradAcc, 0, nByteCnt);
			for(i = nBlkSize; i > 0; i--) {
				pGrad = pGrad_L;
				pOrie = pOrie_L;
				for(j = nBlkSize; j > 0; j--) {
					index = *pOrie >> 2;
					gradVal = (*pGrad < grad_thre) ? 0 : *pGrad;
					gradSgn = (gradVal && 1);
					sliceGradCnt[index] += gradSgn;	// ***
					sliceGradAcc[index] += gradVal;	// ***
					//////////////////////////////////////////////////////////////////////////
					nGradCnt += gradSgn;
					nGradAcc += gradVal;
					pGrad++;
					pOrie++;
				}
				pGrad_L += nOGStep;
				pOrie_L += nOGStep;
			}
			pGrad_B += offsetW;
			pOrie_B += offsetW;

// 			if(nGradCnt < threCnt) {
// 				*pBlockStamps = 0;
// 				pBlockStamps++;
// 				continue;
// 			}

			nGradAcc++;		// 防止除数为0

			// 后面这一部分耗时约10ms,可以通过降低角度分辨率提速
			val_acc = cnt_acc = 0;
			pAcc1 = sliceGradAcc;
			pAcc2 = sliceGradAcc + aPI;
			pCnt1 = sliceGradCnt;
			pCnt2 = sliceGradCnt + aPI;

			index = index_t = acc_radius;
			for(i = acc_range; i > 0; i--) {
				*(pAcc2++) = *pAcc1;
				*(pCnt2++) = *pCnt1;
				val_acc += *(pAcc1++);
				cnt_acc += *(pCnt1++);
			}
			// 中间值加成
			val_macc = val = val_acc + sliceGradAcc[index_t++];
			cnt_macc = cnt_acc;

			pAcc1 = sliceGradAcc;
			pCnt1 = sliceGradCnt;
			pAcc2 = sliceGradAcc + acc_range;
			pCnt2 = sliceGradCnt + acc_range;

			for(i = aPI; i > 0; i--) {
				val_acc += (*(pAcc2++) - *(pAcc1++));
				cnt_acc += (*(pCnt2++) - *(pCnt1++));
				val = val_acc + sliceGradAcc[index_t];
				cnt_macc = (val > val_macc) ? cnt_acc : cnt_macc;
				index = (val > val_macc) ? index_t : index;
				val_macc = (val > val_macc) ? val : val_macc;
				index_t++;
			}


			index = (index >= aPI) ? (index - aPI) : index;
			val_macc -= sliceGradAcc[index];

			index = (index << 2) + 2;

			cnt_acc = cnt_macc;
			val_acc = val_macc;

// 			// 考虑条码边界,对于垂直方向进行加和
// 			if(index - 9 < 90) {
// 				index_t = index + 90;
// 			} else {
// 				index_t = index - 90;
// 			}
// 			for(i = -9; i < 10; i++) {
// 				cnt_acc += sGradCnt[index_t+i];
// 				val_acc += sGradAcc[index_t+i];
// 			}

//			cnt_rat = cnt_acc * 100 / nGradCnt;
			val_rat = val_acc * 100 / nGradAcc;

			if(cnt_acc >= nBlkSize && val_rat > GRADHIST_THRE) {
				*pBlockStamps = (0xf<<24) | (cnt_acc<<16) | (val_rat<<8) | index;
				effectCnt++;
			} else if(nGradCnt > nBlkSize) {
				*pBlockStamps = (0x1<<24);
			} else {
				*pBlockStamps = 0;
			}

			pBlockStamps++;
		}
		pGrad_H += offsetH;
		pOrie_H += offsetH;
	}

	// --Ryu-- ***Cache回写非常非常重要！！没有的话无法得到正确结果且速度很慢
	Cache_wb(sliceStamps, sizeof(int) * sliceBlobsW * sliceBlobsH, Cache_Type_ALL, TRUE);

	*sliceEffcnt = effectCnt;
	return;
}

int FNC_LBC01_GetBlobFeature(int grad_thre)
{
	int i = 0, sliceBlobsHei = 0, effectCnt = 0;
	int remainHei = 0, accHei = 0;

	unsigned char * sliceGrad[NTHREADS], * sliceOrie[NTHREADS];
	int * sliceStamps[NTHREADS];
	int sliceEffcnt[NTHREADS], sliceBlobsW[NTHREADS], sliceBlobsH[NTHREADS];

	omp_set_num_threads(NTHREADS);

	sliceBlobsHei = glcBlobsHei / NTHREADS;
	remainHei = glcBlobsHei % NTHREADS;
	for(i = 0; i < NTHREADS; i++) {
		sliceBlobsW[i] 	= glcBlobsWid;
		sliceBlobsH[i] 	= (i < remainHei) ? (sliceBlobsHei + 1) : sliceBlobsHei;
		sliceGrad[i] 	= glcGradient + glcOGWid * (glcBlobOGSize >> 1) * accHei;
		sliceOrie[i] 	= glcOrientation + glcOGWid * (glcBlobOGSize >> 1) * accHei;
		sliceStamps[i] 	= glcPrimaryMarks + glcBlobsWid * accHei;
		sliceEffcnt[i]	= 0;
		accHei += sliceBlobsH[i];
	}

#pragma omp parallel for shared(sliceGrad, sliceOrie, glcOGWid, glcBlobOGSize, grad_thre, sGradCnt, sGradAcc, sliceStamps, sliceBlobsW, sliceBlobsH, sliceEffcnt) private(i)
	for(i = 0; i < NTHREADS; i++)
	{
		FNC_LBC01_SlaveThreadProc0(sliceGrad[i], sliceOrie[i], glcOGWid, glcBlobOGSize, grad_thre,
				sGradCnt[i], sGradAcc[i], sliceStamps[i], sliceBlobsW[i], sliceBlobsH[i], &sliceEffcnt[i]);
	}

	for(i = 0; i < NTHREADS; i++) {
		effectCnt += sliceEffcnt[i];
	}

	return effectCnt;
}

*/
/*
 * 注：给出的矩形角点顺序为
 * 0-------------1
 * |			 |
 * |			 |
 * |			 |
 * 2-------------3
 */
int FNC_LBC03_GetRegionFeature(int * label_mat, int clus_cnt)
{
	//int nRet = 0;

	int i = 0, j = 0, x = 0, y = 0;
	int * pLabel = 0;
	int effcCnt = 0;

	int index = 0, angle = 0;
	int nTmp1 = 0, nTmp2 = 0;

	FastLocateClus * pClus = 0;
	FastLocateClus temClus = flLctPrimaryClus[0];

	int * pBlockStamps = pnLctPrimaryMarks;
	int A = 0, B = 0, C = 0;
	int A_t = 0, B_t = 0, C_t = 0;

	//int min_intr1, min_intr2, max_intr1, max_intr2;
	//int min_intr_t1, min_intr_t2, max_intr_t1, max_intr_t2;

	//int nCX1 = 0, nCX2 = 0, nCY1 = 0, nCY2 = 0;
	//short * pOGLb = 0;

	float sqrtVal = 0.0;

	//min_intr1 = min_intr2 = max_intr1 = max_intr2 = 0;
	//min_intr_t1 = min_intr_t2 = max_intr_t1 = max_intr_t2 = 0;

	pLabel = label_mat;
	memset(flLctPrimaryClus, 0, sizeof(FastLocateClus) * (clus_cnt+1));

	// 写入聚类结构体数组
	for(j = 0; j < nLctBlobsHei; j++) {
		for(i = 0; i < nLctBlobsWid; i++) {
			if(*pLabel <= 0) {
				*pLabel = 0;
				pBlockStamps++;
				pLabel++;
				continue;
			}
			x = (i + 1) << 4;
			y = (j + 1) << 4;

			index = *pLabel;
			angle = *pBlockStamps & 0xff;
			pClus = &(flLctPrimaryClus[index]);
			pClus->clus_label = index;
			pClus->tgt_num++;
			pClus->center_x += x;
			pClus->center_y += y;

			if(1 == pClus->tgt_num) {
				pClus->LTRB[0] = i;
				pClus->LTRB[1] = j;
				pClus->LTRB[2] = i;
				pClus->LTRB[3] = j;
			} else {
				pClus->LTRB[0] = (pClus->LTRB[0] < i) ? pClus->LTRB[0] : i;
				pClus->LTRB[1] = (pClus->LTRB[1] < j) ? pClus->LTRB[1] : j;
				pClus->LTRB[2] = (pClus->LTRB[2] > i) ? pClus->LTRB[2] : i;
				pClus->LTRB[3] = (pClus->LTRB[3] > j) ? pClus->LTRB[3] : j;
			}

			pClus->intrcpt += ((angle < 90) ? angle : 0);
			pClus->intrcpt_t += ((angle < 90) ? 1 : 0);

			pClus->cen_intr += ((angle < 90) ? 0 : angle);
			pClus->cen_intr_t += ((angle < 90) ? 0 : 1);

			pBlockStamps++;
			pLabel++;
		}
	}

	// 计算角度、中心点参数
	pClus = flLctPrimaryClus + 1;
	for(i = 1; i <= clus_cnt; i++) {
		if(2 >= pClus->tgt_num) {
			memset(pClus, 0, sizeof(FastLocateClus));
			pClus++;
			continue;
		}
		if(pClus->intrcpt_t + pClus->cen_intr_t != pClus->tgt_num) {
			memset(pClus, 0, sizeof(FastLocateClus));
			pClus++;
			continue;
		}

		if(pClus->intrcpt_t > 0 && pClus->cen_intr_t > 0) {
			nTmp1 = pClus->intrcpt / pClus->intrcpt_t;
			nTmp2 = pClus->cen_intr / pClus->cen_intr_t;
			if(nTmp1 - nTmp2 > 90) {
				angle = (nTmp1 * pClus->intrcpt_t + (nTmp2+180) * pClus->cen_intr_t) / pClus->tgt_num;
				angle = (angle >= 180) ? (angle - 180) : angle;
			} else if(nTmp2 - nTmp1 > 90) {
				angle = ((nTmp1+180) * pClus->intrcpt_t + nTmp2 * pClus->cen_intr_t) / pClus->tgt_num;
				angle = (angle >= 180) ? (angle - 180) : angle;
			} else {
				angle = (nTmp1 * pClus->intrcpt_t + nTmp2 * pClus->cen_intr_t) / pClus->tgt_num;
			}
		} else {
			angle = (pClus->intrcpt + pClus->cen_intr) / pClus->tgt_num;
		}

		pClus->angle = angle;
		pClus->center_x /= pClus->tgt_num;
		pClus->center_y /= pClus->tgt_num;

		A = ryuCosShift(pClus->angle+90);
		B = ryuSinShift(pClus->angle+90);
		C = A * pClus->center_x + B * pClus->center_y;
		pClus->cen_intr = C >> 10;

		A = ryuCosShift(pClus->angle+180);
		B = ryuSinShift(pClus->angle+180);
		C_t = A * pClus->center_x + B * pClus->center_y;
		pClus->cen_intr_t = C_t >> 10;

		pClus->intrcpt = pClus->intrcpt_t = 0;

		effcCnt++;
		pClus++;
	}

	// 计算外围矩形参数
	pLabel = label_mat;
	pBlockStamps = pnLctPrimaryMarks;
	for(j = 0; j < nLctBlobsHei; j++) {
		for(i = 0; i < nLctBlobsWid; i++) {
			if(0 >= *pLabel) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			pClus = &(flLctPrimaryClus[*pLabel]);
			if(0 >= pClus->tgt_num) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			x = (i + 1) << 4;
			y = (j + 1) << 4;

			A = ryuCosShift(pClus->angle+90);
			B = ryuSinShift(pClus->angle+90);
			C = (A * x + B * y) >> 10;
			C = abs(C - pClus->cen_intr);

			A = ryuCosShift(pClus->angle+180);
			B = ryuSinShift(pClus->angle+180);
			C_t = (A * x + B * y) >> 10;
			C_t = abs(C_t - pClus->cen_intr_t);

			pClus->intrcpt = (pClus->intrcpt > C) ? pClus->intrcpt : C;
			pClus->intrcpt_t = (pClus->intrcpt_t > C_t) ? pClus->intrcpt_t : C_t;

			pLabel++;
			pBlockStamps++;
		}
	}

	/*
	// 角度匹配、位置相近且形状拟合者，进行合并操作
	for(i = 1; i < clus_cnt; i++) {
		if(0 >= glcPrimaryClus[i].tgt_num)
			continue;
		for(j = i + 1; j < clus_cnt; j++) {
			if(0 >= flLctPrimaryClus[j].tgt_num)
				continue;
			// 角度匹配
			angle = abs(flLctPrimaryClus[i].angle - flLctPrimaryClus[j].angle);
			if(angle < 172 && angle > 8) 
				continue;
			// 计算融合角度
			index = flLctPrimaryClus[i].angle*flLctPrimaryClus[i].tgt_num + flLctPrimaryClus[j].angle*flLctPrimaryClus[j].tgt_num;
			nTmp1 = (flLctPrimaryClus[i].angle < flLctPrimaryClus[j].angle) ? glcPrimaryClus[i].tgt_num : glcPrimaryClus[j].tgt_num;
			nTmp2 = (angle > 90) ? 1 : 0;
			index += 180 * nTmp1 * nTmp2;
			index /= (glcPrimaryClus[i].tgt_num + flLctPrimaryClus[j].tgt_num);
			index = (index < 180) ? index : index - 180;

			// 使用融合角度重新计算各自的中心点参数和截距范围
			A = ryuCosShift(index+90);
			B = ryuSinShift(index+90);
			C = A * flLctPrimaryClus[i].center_x + B * flLctPrimaryClus[i].center_y;
			C = C >> 10;
			nTmp1 = C;
			min_intr1 = C - flLctPrimaryClus[i].intrcpt;
			max_intr1 = C + flLctPrimaryClus[i].intrcpt;
			C = A * flLctPrimaryClus[j].center_x + B * flLctPrimaryClus[j].center_y;
			C = C >> 10;
			nTmp1 = abs(C - nTmp1);		// 中心点截距差(条码垂直方向)
			min_intr2 = C - flLctPrimaryClus[j].intrcpt;
			max_intr2 = C + flLctPrimaryClus[j].intrcpt;

			A_t = ryuCosShift(index+180);
			B_t = ryuSinShift(index+180);
			C_t = A_t * flLctPrimaryClus[i].center_x + B_t * flLctPrimaryClus[i].center_y;
			C_t = C_t >> 10;
			nTmp2 = C_t;
			min_intr_t1 = C_t - flLctPrimaryClus[i].intrcpt_t;
			max_intr_t1 = C_t + flLctPrimaryClus[i].intrcpt_t;
			C_t = A_t * flLctPrimaryClus[j].center_x + B_t * flLctPrimaryClus[j].center_y;
			C_t = C_t >> 10;
			nTmp2 = abs(C_t - nTmp2);		// 中心点截距差(条码方向)
			min_intr_t2 = C_t - flLctPrimaryClus[j].intrcpt_t;
			max_intr_t2 = C_t + flLctPrimaryClus[j].intrcpt_t;

			// 条码垂直方向中心点截距差-应小于-条码方向中心点截距差
			if(nTmp1 > nTmp2)
				continue;

			// 垂直方向位置相近
			if(min_intr1 > max_intr2) {
				nTmp1 = min_intr1 - max_intr2;
			} else if(max_intr1 < min_intr2) {
				nTmp1 = min_intr2 - max_intr1;
			} else {
				nTmp1 = 0;
			}
			if(nTmp1 > 32)
				continue;

			// 水平方向位置相近
			if(min_intr_t1 > max_intr_t2) {
				nTmp2 = min_intr_t1 - max_intr_t2;
			} else if(max_intr_t1 < min_intr_t2) {
				nTmp2 = min_intr_t2 - max_intr_t1;
			} else {
				nTmp2 = 0;
			}
			if(nTmp2 > 64)
				continue;

			// 改变标记号
			pLabel_L = label_mat + flLctPrimaryClus[i].LTRB[1] * blocs_wid + flLctPrimaryClus[i].LTRB[0];
			for(y = flLctPrimaryClus[i].LTRB[3] - flLctPrimaryClus[i].LTRB[1] + 1; y > 0; y--) {
				pLabel = pLabel_L;
				for(x = flLctPrimaryClus[i].LTRB[2] - flLctPrimaryClus[i].LTRB[0] + 1; x > 0; x--) {
					*pLabel = (*pLabel == flLctPrimaryClus[i].clus_label) ? flLctPrimaryClus[j].clus_label : *pLabel;
					pLabel++;
				}
				pLabel_L += blocs_wid;
			}

			min_intr1 = (min_intr1 < min_intr2) ? min_intr1 : min_intr2;
			max_intr1 = (max_intr1 > max_intr2) ? max_intr1 : max_intr2;
			min_intr_t1 = (min_intr_t1 < min_intr_t2) ? min_intr_t1 : min_intr_t2;
			max_intr_t1 = (max_intr_t1 > max_intr_t2) ? max_intr_t1 : max_intr_t2;

			flLctPrimaryClus[j].intrcpt = (max_intr1 - min_intr1) >> 1;
			flLctPrimaryClus[j].intrcpt_t = (max_intr_t1 - min_intr_t1) >> 1;
			flLctPrimaryClus[j].cen_intr = (max_intr1 + min_intr1) >> 1;
			flLctPrimaryClus[j].cen_intr_t = (max_intr_t1 + min_intr_t1) >> 1;

			flLctPrimaryClus[j].LTRB[0] = (flLctPrimaryClus[j].LTRB[0] < flLctPrimaryClus[i].LTRB[0])
				? flLctPrimaryClus[j].LTRB[0] : flLctPrimaryClus[i].LTRB[0];
			flLctPrimaryClus[j].LTRB[1] = (flLctPrimaryClus[j].LTRB[1] < flLctPrimaryClus[i].LTRB[1])
				? flLctPrimaryClus[j].LTRB[1] : flLctPrimaryClus[i].LTRB[1];
			flLctPrimaryClus[j].LTRB[2] = (flLctPrimaryClus[j].LTRB[2] > flLctPrimaryClus[i].LTRB[2])
				? flLctPrimaryClus[j].LTRB[2] : flLctPrimaryClus[i].LTRB[2];
			flLctPrimaryClus[j].LTRB[3] = (flLctPrimaryClus[j].LTRB[3] > flLctPrimaryClus[i].LTRB[3])
				? flLctPrimaryClus[j].LTRB[3] : flLctPrimaryClus[i].LTRB[3];

			flLctPrimaryClus[j].center_x = (flLctPrimaryClus[j].center_x * flLctPrimaryClus[j].tgt_num
				+ flLctPrimaryClus[i].center_x * flLctPrimaryClus[i].tgt_num)
				/ (flLctPrimaryClus[j].tgt_num + flLctPrimaryClus[i].tgt_num);
			flLctPrimaryClus[j].center_y = (flLctPrimaryClus[j].center_y * flLctPrimaryClus[j].tgt_num
				+ flLctPrimaryClus[i].center_y * flLctPrimaryClus[i].tgt_num)
				/ (flLctPrimaryClus[j].tgt_num + flLctPrimaryClus[i].tgt_num);

			glcPrimaryClus[j].angle = index;
			glcPrimaryClus[j].tgt_num += glcPrimaryClus[i].tgt_num;
			glcPrimaryClus[j].fus_num += glcPrimaryClus[i].fus_num;

			memset(&glcPrimaryClus[i], 0, sizeof(FastLocateClus));
			effcCnt--;
			break;
		}
	}	*/

	// 求取拟合矩形角度,修正旋转角度
	pLabel = label_mat;
	pBlockStamps = pnLctPrimaryMarks;
	for(j = 0; j < nLctBlobsHei; j++) {
		for(i = 0; i < nLctBlobsWid; i++) {
			if(0 >= *pLabel) {
				pLabel++;
				pBlockStamps++;
				continue;
			}
			pClus = &(flLctPrimaryClus[*pLabel]);
			if(0 >= pClus->tgt_num) {
				pLabel++;
				pBlockStamps++;
				continue;
			}

			x = (i + 1) << 4;
			y = (j + 1) << 4;
			pClus->Ixx += (x - pClus->center_x) * (x - pClus->center_x);
			pClus->Iyy += (y - pClus->center_y) * (y - pClus->center_y);
			pClus->Ixy += (x - pClus->center_x) * (y - pClus->center_y);

			pLabel++;
			pBlockStamps++;
		}
	}

	// 计算拟合矩形角度,去除形状和角度不符者,拓展边界,获取四点坐标
	pClus = flLctPrimaryClus + 1;
	for(i = 1; i <= clus_cnt; i++) {
		if(0 >= pClus->tgt_num) {
			pClus++;
			continue;
		}

		// 去除形状不符者
		if(pClus->intrcpt > pClus->intrcpt_t) {
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		}

		// 去除尺寸不足者
		if(pClus->intrcpt_t < 32) {
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		}

		// 计算拟合矩形角度
		pClus->Ixx = pClus->Ixx / pClus->tgt_num;
		pClus->Iyy = pClus->Iyy / pClus->tgt_num;
		pClus->Ixy = pClus->Ixy / pClus->tgt_num;

		sqrtVal = sqrt(1.0 * (pClus->Ixx - pClus->Iyy) * (pClus->Ixx - pClus->Iyy) 
			+ 4.0 * pClus->Ixy * pClus->Ixy);
		nTmp1 = (pClus->Ixx + pClus->Iyy + (int)sqrtVal) >> 1;
		nTmp2 = (pClus->Ixx + pClus->Iyy - (int)sqrtVal) >> 1;

		nTmp2 = (abs(nTmp1) < abs(nTmp2)) ? nTmp1 : nTmp2;

		angle = (abs(pClus->Ixx) > abs(pClus->Iyy)) ? 
			ryuArctan180Shift(nTmp2-pClus->Ixx, pClus->Ixy) : ryuArctan180Shift(pClus->Ixy, nTmp2-pClus->Iyy);
		angle = (angle < 90) ? (angle + 90) : (angle - 90);
		pClus->Ixx = pClus->angle;
		pClus->Iyy = angle;

		index = abs(angle - pClus->angle);
		if(index > 18 && index < 162) {		// 去除两个矩形角度相差过多者
			memset(pClus, 0, sizeof(FastLocateClus));
			effcCnt--;
			pClus++;
			continue;
		} else if(index >= 162) {		// 角度修正
// 			if(angle > pClus->angle) {
// 				pClus->angle = (angle * 3 + pClus->angle + 180) >> 2;
// 			} else {
// 				pClus->angle = ((angle + 180) * 3 + pClus->angle) >> 2;
// 			}
			pClus->angle = (angle + pClus->angle + 180) >> 1;
			pClus->angle = (pClus->angle < 180) ? pClus->angle : (pClus->angle - 180);
		} else {
// 			pClus->angle = (angle * 3 + pClus->angle) >> 2;
			pClus->angle = (angle + pClus->angle) >> 1;
		}

		// 适当拓宽边界
		pClus->intrcpt += 32;
		pClus->intrcpt_t += 48;

		// 根据角度扩宽边界,以补足旋转造成的缺损
		nTmp1 = abs(ryuSinShift(pClus->angle));
		nTmp2 = abs(ryuCosShift(pClus->angle));
		nTmp1 = (nTmp1 > nTmp2) ? nTmp1 : nTmp2;
		pClus->intrcpt = (pClus->intrcpt<<10) / nTmp1;
		pClus->intrcpt_t = (pClus->intrcpt_t<<10) / nTmp1;

		A = ryuCosShift(pClus->angle+90);
		B = ryuSinShift(pClus->angle+90);
		A_t = ryuCosShift(pClus->angle+180);
		B_t = ryuSinShift(pClus->angle+180);

		// 修正中心点参数
		pClus->cen_intr = (A * pClus->center_x + B * pClus->center_y) >> 10;
		pClus->cen_intr_t = (A_t * pClus->center_x + B_t * pClus->center_y) >> 10;

		// 获取四点坐标
		pClus->rect_ptX[0] = ((pClus->cen_intr-pClus->intrcpt) * B_t - (pClus->cen_intr_t+pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[0] = ((pClus->cen_intr_t+pClus->intrcpt_t) * A - (pClus->cen_intr-pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[1] = ((pClus->cen_intr+pClus->intrcpt) * B_t - (pClus->cen_intr_t+pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[1] = ((pClus->cen_intr_t+pClus->intrcpt_t) * A - (pClus->cen_intr+pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[2] = ((pClus->cen_intr-pClus->intrcpt) * B_t - (pClus->cen_intr_t-pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[2] = ((pClus->cen_intr_t-pClus->intrcpt_t) * A - (pClus->cen_intr-pClus->intrcpt) * A_t)>>10;

		pClus->rect_ptX[3] = ((pClus->cen_intr+pClus->intrcpt) * B_t - (pClus->cen_intr_t-pClus->intrcpt_t) * B)>>10;
		pClus->rect_ptY[3] = ((pClus->cen_intr_t-pClus->intrcpt_t) * A - (pClus->cen_intr+pClus->intrcpt) * A_t)>>10;

		pClus++;
	}
	
	// 排序
	pClus = flLctPrimaryClus;
	for(i = clus_cnt; i > 0; i--) {
		for(j = i - 1; j > 0; j--) {
			if(pClus[j].tgt_num < pClus[i].tgt_num) {
				temClus = pClus[j];
				pClus[j] = pClus[i];
				pClus[i] = temClus;
			}
		}
	}
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	IplImage * show_img3 = cvCreateImage(cvGetSize(show_img2), 8, 3);
	IplImage * show_img4 = cvCreateImage(cvGetSize(show_img2), 8, 3);
	pClus = flLctPrimaryClus;

	cvCopy(show_img2, show_img4);

	for(i = 1; i <= effcCnt; i++) {
		if(pClus[i].tgt_num < 0)
			break;
		cvCopy(show_img2, show_img3);

		for(int j = 0; j < 4; j++) {
			sprintf(txt, "%d", j);
			cvPutText(show_img3, txt, cvPoint(pClus[i].rect_ptX[j]/4+2, pClus[i].rect_ptY[j]/4), &font, CV_RGB(255,255,0));
			cvCircle(show_img3, cvPoint(pClus[i].rect_ptX[j]/4, pClus[i].rect_ptY[j]/4), 2, CV_RGB(255, 255, 0), CV_FILLED);

			sprintf(txt, "%d", i);
			cvPutText(show_img4, txt, cvPoint(pClus[i].rect_ptX[j]/4+2, pClus[i].rect_ptY[j]/4), &font, CV_RGB(255,255,0));
			cvCircle(show_img4, cvPoint(pClus[i].rect_ptX[j]/4, pClus[i].rect_ptY[j]/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		}

		pLabel = label_mat;
		for(int i_dbg = 0; i_dbg < nLctBlobsHei; i_dbg++) {
			for(int j_dbg = 0; j_dbg < nLctBlobsWid; j_dbg++) {
				if(*pLabel == pClus[i].clus_label) {
					cvRectangle(show_img3, cvPoint(j_dbg*4, i_dbg*4), 
						cvPoint((j_dbg+1)*4-1, (i_dbg+1)*4-1), CV_RGB(0, 255, 0));
					cvRectangle(show_img4, cvPoint(j_dbg*4, i_dbg*4), 
						cvPoint((j_dbg+1)*4-1, (i_dbg+1)*4-1), CV_RGB(0, 255, 0));

				}
				pLabel++;
			}
		}
		printf("\nLabel %d\n", pClus[i].clus_label);
		printf("tgt_num= %d, fus_num= %d\n", pClus[i].tgt_num, pClus[i].fus_num);
		printf("center= (%d, %d), angle= %d\n", pClus[i].center_x, pClus[i].center_y, pClus[i].angle);
		printf("grad_angle = %d, rect_angle = %d\n", pClus[i].Ixx, pClus[i].Iyy);
		printf("cen_intr= %d, cen_intr_t= %d\n", pClus[i].cen_intr, pClus[i].cen_intr_t);
		printf("intrcpt= %d, intrcpt_t= %d\n", pClus[i].intrcpt, pClus[i].intrcpt_t);


		sprintf(txt, "%d", i);
		cvCircle(show_img3, cvPoint(pClus[i].center_x/4, pClus[i].center_y/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		cvCircle(show_img4, cvPoint(pClus[i].center_x/4, pClus[i].center_y/4), 2, CV_RGB(255, 255, 0), CV_FILLED);
		cvPutText(show_img3, txt, cvPoint(pClus[i].center_x/4+4, pClus[i].center_y/4), &font, CV_RGB(0,0,255));
		cvNamedWindow("聚类分步结果");
		cvShowImage("聚类分步结果", show_img3);
		
		cvWaitKey();		
	}
	cvDestroyWindow("聚类分步结果");
	cvNamedWindow("聚类结果");
	cvShowImage("聚类结果", show_img4);
	cvWaitKey();
	cvReleaseImage(&show_img3);
	cvReleaseImage(&show_img4);
#endif
#endif
#endif

	return effcCnt;
}

int FNC_LBC02_FilterBorderBlob(int orin_thre, int thre1, int thre2)
{
	//int nRet = 0;
	int i = 0, j = 0, k = 0;

	int offset[8][8] = {0};
	int w1 = nLctBlobsWid, w2 = nLctBlobsWid<<1;

	int * pBlockStamps = 0;
	int orin1 = 0, orin2 = 0, diff1 = 0, diff2 = 0;
	int index = 0, tBlkStm = 0;
	int status = 0;

	offset[0][0] = 1;		offset[0][1] = 2;		offset[0][2] = w1+1;	offset[0][3] = w1+2;
	offset[0][4] = -1;		offset[0][5] = -2;		offset[0][6] = -w1-1;	offset[0][7] = -w1-2;
	offset[1][0] = w1+1;	offset[1][1] = w1+2;	offset[1][2] = w2+1;	offset[1][3] = w2+2;
	offset[1][4] = -w1-1;	offset[1][5] = -w1-2;	offset[1][6] = -w2-1;	offset[1][7] = -w2-2;
	offset[2][0] = w1+1;	offset[2][1] = w1+2;	offset[2][2] = w2+1;	offset[2][3] = w2+2;
	offset[2][4] = -w1-1;	offset[2][5] = -w1-2;	offset[2][6] = -w2-1;	offset[2][7] = -w2-2;
	offset[3][0] = w1;		offset[3][1] = w1+1;	offset[3][2] = w2;		offset[3][3] = w2+1;
	offset[3][4] = -w1;		offset[3][5] = -w1-1;	offset[3][6] = -w2;		offset[3][7] = -w2-1;
	offset[4][0] = w1;		offset[4][1] = w1-1;	offset[4][2] = w2;		offset[4][3] = w2-1;
	offset[4][4] = -w1;		offset[4][5] = -w1+1;	offset[4][6] = -w2;		offset[4][7] = -w2+1;
	offset[5][0] = w1-1;	offset[5][1] = w1-2;	offset[5][2] = w2-1;	offset[5][3] = w2-2;
	offset[5][4] = -w1+1;	offset[5][5] = -w1+2;	offset[5][6] = -w2+1;	offset[5][7] = -w2+2;
	offset[6][0] = w1-1;	offset[6][1] = w1-2;	offset[6][2] = w2-1;	offset[6][3] = w2-2;
	offset[6][4] = -w1+1;	offset[6][5] = -w1+2;	offset[6][6] = -w2+1;	offset[6][7] = -w2+2;
	offset[7][0] = -1;		offset[7][1] = -2;		offset[7][2] = w1-1;	offset[7][3] = w1-2;
	offset[7][4] = 1;		offset[7][5] = 2;		offset[7][6] = -w1+1;	offset[7][7] = -w1+2;

	// 首次过滤处于条码边缘的点,基于条码垂直方向
	pBlockStamps = pnLctPrimaryMarks + (nLctBlobsWid<<1) + 2;
	for(j = nLctBlobsHei - 4; j > 0; j--) {
		for(i = nLctBlobsWid - 4; i > 0; i--) {
			if(*pBlockStamps>>27) {
				orin1 = (*pBlockStamps&0xff);		// 取出角度
				index = (orin1 >= 90) ? (orin1 - 90) : (orin1 + 90);	// 取垂直方向mask
				index = (index<<1) / 45;
				for(k = 7; k >= 0; k--) {
					tBlkStm = *(pBlockStamps+offset[index][k]);
					orin2 = (tBlkStm&0xff);
					diff1 = ((tBlkStm>>8)&0xff) - ((*pBlockStamps>>8)&0xff);
					diff2 = ((tBlkStm>>16)&0xff) - ((*pBlockStamps>>16)&0xff);
					if((diff1 > thre1 || diff2 > thre2) 
						&& (abs(orin2-orin1) <= orin_thre || abs(orin2-orin1) >= 180-orin_thre)) {
							*pBlockStamps &= 0x07ffffff;
							break;
					}
				}
			}
			pBlockStamps++;
		}
		pBlockStamps += 4;
	}

	// 二次过滤在条码方向上无连接的点
	pBlockStamps = pnLctPrimaryMarks + (nLctBlobsWid<<1) + 2;
	for(j = nLctBlobsHei - 4; j > 0; j--) {
		for(i = nLctBlobsWid - 4; i > 0; i--) {
			if(*pBlockStamps>>27) {
				orin1 = (*pBlockStamps&0xff);		// 取出角度
				index = (orin1<<1) / 45;
				status = 0;
				for(k = 7; k >= 0; k--) {
					tBlkStm = *(pBlockStamps+offset[index][k]);
					orin2 = (tBlkStm&0xff);
					if((abs(orin2-orin1) <= orin_thre || abs(orin2-orin1) >= 180-orin_thre)
						&& (tBlkStm>>26)) {
							status = 1;
							break;
					}
				}
				*pBlockStamps &= (status) ? 0xffffffff : 0x07ffffff;
			}
			pBlockStamps++;
		}
		pBlockStamps += 4;
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	
	pBlockStamps = pnLctPrimaryMarks;
	for(int m = 0; m < nLctBlobsHei; m++) {
		for(int n = 0; n < nLctBlobsWid; n++) {
			if(7 == *pBlockStamps>>24) {
				/*
				cvRectangle(iplShendu1, cvPoint(n*4, m*4), 
					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 0, 0));
				cvRectangle(iplShendu2, cvPoint(n*4, m*4), 
					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 0, 0));
				*/
				cvRectangle(show_img2, cvPoint(n*4, m*4), 
					cvPoint((n+1)*4-1, (m+1)*4-1), CV_RGB(255, 128, 0));
			}
			pBlockStamps++;
		}
	}
	/*
	cvNamedWindow("iplShendu1");
	cvShowImage("iplShendu1", iplShendu1);
	cvNamedWindow("iplShendu2");
	cvShowImage("iplShendu2", iplShendu2);
	cvWaitKey();
	*/
#endif
#endif
#endif

	return 0;
}

int FNC_LBC02_BlobCluster2Line()
{
	int nRet = 0;
	int i = 0, j = 0, k = 0, i1 = 0, j1 = 0, base = 0;
	int count = 0, idx = 0;

	int a = 0, b = 0, trigflag = 0, angle = 0, angle_f = 0, angleflag = 0;
	int grad = 0, grad_f = 0, density = 0, density_f = 0;
	int ele_ang = 0, ele_cnt = 0, length = 0;
	int gap = 0, x = 0, y = 0, dx = 0, dy = 0;
	int x0 = 0, y0 = 0, dx0 = 0, dy0 = 0, xflag = 0;

	int line_end[2][2] = {{0,0}, {0,0}};		// 表示直线两端点的坐标
	int good_line = 0, line_cnt = 1;

	int * pBlockStamps = pnLctPrimaryMarks, * pStamp = 0;
	unsigned char * pBlobMask = ucLctBlobMask, * pMask = 0;
	RyuPoint * pBlobSeq = ptLctBlobSeq;
	LocateClusLine * pBlobClusLine = clLctBlobClusLine;

	LocateClusLine tmpClusLine;

	const int shift = 16;
	const int lineGap = 1;
	const int lineLength = 4;
	const int angdiff_thre = 18;

	Cache_inv(pBlockStamps, nLctBlobsWid * nLctBlobsHei * sizeof(int), Cache_Type_ALL, FALSE);

	// stage 1. 收集所有的非零点，将它们存入数组中，并在mask中标记为1
	for(i = 0; i < nLctBlobsHei; i++) {
		pStamp = pBlockStamps + i * nLctBlobsWid;
		pMask = pBlobMask + i * nLctBlobsWid;
		for(j = 0; j < nLctBlobsWid; j++) {
			if(*pStamp>>27) {
				*pMask = 0xff;
				pBlobSeq->x = j;
				pBlobSeq->y = i;
				count++;
				pBlobSeq++;
			} else {
				*pMask = 0;
			}
			pStamp++;
			pMask++;
		}
	}

	// stage 2. 以随机顺序处理所有的非零点
	pBlobSeq = ptLctBlobSeq;
	for( ; count > 0; count--) {
		// 选择随机点
		idx = rand();
		idx = idx % count;

		ele_cnt = ele_ang = angleflag = length = 0;

		j = line_end[0][0] = line_end[1][0] = pBlobSeq[idx].x;
		i = line_end[0][1] = line_end[1][1] = pBlobSeq[idx].y;

		// 通过重新赋值删除此点
		pBlobSeq[idx].x = pBlobSeq[count-1].x;
		pBlobSeq[idx].y = pBlobSeq[count-1].y;

		// 检查此点是否已经被排除 (例如属于别的直线)
		base = i * nLctBlobsWid + j;
		if(!(pBlobMask[base] & 0xf0))
			continue;

		// 如果超过阈值，就从当前点沿各个方向寻找并提取线段
		density = (pBlockStamps[base]>>16) & 0xff;
		grad  = (pBlockStamps[base]>>8) & 0xff;
		angle = pBlockStamps[base] & 0xff;
		trigflag = (angle <= 90) ? 1: -1;
		a = ryuSinShift(angle);
		b = trigflag * ryuCosShift(angle);
		x0 = j;
		y0 = i;
		ele_cnt = length = 1;
		angleflag = (angle < angdiff_thre || angle > 180 - angdiff_thre);
		ele_ang = (angleflag && angle < 90) ? (angle + 180) : angle;

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
		cvCopy(show_img2, show_img4);
		// 绘制起始点
		cvRectangle(show_img4, cvPoint(x0 * 4, y0 * 4), cvPoint(x0 * 4 + 3, y0 * 4 + 3), CV_RGB(0,255,0));
		printf("\n");
		printf("起始点坐标：%d, %d   搜索角度：%d\n", x0, y0, angle);
		cvNamedWindow("Feature Cluster Line by Step");
		cvShowImage("Feature Cluster Line by Step", show_img4);
		cvWaitKey();
#endif
#endif
#endif
#endif

		// 计算方向上点追踪的x,y坐标步长
		// 此处逻辑示意图见笔记本
		if(a < b) {
			xflag = 1;
			dx0 = -1;
			dy0 = (a << shift) / b * (0 - trigflag);
			y0 = (y0 << shift) + (1 << (shift-1));
		} else {
			xflag = 0;
			dy0 = 0 - trigflag;
			dx0 = 0 - (b << shift) / a;
			x0 = (x0 << shift) + (1 << (shift-1));
		}

		for(k = 0; k < 2; k++) {
			gap = 0; dx = dx0; dy = dy0;

			if(k > 0)
				dx = -dx, dy = -dy;

			x = x0 + dx;
			y = y0 + dy;

			// 用固定步长沿此点的两个方向寻找直线上的点
			// 直至图像边届或出现过大的跳跃
			for( ;; x += dx, y += dy)
			{
				if( xflag ) {
					j1 = x;
					i1 = y >> shift;
				} else {
					j1 = x >> shift;
					i1 = y;
				}

				// 图像边界
				if(j1 < 0 || j1 >= nLctBlobsWid || i1 < 0 || i1 >= nLctBlobsHei) {
					length += lineGap;
					break;
				}

				base = i1 * nLctBlobsWid + j1;
				pMask = pBlobMask + base;
				angle_f = pBlockStamps[base] & 0xff;
				grad_f  = (pBlockStamps[base] >> 8) & 0xff;
				density_f = (pBlockStamps[base]>>16) & 0xff;

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
				// 绘制测量点
				if(*pMask) {
					cvRectangle(show_img4, cvPoint(j1 * 4, i1 * 4), cvPoint(j1 * 4 + 3, i1 * 4 + 3), CV_RGB(128,0,0));
					printf("搜索点坐标%d, %d  mask%d  该点角度%d  角度差%d\n", j1, i1, *pMask, (*pStamp & 0xff),
						ANGLE_DIFF(abs(angle_f-angle)));
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
				}
#endif
#endif
#endif
#endif
				// 对于聚类点：
				// 更新线的端点,
				// 重令gap为0
				if(*pMask && angdiff_thre > ryuCycleDistance(angle, angle_f, 180)) {
					gap = 0;
					ele_cnt++;
					grad += grad_f;
					density += density_f;
					ele_ang += (angleflag && angle_f < 90) ? (angle_f + 180) : angle_f;
					line_end[k][1] = i1;
					line_end[k][0] = j1;
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
					// 标记测量点
					cvRectangle(show_img4, cvPoint(j1 * 4, i1 * 4), cvPoint(j1 * 4 + 3, i1 * 4 + 3), CV_RGB(255,255,0));
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
#endif
#endif
#endif
#endif
				// 空隙数超过阈值，过大的跳跃
				} else if (++gap > lineGap) {
#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
					printf("空隙过大，结束该方向检测\n");
					cvNamedWindow("Feature Cluster Line by Step");
					cvShowImage("Feature Cluster Line by Step", show_img4);
					cvWaitKey();
#endif
#endif
#endif
#endif
					break;
				}
				length++;
			}
		}

		if(0 >= ele_cnt)
			continue;

		// 判断此线是否是满足长度lineLength的直线
		good_line = abs(line_end[1][0] - line_end[0][0]) >= lineLength ||
			abs(line_end[1][1] - line_end[0][1]) >= lineLength;

		if(!good_line)
			continue;

		// 重新遍历此线来:
		// 置线上的点mask为0
		// 对good_line来减去此线在累加器上的作用
		for(k = 0; k < 2; k++) {
			dx = dx0, dy = dy0;

			if(k > 0)
				dx = -dx, dy = -dy;

			x = x0 + dx;
			y = y0 + dy;

			for( ; ; x += dx, y += dy) {
				if( xflag ) {
					j1 = x;
					i1 = y >> shift;
				} else {
					j1 = x >> shift;
					i1 = y;
				}

				// 图像边界
				if(j1 < 0 || j1 >= nLctBlobsWid || i1 < 0 || i1 >= nLctBlobsHei)
					break;

				pMask = pBlobMask + i1 * nLctBlobsWid + j1;
				if(*pMask) {
					*pMask &= 0x0f;		// 去除首选点
				}

				if(i1 == line_end[k][1] && j1 == line_end[k][0])
					break;
			}
		}

		//  把此线端点存入lines结构
		if(good_line) {
			pBlobClusLine[line_cnt].label = 0;
			pBlobClusLine[line_cnt].line.pt0.x = line_end[0][0];
			pBlobClusLine[line_cnt].line.pt0.y = line_end[0][1];
			pBlobClusLine[line_cnt].line.pt1.x = line_end[1][0];
			pBlobClusLine[line_cnt].line.pt1.y = line_end[1][1];
			pBlobClusLine[line_cnt].center.x = (line_end[0][0] + line_end[1][0]) >> 1;
			pBlobClusLine[line_cnt].center.y = (line_end[0][1] + line_end[1][1]) >> 1;
			pBlobClusLine[line_cnt].angle = angle;
			pBlobClusLine[line_cnt].element = ele_cnt;
			pBlobClusLine[line_cnt].length = length - 2 * lineGap;
			ele_ang /= ele_cnt;
			pBlobClusLine[line_cnt].avg_angle = (180 <= ele_ang) ? (ele_ang - 180) : ele_ang;
			grad /= ele_cnt;
			pBlobClusLine[line_cnt].avg_grad = (grad > 0xff) ? 0xff : grad;
			pBlobClusLine[line_cnt].density = density;
			if(++line_cnt >= nLctClusMaxSize)
				break;
		}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
#ifdef _DEBUG_THIS_PARAGRAGH
		cvLine(show_img4, cvPoint(line_end[0][0]*4+2, line_end[0][1]*4+2), cvPoint(line_end[1][0]*4+2, line_end[1][1]*4+2),
			CV_RGB(255,255,0), 1);
		cvNamedWindow("Feature Cluster Line by Step");
		cvShowImage("Feature Cluster Line by Step", show_img4);
		cvWaitKey();
#endif
#endif
#endif
#endif
	}

	line_cnt -= 1;

	// 根据长度对测得直线进行排序
	for(i = 1; i <= line_cnt; i++) {
		for(j = i + 1; j <= line_cnt; j++) {
			if(pBlobClusLine[i].element < pBlobClusLine[j].element) {
				tmpClusLine = pBlobClusLine[i];
				pBlobClusLine[i] = pBlobClusLine[j];
				pBlobClusLine[j] = tmpClusLine;
			}
		}
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	cvDestroyWindow("Feature Cluster Line by Step");

#ifdef _DEBUG_THIS_PARAGRAGH
	for(i = 1; i <= line_cnt; i++) {
		cvCopy(show_img2, show_img3);
		cvLine(show_img3, cvPoint(pBlobClusLine[i].line.pt0.x*4+2, pBlobClusLine[i].line.pt0.y*4+2),
			cvPoint(pBlobClusLine[i].line.pt1.x*4+2, pBlobClusLine[i].line.pt1.y*4+2), CV_RGB(0,255,0));
		cvNamedWindow("Feature Cluster Line");
		cvShowImage("Feature Cluster Line", show_img3);
		cvWaitKey();
	}
#endif

	cvCopy(show_img, show_img3);
	for(i = line_cnt; i > 0; i--) {
		cvLine(show_img3, cvPoint(pBlobClusLine[i].line.pt0.x*4+2, pBlobClusLine[i].line.pt0.y*4+2),
			cvPoint(pBlobClusLine[i].line.pt1.x*4+2, pBlobClusLine[i].line.pt1.y*4+2),
			CV_RGB(rand()%192+63,rand()%192+63,rand()%192+63), 1);
	}

	printf("Find %d Feature Cluster Lines\n", line_cnt);
	cvNamedWindow("Feature Cluster Line");
	cvShowImage("Feature Cluster Line", show_img3);
	cvWaitKey();
#endif
#endif
#endif

	nRet = line_cnt;

	return nRet;
}


inline int fnc_inline_avg_angle(int ang0, int cnt0, int ang1, int cnt1)
{
	int avg_ang = 0;

	if(90 < abs(ang0 - ang1)) {
		if(ang0 > ang1) {
			avg_ang = (ang0 * cnt0 + (ang1 + 180) * cnt1) / (cnt0 + cnt1);
		} else {
			avg_ang = ((ang0 + 180) * cnt0 + ang1 * cnt1) / (cnt0 + cnt1);
		}
		avg_ang = (avg_ang < 180) ? avg_ang : (avg_ang - 180);
	} else {
		avg_ang = FNC_AVG_VALUE(ang0, cnt0, ang1, cnt1);
	}

	return avg_ang;
}


int FNC_LBC03_LineCluster2Area(int line_cnt)
{
	int nRet = 0;
	int i = 0, j = 0;
	int clus_cnt = 1, isMatch = 0, effcclus_cnt = 0, goodclus_cnt = 0;
	int theta0 = 0, dist0 = 0, dist = 0, idx0 = 0, idx = 0;

	LocateClusLine * pBlobClusLine = clLctBlobClusLine;
	LocateClusArea * pBlobClusArea = caLctBlobClusArea, * pBCA = 0;
	LocateClusArea tmpBlobClusArea;

	int A = 0, B = 0, AT = 0, BT = 0, C0 = 0, C1 = 0, CT0 = 0, CT1 = 0;

	const int distthreshold = 2 << TRIGONOMETRIC_SHIFT_DIGIT;
	const int distthreshold2 = (1 << TRIGONOMETRIC_SHIFT_DIGIT) + (1 << (TRIGONOMETRIC_SHIFT_DIGIT-1));
	const int onelineclusthre = 5;
	const int intcpt_ext = 24, ontcpt_ext = 32;
	const int densitythreshold = 200;

	// 特征线聚类
	for(i = 1; i <= line_cnt; i++) {
		isMatch = 0;
		theta0 = pBlobClusLine[i].avg_angle + 90;
		dist0 = pBlobClusLine[i].center.x * ryuCosShift(theta0) + pBlobClusLine[i].center.y * ryuSinShift(theta0);
		idx0 = pBlobClusLine[i].label;
		// 若所在聚类区域为子节点，则计算时指向父区域
		pBCA = &pBlobClusArea[idx0];
		while(idx0 > 0 && pBCA->parent > 0) {
			idx0 = pBCA->parent;
			pBCA = &pBlobClusArea[idx0];
		}

		// 当聚类中心直线长度已明显小于聚类长度时，则不继续作为中心进行聚类
		if(idx0 && pBlobClusLine[i].length < pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length / 4) {
			continue;
		}

		for(j = i + 1; j <= line_cnt; j++) {
			idx = pBlobClusLine[j].label;
			pBCA = &pBlobClusArea[idx];
			while(idx > 0 && pBCA->parent > 0) {
				idx = pBCA->parent;
				pBCA = &pBlobClusArea[idx];
			}

			if(idx0 && idx0 == idx)
				continue;

//			// 元素数验证
//			if(pBlobClusLine[i].element > 2 * pBlobClusLine[j].element)
//				continue;
//
//			// 长度验证
//			if(pBlobClusLine[i].length > 2 * pBlobClusLine[j].length)
//				continue;
//
//			if(idx0 && pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length > 2 * pBlobClusLine[j].length)
//				continue;

			// 角度验证
			if(18 < ryuCycleDistance(pBlobClusLine[i].avg_angle, pBlobClusLine[j].avg_angle, 180))
				continue;

			// 中心点距离验证
			if(abs(pBlobClusLine[i].center.x - pBlobClusLine[j].center.x) > pBlobClusLine[i].length / 2)
				continue;

			if(abs(pBlobClusLine[i].center.y - pBlobClusLine[j].center.y) > pBlobClusLine[i].length / 2)
				continue;

			dist = pBlobClusLine[j].center.x * ryuCosShift(theta0) + pBlobClusLine[j].center.y * ryuSinShift(theta0);
			if(abs(dist - dist0) > distthreshold)
				continue;

			// 修改新的长度聚类准则，将邻近短线加入聚类，更加强调聚类的集中性
			if(pBlobClusLine[j].length < 3 * pBlobClusLine[i].length / 4) {
				// 若短线不够接近，则过滤
				if(abs(dist - dist0) > distthreshold2) {
					continue;
				}
				// 如果i元素有已知聚类，且i元素长度与聚类长度接近，则i元素具备聚类周围短线的能力；否则过滤
				if(idx0 && pBlobClusLine[i].length < 3 * pBlobClusLine[pBlobClusArea[idx0].maxlineidx].length / 4) {
					continue;
				}
			}

			// 聚类添加元素
			if(idx0) {
				// 聚类合并
				if(idx) {
					if(idx0 < idx) {
						pBlobClusArea[idx].parent = idx0;
						pBlobClusArea[idx0].linecnt += pBlobClusArea[idx].linecnt;
						pBlobClusArea[idx0].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].center.x, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].center.y, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].grad, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].angle, pBlobClusArea[idx].element);
						pBlobClusArea[idx0].element += pBlobClusArea[idx].element;
						pBlobClusArea[idx0].density += pBlobClusArea[idx].density;
					} else {
						pBlobClusArea[idx0].parent = idx;
						pBlobClusArea[idx].linecnt += pBlobClusArea[idx0].linecnt;
						pBlobClusArea[idx].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].center.x, pBlobClusArea[idx].element);
						pBlobClusArea[idx].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].center.y, pBlobClusArea[idx].element);
						pBlobClusArea[idx].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].grad, pBlobClusArea[idx].element);
						pBlobClusArea[idx].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element,
							pBlobClusArea[idx].angle, pBlobClusArea[idx].element);
						pBlobClusArea[idx].element += pBlobClusArea[idx0].element;
						pBlobClusArea[idx].density += pBlobClusArea[idx0].density;
						idx0 = idx;
					}
				}
				// 添加一个元素
				else {
					pBlobClusLine[j].label = idx0;
					pBlobClusArea[idx0].linecnt += 1;
					pBlobClusArea[idx0].center.x = FNC_AVG_VALUE(pBlobClusArea[idx0].center.x, pBlobClusArea[idx0].element,
						pBlobClusLine[j].center.x, pBlobClusLine[j].element);
					pBlobClusArea[idx0].center.y = FNC_AVG_VALUE(pBlobClusArea[idx0].center.y, pBlobClusArea[idx0].element,
						pBlobClusLine[j].center.y, pBlobClusLine[j].element);
					pBlobClusArea[idx0].grad = FNC_AVG_VALUE(pBlobClusArea[idx0].grad, pBlobClusArea[idx0].element,
						pBlobClusLine[j].avg_grad, pBlobClusLine[j].element);
					pBlobClusArea[idx0].angle = fnc_inline_avg_angle(pBlobClusArea[idx0].angle, pBlobClusArea[idx0].element,
						pBlobClusLine[j].angle, pBlobClusLine[j].element);
					pBlobClusArea[idx0].element += pBlobClusLine[j].element;
					pBlobClusArea[idx0].density += pBlobClusLine[j].density;
				}
			}
			// 加入已有聚类
			else if(idx) {
				idx0 = idx;
				pBlobClusLine[i].label = idx;
				pBlobClusArea[idx].linecnt += 1;
				pBlobClusArea[idx].center.x = FNC_AVG_VALUE(pBlobClusArea[idx].center.x, pBlobClusArea[idx].element,
					pBlobClusLine[i].center.x, pBlobClusLine[i].element);
				pBlobClusArea[idx].center.y = FNC_AVG_VALUE(pBlobClusArea[idx].center.y, pBlobClusArea[idx].element,
					pBlobClusLine[i].center.y, pBlobClusLine[i].element);
				pBlobClusArea[idx].grad = FNC_AVG_VALUE(pBlobClusArea[idx].grad, pBlobClusArea[idx].element,
					pBlobClusLine[i].avg_grad, pBlobClusLine[i].element);
				pBlobClusArea[idx].angle = fnc_inline_avg_angle(pBlobClusArea[idx].angle, pBlobClusArea[idx].element,
					pBlobClusLine[i].angle, pBlobClusLine[i].element);
				pBlobClusArea[idx].element += pBlobClusLine[i].element;
				pBlobClusArea[idx].density += pBlobClusLine[i].density;
			}
			// 生成新聚类
			else {
				pBlobClusArea[clus_cnt].flag = 0;
				idx0 = pBlobClusArea[clus_cnt].label = clus_cnt;
				pBlobClusArea[clus_cnt].parent = 0;
				pBlobClusLine[i].label = pBlobClusLine[j].label = clus_cnt;
				pBlobClusArea[clus_cnt].linecnt = 2;
				pBlobClusArea[clus_cnt].maxlineidx = i;
				pBlobClusArea[clus_cnt].element = pBlobClusLine[i].element + pBlobClusLine[j].element;
				pBlobClusArea[clus_cnt].center.x = FNC_AVG_VALUE(pBlobClusLine[i].center.x, pBlobClusLine[i].element,
					pBlobClusLine[j].center.x, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].center.y = FNC_AVG_VALUE(pBlobClusLine[i].center.y, pBlobClusLine[i].element,
					pBlobClusLine[j].center.y, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].grad = FNC_AVG_VALUE(pBlobClusLine[i].avg_grad, pBlobClusLine[i].element,
					pBlobClusLine[j].avg_grad, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].angle = fnc_inline_avg_angle(pBlobClusLine[i].angle, pBlobClusLine[i].element,
					pBlobClusLine[j].angle, pBlobClusLine[j].element);
				pBlobClusArea[clus_cnt].density = pBlobClusLine[i].density + pBlobClusLine[j].density;
				clus_cnt++;
			}
			isMatch = 1;
		}

		// 单线形成聚类
		if(0 == isMatch && 0 == idx0) {
			if(pBlobClusLine[i].length > onelineclusthre) {
				pBlobClusArea[clus_cnt].flag = 0;
				pBlobClusArea[clus_cnt].label = clus_cnt;
				pBlobClusArea[clus_cnt].parent = 0;
				pBlobClusArea[clus_cnt].linecnt = 1;
				pBlobClusArea[clus_cnt].maxlineidx = i;
				pBlobClusArea[clus_cnt].element = pBlobClusLine[i].element;
				pBlobClusArea[clus_cnt].center = pBlobClusLine[i].center;
				pBlobClusArea[clus_cnt].angle = pBlobClusLine[i].avg_angle;
				pBlobClusArea[clus_cnt].grad = pBlobClusLine[i].avg_grad;
				pBlobClusArea[clus_cnt].density = pBlobClusLine[i].density;
				pBlobClusLine[i].label = idx0 = clus_cnt;
				clus_cnt++;
			}
		}
	}

	clus_cnt -= 1;

	// 矫正线标记，计算特征区域边界参数
	for(j = 1; j <= line_cnt; j++) {
		idx = pBlobClusLine[j].label;
		pBCA = &pBlobClusArea[idx];
		while(idx > 0 && pBCA->parent > 0) {
			idx = pBCA->parent;
			pBCA = &pBlobClusArea[idx];
		}
		pBlobClusLine[j].label = idx;

		if(0 == idx) {
			continue;
		}

		A = ryuCosShift(pBlobClusArea[idx].angle + 90);
		B = ryuSinShift(pBlobClusArea[idx].angle + 90);
		C0 = A * ((pBlobClusLine[j].line.pt0.x + 1)<<4) + B * ((pBlobClusLine[j].line.pt0.y+1)<<4);
		C0 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		C1 = A * ((pBlobClusLine[j].line.pt1.x + 1)<<4) + B * ((pBlobClusLine[j].line.pt1.y+1)<<4);
		C1 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		AT = ryuCosShift(pBlobClusArea[idx].angle + 180);
		BT = ryuSinShift(pBlobClusArea[idx].angle + 180);
		CT0 = AT * ((pBlobClusLine[j].line.pt0.x + 1)<<4) + BT * ((pBlobClusLine[j].line.pt0.y+1)<<4);
		CT0 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		CT1 = AT * ((pBlobClusLine[j].line.pt1.x + 1)<<4) + BT * ((pBlobClusLine[j].line.pt1.y+1)<<4);
		CT1 >>= TRIGONOMETRIC_SHIFT_DIGIT;
		if(pBlobClusArea[idx].flag) {
			pBlobClusArea[idx].min_intcpt = RYUMIN(pBlobClusArea[idx].min_intcpt, C0);
			pBlobClusArea[idx].min_intcpt = RYUMIN(pBlobClusArea[idx].min_intcpt, C1);
			pBlobClusArea[idx].max_intcpt = RYUMAX(pBlobClusArea[idx].max_intcpt, C0);
			pBlobClusArea[idx].max_intcpt = RYUMAX(pBlobClusArea[idx].max_intcpt, C1);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(pBlobClusArea[idx].min_ontcpt, CT0);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(pBlobClusArea[idx].min_ontcpt, CT1);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(pBlobClusArea[idx].max_ontcpt, CT0);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(pBlobClusArea[idx].max_ontcpt, CT1);
		} else {
			pBlobClusArea[idx].min_intcpt = RYUMIN(C0, C1);
			pBlobClusArea[idx].max_intcpt = RYUMAX(C0, C1);
			pBlobClusArea[idx].min_ontcpt = RYUMIN(CT0, CT1);
			pBlobClusArea[idx].max_ontcpt = RYUMAX(CT0, CT1);
			pBlobClusArea[idx].flag = 1;
		}
	}

	// 根据聚类标号、形状特征等筛选有效区域，计算有效区域边界四点
	for(i = 1; i <= clus_cnt; i++) {
		// 标记子区域及空区域标识为0
		if(0 != pBlobClusArea[i].parent || 0 >= pBlobClusArea[i].linecnt) {
			pBlobClusArea[i].flag = 0;
			continue;
		}

		if(densitythreshold > pBlobClusArea[i].density) {
			pBlobClusArea[i].flag = 0;
			continue;
		}


		C0  = pBlobClusArea[i].max_intcpt - pBlobClusArea[i].min_intcpt;
		CT0 = pBlobClusArea[i].max_ontcpt - pBlobClusArea[i].min_ontcpt;

		// 版本2.3.2修改20170228
		// 过滤掉超大型区域，优化算法时间
		//printf("[%d] C0=%d, CT0=%d, AREA=%d\n", i, C0, CT0, C0*CT0);
		if(C0 > LOCATE_OUTPUT_MAXHEIGHT || CT0 > LOCATE_OUTPUT_MAXWIDTH
			|| C0 * CT0 > LOCATE_OUTPUT_MAXAREAS) {
			pBlobClusArea[i].flag = 0;
			continue;
		}

		// 标记为effcclus
		effcclus_cnt++;

		// 标记形状不符一般条形码特征区域的标识为1
		//if(C0 > CT0) {
		if(C0 > CT0 * 2) {
			pBlobClusArea[i].flag = 1;
		}
		// 标记形状符合一般条形码特征区域的标识为2，认为是goodclus
		else {
			pBlobClusArea[i].flag = 2;
			goodclus_cnt++;
		}

		// 换算为图像实际坐标系
		pBlobClusArea[i].center.x = (pBlobClusArea[i].center.x + 1) << 4;
		pBlobClusArea[i].center.y = (pBlobClusArea[i].center.y + 1) << 4;

		// 适当拓宽边界，计算边界四点
		pBlobClusArea[i].min_intcpt -= intcpt_ext;
		pBlobClusArea[i].max_intcpt += intcpt_ext;
		pBlobClusArea[i].min_ontcpt -= ontcpt_ext;
		pBlobClusArea[i].max_ontcpt += ontcpt_ext;

// 		C0  += (intcpt_ext << 1);
// 		CT0 += (ontcpt_ext << 1);
// 
// 		// 根据角度扩宽边界,以补足旋转造成的缺损
// 		A = abs(ryuSinShift(pBlobClusArea[i].angle));
// 		B = abs(ryuCosShift(pBlobClusArea[i].angle));
// 		A = RYUMAX(A, B);
// 		C1 = (((C0<<10) / A - C0) >> 1) + 1;
// 		CT1 = (((CT0<<10) / A - CT0) >> 1) + 1;
// 
// 		pBlobClusArea[i].min_intcpt -= C1;
// 		pBlobClusArea[i].max_intcpt += C1;
// 		pBlobClusArea[i].min_ontcpt -= CT1;
// 		pBlobClusArea[i].max_ontcpt += CT1;

		// 获取四点坐标
		A = ryuCosShift(pBlobClusArea[i].angle+90);
		B = ryuSinShift(pBlobClusArea[i].angle+90);
		AT = ryuCosShift(pBlobClusArea[i].angle+180);
		BT = ryuSinShift(pBlobClusArea[i].angle+180);

		pBlobClusArea[i].corner[0].x = (pBlobClusArea[i].min_intcpt * BT - pBlobClusArea[i].max_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[0].y = (pBlobClusArea[i].max_ontcpt * A - pBlobClusArea[i].min_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[1].x = (pBlobClusArea[i].max_intcpt * BT - pBlobClusArea[i].max_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[1].y = (pBlobClusArea[i].max_ontcpt * A - pBlobClusArea[i].max_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[2].x = (pBlobClusArea[i].min_intcpt * BT - pBlobClusArea[i].min_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[2].y = (pBlobClusArea[i].min_ontcpt * A - pBlobClusArea[i].min_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;

		pBlobClusArea[i].corner[3].x = (pBlobClusArea[i].max_intcpt * BT - pBlobClusArea[i].min_ontcpt * B)>>TRIGONOMETRIC_SHIFT_DIGIT;
		pBlobClusArea[i].corner[3].y = (pBlobClusArea[i].min_ontcpt * A - pBlobClusArea[i].max_intcpt * AT)>>TRIGONOMETRIC_SHIFT_DIGIT;
	}

	// 根据优先级和元素个数排序
	for(i = 1; i <= clus_cnt; i++) {
		for(j = i + 1; j <= clus_cnt; j++) {
			if(pBlobClusArea[i].flag < pBlobClusArea[j].flag) {
				tmpBlobClusArea = pBlobClusArea[i];
				pBlobClusArea[i] = pBlobClusArea[j];
				pBlobClusArea[j] = tmpBlobClusArea;
			} else if(pBlobClusArea[i].flag == pBlobClusArea[j].flag
				&& pBlobClusArea[i].density < pBlobClusArea[j].density) {
					tmpBlobClusArea = pBlobClusArea[i];
					pBlobClusArea[i] = pBlobClusArea[j];
					pBlobClusArea[j] = tmpBlobClusArea;
			}
		}
	}

#ifdef _DEBUG_FLAG
#ifdef _DEBUG_IN_WINDOWS
#ifdef _DEBUG_FASTLOCATE
	cvCopy(show_img, show_img2);
	printf("共找到%d个有效聚类区域\n", goodclus_cnt);
	for(i = 1; i <= clus_cnt; i++) {
		if(0 != pBlobClusArea[i].parent || 0 >= pBlobClusArea[i].linecnt)
			continue;
		printf("聚类区域%d: %d条, 最长长度%d, 拟合角度%d\n",
			i, pBlobClusArea[i].linecnt, pBlobClusLine[pBlobClusArea[i].maxlineidx].length, pBlobClusArea[i].angle);
		CvScalar rgb = CV_RGB(rand()%192+63,rand()%192+63,rand()%192+63);
		for(j = 1; j <= line_cnt; j++) {
			if(pBlobClusArea[i].label == pBlobClusLine[j].label) {
				cvLine(show_img2, cvPoint(pBlobClusLine[j].line.pt0.x*4+2, pBlobClusLine[j].line.pt0.y*4+2),
					cvPoint(pBlobClusLine[j].line.pt1.x*4+2, pBlobClusLine[j].line.pt1.y*4+2),
					rgb, 1);
			}
		}
		for(j = 0; j < 4; j++) {
			sprintf(txt, "%d", j);
			cvPutText(show_img2, txt, cvPoint(pBlobClusArea[i].corner[j].x/4+2, pBlobClusArea[i].corner[j].y/4), &font, rgb);
			cvCircle(show_img2, cvPoint(pBlobClusArea[i].corner[j].x/4, pBlobClusArea[i].corner[j].y/4), 2, rgb, CV_FILLED);
		}
	}
	cvNamedWindow("Feature Cluster Area");
	cvShowImage("Feature Cluster Area", show_img2);
	cvWaitKey();
#endif
#endif
#endif

	nRet = goodclus_cnt;

	return nRet;
}


