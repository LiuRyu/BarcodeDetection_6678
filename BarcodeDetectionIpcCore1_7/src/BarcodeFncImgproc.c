

#include "BarcodeFncImgproc.h"

#ifdef	_DEBUG_
#define _DEBUG_IMGPROC
#ifdef  _DEBUG_IMGPROC
#include "OpenCv_debugTools.h"
#endif
#endif

#define IMGPROC_AOTU_CONTRAST_THRESH	(0.05)
#define IMGPROC_DISTING_GRAYSCALE		(36)
#define IMGPROC_MAX_PIECE_CNT			(8)

int nImgprocMaxWidth = 0;
int nImgprocMaxHeight = 0;
int nImgprocMaxMemScale = 0;

unsigned char * ucImgprocBuff1 = 0;
unsigned char * ucImgprocOutput = 0;

int gnImgprocInitFlag = 0;

int ImVerticalIntegrogram(unsigned char * in_data, int width, int height, int * integrogram);

int USMSharpeningWithBinarize( unsigned char * in_data, unsigned char * out_data, int wid, int hei,
	int amount, int thresh, int radius, int bina_thresh );

int USMSharpening( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
	int amount, int thresh, int radius);

int AutoContrast(unsigned char * img, unsigned char * rglr_img, int width, int height,
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale, int opration);

int AutoContrastAnalyze(unsigned char * img, int width, int height, int widthstep, 
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale);

int ryuThreshold(unsigned char * img, unsigned char * bina, int width, int height, int widthstep, int thresh);

int GaussianBlur3x3(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int GaussianBlur5x5(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int GaussianBlur5x5_Fast(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int DilateDenoising(unsigned char * in_data, unsigned char * out_data, int wid, int hei);

int BarcodeImgProcess(unsigned char * in_data, int width, int height)
{
	int status = 0, nRet = 0, i = 0;

//	int piece = 0, pieceW[IMGPROC_MAX_PIECE_CNT] = {0}, thresh[IMGPROC_MAX_PIECE_CNT] = {0};
	int nTmp1 = 0, nTmp2 = 0, nTmp3 = 0;
	int gravity = 0;

	unsigned char * pImg = 0, * pBuff = 0;
	unsigned char * out_data = ucImgprocOutput;
	int piece = 0, offsetW[128] = {0}, pieceW[128] = {0}, thresh[128] = {0};

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data || !out_data ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x, out_data=0x%x\n",
			in_data, out_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0
		|| width * height > nImgprocMaxMemScale) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height * 3), 8, 1);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*y))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = AutoContrast(in_data, out_data, width, height, IMGPROC_AOTU_CONTRAST_THRESH, 0, 0, &gravity, 1);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height)))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	//////////////////////////////////////////////////////////////////////////
	// 2.0.7更新，将分块和分块阈值计算放到锐化之前
	// 图像分块
	/*
	piece = width * 2 / height;
	piece = RYUMAX( 3, RYUMIN(piece, IMGPROC_MAX_PIECE_CNT) );	// 这里将最小块数限制为3
	nTmp1 = width / piece;
	nTmp2 = 0;
	for( i = 0; i < piece - 1; i++ ) {
		pieceW[i] = nTmp1;
		nTmp2 += nTmp1;
	}
	pieceW[piece-1] = width - nTmp2;


	nTmp3 = pieceW[0];
	for(i = 1; i < piece-1; i++) {
		pIn = in_data + nTmp3;
		nTmp3 += pieceW[i];

		// 获取二值化阈值
		AutoContrastAnalyze(pIn, pieceW[i], height, width, 0.1, &nTmp1, &nTmp2, &gravity);

		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
			if(nTmp2 <= 64)
				thresh[i] = nTmp2 + 1;	// 置阈值为大值(多数置黑)
			else
				thresh[i] = nTmp1;		// 置阈值为小值(多数置白)
		} else if(nTmp1 >= 192) {
			thresh[i] = nTmp1;			// 置阈值为小值(多数置白)
		} else {
			thresh[i] = gravity;	// 取重值
		}
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
		printf("idx=%d, min=%d, max=%d, oldthresh=%d, grav=%d, thresh=%d\n", i, nTmp1, nTmp2, (nTmp1+nTmp2)/2, gravity, thresh[i]);
#endif
#endif
	}
	thresh[0] = thresh[1];
	thresh[piece-1] = thresh[piece-2];
	*/
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 2.1.5更新，将分块宽度固定为32，使分块更细致，阈值更可靠
	piece = (width - 32) / 32 + 2;
	if(piece > 128 || width < 32) {
		nRet = -2;
		goto nExit;
	}
	nTmp1 = width % 32;
	nTmp2 = nTmp1 >> 1;
	nTmp3 = 16 + nTmp2;
	offsetW[0] = 0;
	pieceW[0] = nTmp3 + 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] = nTmp3 + (i-1) * 32 - 16;
		pieceW[i] = 64;
	}
	offsetW[piece-1] = nTmp3 + (piece-2) * 32 - 16;
	pieceW[piece-1] = nTmp1 - nTmp2 + 32;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];

		// 获取二值化阈值
		AutoContrastAnalyze(pImg, pieceW[i], height, width, 0.1, &nTmp1, &nTmp2, &nTmp3);

		if(nTmp2 - nTmp1 <= IMGPROC_DISTING_GRAYSCALE) {
			if(nTmp2 <= 64)
				thresh[i] = nTmp2 + 1;	// 置阈值为大值(多数置黑)
			else
				thresh[i] = nTmp1;		// 置阈值为小值(多数置白)
		} else if(nTmp1 >= 192) {
			thresh[i] = nTmp1;			// 置阈值为小值(多数置白)
		} else {
//			thresh[i] = gravity;	// 取重值
			thresh[i] = (nTmp1 + nTmp2) / 2;	// 取均值
			thresh[i] = (thresh[i] > nTmp3) ? nTmp3 : thresh[i];
		}
