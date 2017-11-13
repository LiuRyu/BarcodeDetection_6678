/*
 * BarcodeFncSegment.h
 *
 *  Created on: 2015-11-5
 *      Author: windlyu
 */

#ifndef BARCODE_SEGMENTATION_H
#define BARCODE_SEGMENTATION_H

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

int mapBarcodeSegmentGlobalPtrs(BarcodeGlobalPointers * globalPtr);

SegmentBarcodeArea * GetSegmentBarcodeAreaPtr();

int SegmentBarcode( unsigned char * in_data, int width, int height,
		RyuPoint * corner, RyuPoint * intercept, RyuPoint * ontercept, int * angle, int ref_grad,
		int barcode_area_idx);

void UpdateCodeCorner( SegmentBarcodeArea * codeArea, int leftOffset, int rightOffset );

int InterceptCvt2Corners( RyuPoint intercept, RyuPoint ontercept, int angle, RyuPoint * corners );


#endif
