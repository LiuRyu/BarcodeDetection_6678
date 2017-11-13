/*
 * BarcodeFncSegment.h
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#ifndef BARCODEFNCSEGMENT_H_
#define BARCODEFNCSEGMENT_H_

#include "DspCommon.h"
#include "RyuCore.h"

#define MAX_SEGMENT_COUNT		(128)
#define MAX_SEGMENT_SLAVE_COUNT	(18)

typedef struct tagSegmentBarcodeArea
{
	RyuPoint	corner[4];
	RyuPoint	corner_ext[4];
	int			angle;
	int			zoom;			// 是否需要做放大处理
	int			min_intcpt;
	int			max_intcpt;
	int			min_ontcpt;
	int			max_ontcpt;
	// 新增加OCR附加变量
	int			ex_symbol;
	int			ex_startbit;
	int			ex_checkbit;
} SegmentBarcodeArea;

int BarcodeSegment_init(IHeap_Handle heap_id, int img_max_wid, int img_max_hei, int seq_size,
		BarcodeGlobalPointers ** globalPtrs);

void BarcodeSegment_release(IHeap_Handle heap_id);


#endif /* BARCODEFNCSEGMENT_H_ */