#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
		printf("idx=%d, min=%d, max=%d, oldthresh=%d, grav=%d, thresh=%d\n", i, nTmp1, nTmp2, (nTmp1+nTmp2)/2, gravity, thresh[i]);
#endif
#endif
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 更新：将图像在条码方向上切为n块，每块计算二值化阈值，使用不同阈值进行二值化
	// 新增算法部分
 	status = USMSharpening(out_data, out_data, width, height, 5, 0, 2);
 	if( status <= 0 ) {
 #ifdef	_PRINT_PROMPT
 		printf("Warning! Unexpected return of USMSharpening, return=%d, BarcodeImgProcess exit\n",
 			status);
 #endif
 		nRet = 0;
 		goto nExit;
 	}

 	// XXX COMMENT by v2.3.3.6
 	/*
	//////////////////////////////////////////////////////////////////////////
	// 2.1.5更新，更改分块方式
	offsetW[0] = 0;
	pieceW[0] -= 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] += 16;
		pieceW[i] = 32;
	}
	offsetW[piece-1] += 16;
	pieceW[piece-1] -= 16;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];
		ryuThreshold(pImg, pImg, pieceW[i], height, width, thresh[i]);
	}
	//////////////////////////////////////////////////////////////////////////
	 */
 	// XXX COMMENT END

	// XXX MODIFY v2.3.3.6 注释上一段代码【2.1.5更新，更改分块方式】，增加去噪模块
	offsetW[0] = 0;
	pieceW[0] -= 16;
	for( i = 1; i < piece - 1; i++ ) {
		offsetW[i] += 16;
		pieceW[i] = 32;
	}
	offsetW[piece-1] += 16;
	pieceW[piece-1] -= 16;

	for(i = 0; i < piece; i++) {
		pImg = out_data + offsetW[i];
		pBuff = ucImgprocBuff1 + offsetW[i];
		ryuThreshold(pImg, pBuff, pieceW[i], height, width, thresh[i]);
	}
	DilateDenoising(ucImgprocBuff1, out_data, width, height);
	// XXX MODIFY ENDING

	nRet = 1;

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height*2)))[x]=in_data[y*width+x];
		}
	}
	cvNamedWindow("Imgproc");
	cvShowImage("Imgproc", iplImgproc);
	cvWaitKey();
	cvReleaseImage(&iplImgproc);
