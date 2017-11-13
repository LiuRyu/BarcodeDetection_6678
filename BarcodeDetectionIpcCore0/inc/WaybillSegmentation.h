/*
 * WaybillSegmentation.h
 *
 *  Created on: 2017-1-11
 *      Author: Ryu
 */

#ifndef WAYBILLSEGMENTATION_H_
#define WAYBILLSEGMENTATION_H_

#include "DspCommon.h"
#include "RyuCore.h"

#define WAYBILLSEG_RESIZE_SCALE			(400)

typedef struct code_area_node
{
	int		angle;
	int		min_x, max_x;
	int		min_y, max_y;
	int		width;
	int		height;

	int		flag;
	char	str_code[CODE_RESULT_ARR_LENGTH];
} CAN;

typedef struct flood_fill_node
{
	int		label;
	int		min_x, max_x;
	int		min_y, max_y;
	int		pixel_count;

	int		width;
	int		height;

	int		min_v, max_v;

	int		contour_idx;
	int		contour_cnt;
	int		class_idx;
	int		code_in;
	int		code_cnt;

	int		flag;	// For processing,代表图片中的第i个联通区域存在
} FFN;

typedef struct flood_fill_classcluster
{
	int		index;
	int		min_x, max_x;
	int		min_y, max_y;
	int		ffn_count;

	int		width;
	int		height;

	int		flag;	// For processing,代表图片中的第i个联通区域存在
} FFC;


int WaybillSegment_init(IHeap_Handle heap_id, RyuSize sz, int max_count, BarcodeGlobalPointers ** globalPtrs);

int WaybillSegment_release(IHeap_Handle heap_id);


#endif /* WAYBILLSEGMENTATION_H_ */
