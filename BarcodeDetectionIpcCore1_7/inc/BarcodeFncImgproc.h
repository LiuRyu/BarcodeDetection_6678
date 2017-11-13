

#ifndef BARCODE_IMAGEPROC_H
#define BARCODE_IMAGEPROC_H

#include "DspCommon.h"
#include "RyuCore.h"

int mapBarcodeImgprocGlobalPtrs(BarcodeGlobalPointers * globalPtr);

int BarcodeImgProcessIntegrogram(unsigned char * in_data, int width, int height);

int BarcodeImgProcess(unsigned char * in_data, int width, int height);

unsigned char * getBarcodeImgProcOutput();

int BarcodeImageproc_recombination( unsigned char * in_data, int width, int height );

#endif



