#ifndef IMAGE_ROTATION_H
#define IMAGE_ROTATION_H


#include "DspCommon.h"
#include "RyuCore.h"

int mapBarcodeRotateGlobalPtrs(BarcodeGlobalPointers * globalPtr);

// angle����ת�ĽǶȣ��Զ�Ϊ��λ,ָ����ʱ����ת�Ƕ�
// srcPts:��Ч����������ĸ��˵����꣬xy�������ʽ
int RotateImage(unsigned char* ucSrcImg, short sSrcImgWidth, short sSrcImgHeight,
				 RyuPoint * corner, int cAngle, int cZoom, short * usDstW,short * usDstH);

unsigned char * GetRotateImage();

int GetZoomOutImage(unsigned char * src, int src_wid, int src_hei, int zoom,
		unsigned char * dst, int * dst_wid, int * dst_hei);


#endif