#endif
#endif

nExit:
	return nRet;
}

int BarcodeImgProcessIntegrogram(unsigned char * in_data, int width, int height)
{
	int status = 0, nRet = 0;

	unsigned char * out_data = ucImgprocOutput;

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data || !out_data) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x, out_data=0x%x\n",
			in_data, out_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0
		|| width > nImgprocMaxWidth || height > nImgprocMaxHeight ) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

	if(width * height > (nImgprocMaxWidth * nImgprocMaxHeight) / 4) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Too large input value of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = 0;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height * 3), 8, 1);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*y))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = AutoContrast(in_data, in_data, width, height, IMGPROC_AOTU_CONTRAST_THRESH, 0, 0, 0, 1);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height)))[x]=in_data[y*width+x];
		}
	}
#endif
#endif

	status = ImVerticalIntegrogram(in_data, width, height, (int *)out_data);
	if( status <= 0 ) {
#ifdef	_PRINT_PROMPT
		printf("Warning! Unexpected return of AutoContrast, return=%d, BarcodeImgProcess exit\n",
			status);
#endif
		nRet = 0;
		goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	int * pOut = (int *)out_data;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			((unsigned char *)(iplImgproc->imageData + iplImgproc->widthStep*(y+height*2)))[x]=pOut[y*width+x] / (y+1);
		}
	}
	cvNamedWindow("Imgproc");
	cvShowImage("Imgproc", iplImgproc);
	cvWaitKey();
	cvReleaseImage(&iplImgproc);
#endif
#endif

	nRet = 1;

nExit:
	return nRet;
}

unsigned char * getBarcodeImgProcOutput()
{
	return ucImgprocOutput;
}

int mapBarcodeImgprocGlobalPtrs(BarcodeGlobalPointers * globalPtr)
{
	nImgprocMaxWidth 	= globalPtr->nImgRttMaxLineSize;
	nImgprocMaxHeight 	= globalPtr->nImgRttMaxLineSize;
	nImgprocMaxMemScale = globalPtr->nImgRttMaxMemScale;
	ucImgprocBuff1 		= globalPtr->ucImgprocBuff1;
	ucImgprocOutput		= globalPtr->ucImgprocOutput;

	gnImgprocInitFlag 	= 1;

	return 1;
}

int ImVerticalIntegrogram(unsigned char * in_data, int width, int height, int * integrogram)
{
	int ret_val = 0;

	int i = 0, j = 0;

	unsigned char * pIn = 0, * pInL = in_data;
	int * pOut = 0, * pOutL = integrogram;
	int * pOutUppr = 0;

	if(NULL == in_data || NULL == integrogram) {
		ret_val = -1;
		goto nExit;
	}

	if(0 >= width || 0 >= height) {
		return -1;
	}

	// 首行
	pIn = pInL;
	pOut = pOutL;
	for(i = 0; i < width; i++) {
		*pOut = *pIn;
		pIn++;
		pOut++;
	}
	pInL += width;
	pOutL += width;

	for(j = 1; j < height; j++) {
		pIn = pInL;
		pOut = pOutL;
		pOutUppr = pOutL - width;
		for(i = 0; i < width; i++) {
			*pOut = *pIn + *pOutUppr;
			pIn++;
			pOut++;
			pOutUppr++;
		}
		pInL += width;
		pOutL += width;
	}

	ret_val = 1;

nExit:

	return ret_val;
}

