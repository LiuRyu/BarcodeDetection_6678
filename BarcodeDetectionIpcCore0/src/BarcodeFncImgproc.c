
#include "BarcodeFncImgproc.h"


static int gnImgprocInitFlag = 0;


int BarcodeImgproc_init(IHeap_Handle heap_id, int max_wid, int max_hei, BarcodeGlobalPointers ** globalPtrs)
{
	int ret_val = 0;
	int i = 0;

	if(gnImgprocInitFlag) {
		ret_val = -10820000;
		goto nExit;
	}

	if(max_wid <= 0 || max_hei <= 0) {
		ret_val = -10820001;
		goto nExit;
	}

	for(i = 0; i < NTHREADS; i++) {
		// 内存复用
		globalPtrs[i]->ucImgprocBuff1 = globalPtrs[i]->ucImgRttZoom;
		if(!globalPtrs[i]->ucImgprocBuff1) {
			ret_val = -10820002;
			goto nExit;
		}

		globalPtrs[i]->ucImgprocOutput = globalPtrs[i]->ucImgRttImage2;
		if(!globalPtrs[i]->ucImgprocOutput) {
			ret_val = -10820003;
			goto nExit;
		}
	}

	gnImgprocInitFlag = 1;
	ret_val = 1;

nExit:
	return ret_val;
}

void BarcodeImgproc_release(IHeap_Handle heap_id)
{
	gnImgprocInitFlag = 0;

	return;
}


int ryuImageContrastAnalyze(unsigned char * img, int width, int height, int widthstep, int * hist,
						float low_ratio, int * low_scale, float high_ratio, int * high_scale,
						int * min_scale, int * max_scale,
						int * avg_scale, int * mid_scale, int * grav_scale)
{
	int gHistNorm_hist[256] = {0};

	int i = 0, j = 0;
	int index = 0;
	int temp = 0, temp2 = 0;
	int size = height * width;

	int lowThresh = (int)(size * low_ratio);
	int highThresh = (int)(size * high_ratio);

	long sum = 0;

	unsigned char * pImg = 0, * pImgL = img;
	int * pHist = 0;

	if(NULL == hist)
		hist = gHistNorm_hist;
	else
		memset(hist, 0, 256 * sizeof(int));

	// 统计图像亮度直方图
	for(j = 0; j < height; j++) {
		pImg = pImgL;
		for(i = 0; i < width; i++) {
			hist[*pImg]++;
			sum += *pImg;
			pImg++;
		}
		pImgL += widthstep;
	}

	if(avg_scale) {
		*avg_scale = (int)(1.0 * sum / size + 0.5);
	}

	if(min_scale) {
		pHist = hist;
		for(i = 0; i < 256; i++) {
			if(*pHist > 0) {
				index = i;
				break;
			}
			pHist++;
		}
		*min_scale = index;
	}

	if(max_scale) {
		pHist = hist + 255;
		for(i = 255; i >= 0; i--) {
			if(*pHist > 0) {
				index = i;
				break;
			}
			pHist--;
		}
		*max_scale = index;
	}

	if(low_scale && lowThresh > 0) {
		index = temp = 0;
		pHist = hist;
		for(i = 0; i < 256; i++) {
			temp += (*pHist);
			pHist++;
			if(temp >= lowThresh) {
				index = i;
				break;
			}
		}
		*low_scale = index;
	}

	if(high_scale && highThresh > 0) {
		index = temp = 0;
		pHist = hist + 255;
		for(i = 255; i >= 0; i--) {
			temp += (*pHist);
			pHist--;
			if(temp >= highThresh) {
				index = i;
				break;
			}
		}
		*high_scale = index;
	}

	if(mid_scale) {
		index = temp = 0;
		pHist = hist;
		for(i = 0; i < 256; i++) {
			temp += (*pHist);
			pHist++;
			if(temp >= (size>>1)) {
				index = i;
				break;
			}
		}
		*mid_scale = index;
	}

	if(grav_scale) {
		temp = temp2 = 0;
		pHist = hist + 255;
		for(i = 255; i >= 0; i--) {
			temp += (*pHist);
			temp2 += (*pHist) * i;
			pHist--;
		}

		if(temp > 0) {
			*grav_scale = temp2 / temp;
		} else {
			*grav_scale = 0;
		}
	}

	return 1;
}

