/* 
 * windlyu 20131219 modify 
 */
#ifndef BARCODE_DECODING_H
#define BARCODE_DECODING_H

#include "DspCommon.h"
#include "RyuCore.h"

typedef struct tagOcrParamVoter
{
	int			symbol;
	int			startbit;
	int			checkbit;
	int			derection;
	int			count;
} OcrParamVoter;

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

int mapBarcodeDecodeGlobalPtrs(BarcodeGlobalPointers * globalPtr);

int DecodeBarcode( unsigned char * bina, int width, int height, int sliceH,
	int * code_type, int * char_num, int * char_valid, int * module_num,
	int * code_direct, int * leftOffset, int * rightOffset, char * code_result);

int BarcodeDecoding_run( unsigned char * im, int * integr, int width, int height, int slice_height,
			int * code_type, int * char_num, int * char_valid, int * module_num,
			int * code_direct, int * leftOffset, int * rightOffset, float * minModule,
			char * code_result);

void ResetOcrParams();

int GetOcrParams(int * symbol, int * startbit, int * checkbit, int * derection);

#endif