// 识别图像重组
int BarcodeImageproc_recombination( unsigned char * in_data, int width, int height )
{
	int i = 0, nRet = 0;
	unsigned char * pBuf = ucImgprocBuff1, * pImg1 = 0, * pImg2 = 0;
	int offsetW = width >> 1, offsetH = height >> 1;

	if( 1 != gnImgprocInitFlag ) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! BarcodeImgProcess run WITHOUT init\n");
#endif
		nRet = -1;
		goto nExit;
	}

	if( !in_data) {
#ifdef	_PRINT_PROMPT
		printf("ERROR! Invalid input of BarcodeImgProcess, in_data=0x%x\n",
			in_data);
#endif
		nRet = -1;
		goto nExit;
	}

	if( width <= 0 || height <= 0
		|| width * height > nImgprocMaxMemScale) {
#ifdef	_PRINT_PROMPT
			printf("ERROR! Invalid input of BarcodeImgProcess, width=%d, height=%d\n",
				width, height);
#endif
			nRet = -1;
			goto nExit;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	IplImage * iplImgproc = cvCreateImage(cvSize(width, height*2+16), 8, 1);
	IplImage * iplImgprocHeader = cvCreateImageHeader(cvSize(width, height), 8, 1);
	iplImgprocHeader->imageData = iplImgproc->imageData;
	uc2IplImageGray(in_data, iplImgprocHeader);
#endif
#endif

	pImg1 = in_data + (offsetH-1) * width + offsetW;
	pImg2 = in_data + offsetH * width + offsetW;
	for(i = 0; i < offsetH; i++) {
		memcpy(pBuf, pImg1, sizeof(unsigned char) * offsetW);
		memcpy(pImg1, pImg2, sizeof(unsigned char) * offsetW);
		memcpy(pImg2, pBuf, sizeof(unsigned char) * offsetW);
		pImg1 -= width;
		pImg2 += width;
	}

#ifdef _DEBUG_
#ifdef _DEBUG_IMGPROC
	iplImgprocHeader->imageData = iplImgproc->imageData + (height+16) * iplImgproc->widthStep;
	uc2IplImageGray(in_data, iplImgprocHeader);
	cvNamedWindow("recombination");
	cvShowImage("recombination", iplImgproc);
	cvWaitKey();
	cvDestroyWindow("recombination");
	cvReleaseImage(&iplImgproc);
	cvReleaseImageHeader(&iplImgprocHeader);
#endif
#endif

	nRet = 1;

nExit:
	return nRet;
}


int USMSharpeningWithBinarize( unsigned char * in_data, unsigned char * out_data, int wid, int hei,
		int amount, int thresh, int radius, int bina_thresh )
{
	int i = 0;

	int sub = 0;
	int nsize = wid * hei;

	unsigned char * pIn = in_data;
	unsigned char * pBlur = ucImgprocBuff1;
	unsigned char * pOut = out_data;

	if(1 != gnImgprocInitFlag) {
		return -1;
	}

	if(!in_data || !out_data) {
		return -1;
	}

	if(wid <= 0 || hei <= 0 || wid * hei > nImgprocMaxMemScale) {
		return -1;
	}

	amount = RYUMAX( 0, RYUMIN(255, amount) );
	thresh = RYUMAX( 0, RYUMIN(255, thresh) );

	switch (radius)
	{
	case (1):
		GaussianBlur3x3(pIn, pBlur, wid, hei);

	case (2):
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);

	default:
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);
	}

	for(i = nsize; i > 0; i--) {
		sub = *pIn - *pBlur;
		if(abs(sub) > thresh) {
			sub = *pIn + amount * sub;
// 			sub = (sub > 255) ? 255 : sub;
// 			sub = (sub < 0) ? 0 : sub;
// 			*pOut = sub;
		} else {
//			*pOut = *pIn;
			sub = *pIn;
		}

		*pOut = (sub > bina_thresh) ? 0xff : 0;

		pIn++;
		pBlur++;
		pOut++;
	}

	return 1;
}


