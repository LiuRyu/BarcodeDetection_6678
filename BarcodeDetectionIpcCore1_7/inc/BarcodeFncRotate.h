#ifndef IMAGE_ROTATION_H
#define IMAGE_ROTATION_H


#include "DspCommon.h"
#include "RyuCore.h"

int mapBarcodeRotateGlobalPtrs(BarcodeGlobalPointers * globalPtr);

// angle：旋转的角度，以度为单位,指按逆时针旋转角度
// srcPts:有效矩形区域的四个端点坐标，xy共存的形式
int RotateImage(unsigned char* ucSrcImg, short sSrcImgWidth, short sSrcImgHeight,
				 RyuPoint * corner, int cAngle, int cZoom, short * usDstW,short * usDstH);

unsigned char * GetRotateImage();

int GetZoomOutImage(unsigned char * src, int src_wid, int src_hei, int zoom,
		unsigned char * dst, int * dst_wid, int * dst_hei);


#endif


