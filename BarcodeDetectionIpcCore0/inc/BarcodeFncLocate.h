#ifndef _LOCATE_CODE_H
#define _LOCATE_CODE_H

#include "DspCommon.h"
#include "RyuCore.h"

#define USE_NEW_LOCATE_ALGORITHM

typedef struct tagLocateClusLine
{
	int			label;
	RyuLine		line;
	RyuPoint	center;
	int			angle;
	int			length;
	int			element;
	int			avg_angle;
	int			avg_grad;
	int			density;
} LocateClusLine;

typedef struct tagLocateClusArea
{
	int			flag;
	int			label;
	int			parent;
	int			angle;
	int			grad;
	int			element;
	int			linecnt;
	int			density;
	RyuPoint	corner[4];
	RyuPoint	center;
	int			maxlineidx;
	int			min_intcpt;
	int			max_intcpt;
	int			min_ontcpt;
	int			max_ontcpt;
} LocateClusArea;

typedef struct tagFastLocateClus
{
	int		clus_label;
	int		tgt_num;
	int		fus_num;
	int		angle;
	int		center_x;
	int		center_y;
	int		intrcpt;
	int		intrcpt_t;
	int		cen_intr;
	int		cen_intr_t;
	int		Ixx;
	int		Iyy;
	int		Ixy;
	short	LTRB[4];
	short	rect_ptX[4];
	short	rect_ptY[4];
} FastLocateClus;

int  BarcodeLocate_init(IHeap_Handle heap_id, int img_max_wid, int img_max_hei, BarcodeGlobalPointers ** globalPtrs);

void BarcodeLocate_release(IHeap_Handle heap_id);


#endif