int USMSharpening( unsigned char * in_data, unsigned char * out_data, int wid, int hei, 
		int amount, int thresh, int radius)
{
	int i = 0;

	int sub = 0;
	int nsize = wid * hei;

	unsigned char * pIn = in_data;
	unsigned char * pBlur = ucImgprocBuff1;
	unsigned char * pOut = out_data;

	if(1 != gnImgprocInitFlag) {
		return -1;
	}

	if(!in_data || !out_data) {
		return -1;
	}

	if(wid <= 0 || hei <= 0 || wid * hei > nImgprocMaxMemScale) {
		return -1;
	}

	amount = RYUMAX( 0, RYUMIN(255, amount) );
	thresh = RYUMAX( 0, RYUMIN(255, thresh) );

	switch (radius)
	{
	case (1):
		GaussianBlur3x3(pIn, pBlur, wid, hei);

	case (2):
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);

	default:
		GaussianBlur5x5_Fast(pIn, pBlur, wid, hei);
	}

	for(i = nsize; i > 0; i--) {
		sub = *pIn - *pBlur;
		if(abs(sub) > thresh) {
			sub = *pIn + amount * sub;
		} else {
			sub = *pIn;
		}

		*pOut = RYUMAX(0, RYUMIN(255, sub));

		pIn++;
		pBlur++;
		pOut++;
	}

	return 1;
}


