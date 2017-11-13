/* 
 * windlyu 20131219 modify 
 */
#ifndef RECDECODE_MOD_H
#define RECDECODE_MOD_H

#include "DspCommon.h"
#include "RyuCore.h"

typedef struct DecodeDemarcateNode
{
	int type;
	int idx_b;
	int idx_e;
	int idxex_b;
	int idxex_e;
	int acc;
	int max_v;
	int count;
	float gravity;
} DecodeDemarcateNode;

int BarcodeDecode_init(IHeap_Handle heap_id, int maxImgWid, int maxImgHei, BarcodeGlobalPointers ** globalPtrs);

void BarcodeDecode_release(IHeap_Handle heap_id);


#endif
