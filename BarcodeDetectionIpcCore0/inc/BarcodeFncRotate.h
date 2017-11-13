#ifndef ROTATE_H
#define ROTATE_H

#include "DspCommon.h"
#include "RyuCore.h"

int BarcodeRotate_init(IHeap_Handle heap_id, int width, int height, BarcodeGlobalPointers ** globalPtrs);

void BarcodeRotate_release(IHeap_Handle heap_id);

#endif