int AutoContrast(unsigned char * img, unsigned char * rglr_img, int width, int height,
	float thre_ratio, int * min_scale, int * max_scale, int * grav_scale, int opration)
{
	int gHistNorm_hist[256] = {0};
	int gHistNorm_eqHist[256] = {0};
	int * hist = gHistNorm_hist;
	int * eqHist = gHistNorm_eqHist;

	int ret_val = 0;
	int i = 0, j = 0;
	int l_index = 0, r_index = 255;
	int temp = 0, temp2 = 0, temp3 = 0, m = 0, n = 0;
	int size = height * width;
	int threshold = size * thre_ratio;

	float avg = 0, acc = 0;

	unsigned char * pImg = 0;
	unsigned char * pNorm = 0;
	int * pHist = 0;
	int * pEqHist = 0;

// 	memset(hist, 0, sizeof(int)*256);
// 	memset(eqHist, 0, sizeof(int)*256);

	pImg = img;
	for (i = size; i > 0; i--) //计算图像亮度直方图
	{
		hist[*pImg]++;
		pImg++;
	}

	// 由于0一般都是旋转填充引起的，所以去掉0的累积
	temp = 0;
	pHist = hist + 1;
	for(i = 255; i > 0; i--) {
		temp += (*pHist);
		pHist++;
		if(temp >= threshold) {
			l_index = 256 - i;
			break;
		}
	}

	// 由于255一般都是反光引起的，所以去掉255的累积
	temp = 0;
	pHist = hist + 254;
	for(i = 255; i > 0; i--) {
		temp += (*pHist);
		pHist--;
		if(temp >= threshold) {
			r_index = i - 1;
			break;
		}
	}

	m = r_index - l_index;
	n = 255 - m;

	if(min_scale && max_scale) {
		*min_scale = l_index;
		*max_scale = r_index;
	}

	if(grav_scale) {
		temp = temp2 = 0;
		pHist = hist + 254;
		for(i = 255; i > 0; i--) {
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

	if(1 != opration) {
		ret_val = 0;
		goto nExit;
	}

	if(m < 0 || n < 0) {
		ret_val = -1;
		goto nExit;
	}

	if(m <= 0 || n <= 0) {
		if(img == rglr_img) {
			ret_val = 1;
			goto nExit;
		}
		else {
			memcpy(rglr_img, img, size * sizeof(unsigned char));
			ret_val = 1;
			goto nExit;
		}
	}

	pEqHist = eqHist;
	for(i = l_index + 1; i > 0; i--) {
		*(pEqHist++) = 0;
	}

	pEqHist = eqHist + 255;
	for(i = 255 - r_index + 1; i > 0; i--) {
		*(pEqHist--) = 255;
	}

	if(m < n) {
		avg = n * 1.0 / m;
		acc = 1;
		pEqHist = eqHist + l_index + 1;
		for(i = m - 1; i > 0; i--) {
			acc = acc + avg;
			*(pEqHist++) = acc + 0.5;
			acc = acc + 1;
		}
	} else {
		avg = m * 1.0 / n;
		acc = avg / 2;
		temp = temp2 = 0;
		temp3 = 1;
		pEqHist = eqHist + l_index + 1;
		for(i = n; i > 0; i--) {
			temp = acc + 0.5;
			temp -= temp2;
			for(j = temp; j > 0; j--) {
				*(pEqHist++) = (temp3++);
			}
			temp2 += temp;
			temp3++;
			acc = acc + avg;
		}

		for(i = m - temp2; i > 0; i--) {
			*(pEqHist++) = (temp3++);
		}
	}

	pImg = img;
	pNorm = rglr_img;
	for (i = size; i > 0; i--) //进行灰度映射均衡化
	{
		*pNorm = eqHist[*pImg];
		pNorm++;
		pImg++;
	}

	ret_val = 2;

nExit:
	return ret_val;
}


int AutoContrastAnalyze(unsigned char * img, int width, int height, int widthstep, 
		float thre_ratio, int * min_scale, int * max_scale, int * grav_scale)
{
	int gHistNorm_hist[256] = {0};
	int * hist = gHistNorm_hist;

	int i = 0, j = 0;
	int l_index = 0, r_index = 255;
	int temp = 0, temp2 = 0;
	int size = height * width;
	int threshold = size * thre_ratio;

	unsigned char * pImg = 0, * pImgL = img;
	int * pHist = 0;

	// 统计图像亮度直方图
	for(j = 0; j < height; j++) {
		pImg = pImgL;
		for(i = 0; i < width; i++) {
			hist[*pImg]++;
			pImg++;
		}
		pImgL += widthstep;
	}

	if( threshold > 0 ) {
		temp = 0;
		pHist = hist;
		for(i = 0; i < 256; i++) {
			temp += (*pHist);
			pHist++;
			if(temp >= threshold) {
				l_index = i;
				break;
			}
		}

		// 由于255一般都是反光引起的，所以去掉255的累积
		temp = 0;
		pHist = hist + 255;
		for(i = 255; i > 0; i--) {
			temp += (*pHist);
			pHist--;
			if(temp >= threshold) {
				r_index = i;
				break;
			}
		}

		if(min_scale && max_scale) {
			*min_scale = l_index;
			*max_scale = r_index;
		}
	}

	if(grav_scale) {
		//////////////////////////////////////////////////////////////////////////
		// 2.0.7修正参数，将255像素计算在内
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


int ryuThreshold(unsigned char * img, unsigned char * bina, int width, int height, int widthstep, int thresh)
{
	int i = 0, j = 0;
	unsigned char * pImg = 0, * pImgL = img;
	unsigned char * pBina = 0, * pBinaL = bina;

	for(j = 0; j < height; j++) {
		pImg = pImgL;
		pBina = pBinaL;
		for(i = 0; i < width; i++) {
			*pBina = (*pImg > thresh) ? 255 : 0;
			pImg++;
			pBina++;
		}
		pImgL += widthstep;
		pBinaL += widthstep;
	}

	return 1;
}


int GaussianBlur3x3(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 2;
	int nw		= wid - 2;

	int nsize = wid * hei;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b;
	unsigned char * poffset, * poffset_t, * poffset_b;

	loffset = loffset_t = loffset_b = 0;
	poffset = poffset_t = poffset_b = 0;

	memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * wid);

	loffset		= pIn + 1;
	loffset_t	= pIn - wid + 1;
	loffset_b	= pIn + wid + 1;
	lOut		= pOut + 1;

	for(i = nh; i > 0; i--)
	{
		loffset		+= wid;
		loffset_t	+= wid;
		loffset_b	+= wid;
		poffset		= loffset;
		poffset_t	= loffset_t;
		poffset_b	= loffset_b;

		lOut += wid;
		pOut = lOut;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t-1))	+ ((*poffset_t)<<1)	+ (*(poffset_t+1))
				+ ((*(poffset-1))<<1)	+ ((*poffset)<<2)	+ ((*(poffset+1))<<1)
				+ (*(poffset_b-1))	+ ((*poffset_b)<<1)	+ (*(poffset_b+1));

			*pOut = (t>>4);

			poffset++;
			poffset_t++;
			poffset_b++;
			pOut++;
		}

		*(lOut-1) = *lOut;	// 首元素处理
		*pOut = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+1, poffset+1, sizeof(unsigned char) * wid);

	return 0;
}

int GaussianBlur5x5(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 4;
	int nw		= wid - 4;

	int nsize = wid * hei;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b, * loffset_t2, * loffset_b2;
	unsigned char * poffset, * poffset_t, * poffset_b, * poffset_t2, * poffset_b2;

	loffset = loffset_t = loffset_b = loffset_t2 = loffset_b2 = 0;
	poffset = poffset_t = poffset_b = poffset_t2 = poffset_b2 = 0;

	memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * (wid<<1));

	loffset_t2	= pIn + 2;
	loffset_t	= loffset_t2 + wid;
	loffset		= loffset_t + wid;
	loffset_b	= loffset + wid;
	loffset_b2	= loffset_b + wid;
	lOut		= pOut + (wid<<1) + 2;

	for(i = nh; i > 0; i--)
	{
		poffset_t2  = loffset_t2;
		poffset_t	= loffset_t;
		poffset		= loffset;
		poffset_b	= loffset_b;
		poffset_b2	= loffset_b2;

		loffset_t2	+= wid;
		loffset_t	+= wid;
		loffset		+= wid;
		loffset_b	+= wid;
		loffset_b2	+= wid;

		pOut = lOut;
		lOut += wid;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t2-2))		+ ((*(poffset_t2-1))<<2) + ((*(poffset_t2))*7) + ((*(poffset_t2+1))<<2) + (*(poffset_t2+2))
				+ ((*(poffset_t-2))<<2)	+ ((*(poffset_t-1))<<4)	 + ((*(poffset_t))*26) + ((*(poffset_t+1))<<4)	+ ((*(poffset_t+2))<<2)
				+ ((*(poffset-2))*7)	+ ((*(poffset-1))*26)	 + ((*(poffset))*41)   + ((*(poffset+1))*26)	+ ((*(poffset+2))*7)
				+ ((*(poffset_b-2))<<2)	+ ((*(poffset_b-1))<<4)	 + ((*(poffset_b))*26) + ((*(poffset_b+1))<<4)	+ ((*(poffset_b+2))<<2)
				+ (*(poffset_b2-2))		+ ((*(poffset_b2-1))<<2) + ((*(poffset_b2))*7) + ((*(poffset_b2+1))<<2) + (*(poffset_b2+2));

			*pOut = t / 273;

			poffset_t2++;
			poffset_t++;
			poffset++;
			poffset_b++;
			poffset_b2++;
			pOut++;
		}

		*(lOut-1) = *lOut;		// 首元素处理
		*(lOut-2) = *lOut;		// 首元素处理
		*pOut = *(pOut-1);		// 尾元素处理
		*(pOut+1) = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+2, poffset+2, sizeof(unsigned char) * (wid<<1));

	return 0;
}

int GaussianBlur5x5_Fast(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 4;
	int nw		= wid - 4;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b, * loffset_t2, * loffset_b2;
	unsigned char * poffset, * poffset_t, * poffset_b, * poffset_t2, * poffset_b2;

	loffset = loffset_t = loffset_b = loffset_t2 = loffset_b2 = 0;
	poffset = poffset_t = poffset_b = poffset_t2 = poffset_b2 = 0;

	//memset(pOut, 0, sizeof(unsigned char) * nsize);

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * (wid<<1));

	loffset_t2	= pIn + 2;
	loffset_t	= loffset_t2 + wid;
	loffset		= loffset_t + wid;
	loffset_b	= loffset + wid;
	loffset_b2	= loffset_b + wid;
	lOut		= pOut + (wid<<1) + 2;

	for(i = nh; i > 0; i--)
	{
		poffset_t2  = loffset_t2;
		poffset_t	= loffset_t;
		poffset		= loffset;
		poffset_b	= loffset_b;
		poffset_b2	= loffset_b2;

		loffset_t2	+= wid;
		loffset_t	+= wid;
		loffset		+= wid;
		loffset_b	+= wid;
		loffset_b2	+= wid;

		pOut = lOut;
		lOut += wid;

		for(j = nw; j > 0; j--)
		{
			t = (*(poffset_t2-2))		+ ((*(poffset_t2-1))<<2) + ((*(poffset_t2))<<3) + ((*(poffset_t2+1))<<2) + (*(poffset_t2+2))
				+ ((*(poffset_t-2))<<2)	+ ((*(poffset_t-1))<<4)	 + ((*(poffset_t))<<5) + ((*(poffset_t+1))<<4)	+ ((*(poffset_t+2))<<2)
				+ ((*(poffset-2))<<3)	+ ((*(poffset-1))<<5)	 + ((*(poffset))<<6)   + ((*(poffset+1))<<5)	+ ((*(poffset+2))<<3)
				+ ((*(poffset_b-2))<<2)	+ ((*(poffset_b-1))<<4)	 + ((*(poffset_b))<<5) + ((*(poffset_b+1))<<4)	+ ((*(poffset_b+2))<<2)
				+ (*(poffset_b2-2))		+ ((*(poffset_b2-1))<<2) + ((*(poffset_b2))<<3) + ((*(poffset_b2+1))<<2) + (*(poffset_b2+2));

			*pOut = t / 324;

			poffset_t2++;
			poffset_t++;
			poffset++;
			poffset_b++;
			poffset_b2++;
			pOut++;
		}

		*(lOut-1) = *lOut;		// 首元素处理
		*(lOut-2) = *lOut;		// 首元素处理
		*pOut = *(pOut-1);		// 尾元素处理
		*(pOut+1) = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+2, poffset+2, sizeof(unsigned char) * (wid<<1));

	return 0;
}

// 用变形膨胀法对二值图像进行去噪，去除栅栏间噪声
int DilateDenoising(unsigned char * in_data, unsigned char * out_data, int wid, int hei)
{
	int nh		= hei - 2;
	int nw		= wid - 2;

	int i = 0, j = 0;

	int t = 0;

	unsigned char * pIn = in_data;
	unsigned char * pOut = out_data;
	unsigned char * lOut = 0;

	unsigned char * loffset, * loffset_t, * loffset_b;
	unsigned char * poffset, * poffset_t, * poffset_b;

	loffset = loffset_t = loffset_b = 0;
	poffset = poffset_t = poffset_b = 0;

	// 首行处理
	memcpy(pOut, pIn, sizeof(unsigned char) * wid);

	loffset		= pIn + 1;
	loffset_t	= pIn - wid + 1;
	loffset_b	= pIn + wid + 1;
	lOut		= pOut + 1;

	for(i = nh; i > 0; i--)
	{
		loffset		+= wid;
		loffset_t	+= wid;
		loffset_b	+= wid;
		poffset		= loffset;
		poffset_t	= loffset_t;
		poffset_b	= loffset_b;

		lOut += wid;
		pOut = lOut;

		for(j = nw; j > 0; j--)
		{
			if(0 == *poffset) {
				t = (*(poffset_t-1) == *poffset) + (*poffset_t == *poffset) + (*(poffset_t+1) == *poffset)
					+ (*(poffset-1) == *poffset) + (*(poffset+1) == *poffset)
					+ (*(poffset_b-1) == *poffset) + (*poffset_b == *poffset) + (*(poffset_b+1) == *poffset);

				*pOut = (2 >= t) ? 255 : 0;
			} else {
				*pOut = 255;
			}

			poffset++;
			poffset_t++;
			poffset_b++;
			pOut++;
		}

		*(lOut-1) = *lOut;	// 首元素处理
		*pOut = *(pOut-1);	// 尾元素处理
	}

	// 末行处理
	memcpy(pOut+1, poffset+1, sizeof(unsigned char) * wid);

	return 0;
}

