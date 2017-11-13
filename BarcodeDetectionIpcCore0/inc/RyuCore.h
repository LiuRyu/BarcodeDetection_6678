#ifndef _RYU_CORE_H
#define _RYU_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "RyuType.h"

#define TRIGONOMETRIC_SHIFT_DIGIT		(10)
#define FLOAT2FIXED_SHIFT_DIGIT			(10)
#define CODE_RESULT_ARR_LENGTH			(128)

#ifndef RYUMAX
#define RYUMAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef RYUMIN
#define RYUMIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

/*
int	ryuSinShift(int angel);

int	ryuCosShift(int angel);

int ryuAtanLUTHalf(int angle);
*/
int	ryuArctan180Shift(int dy, int dx);
/*
int ryuCycle(int a, int range);

int	ryuCycleDistance(int a, int b, int range);

RyuPoint ryuDivideIntPoint(int point);

int ryuDistanceBtPoints(RyuPoint pt1, RyuPoint pt2);

void ryuMakeGaussianKernal();


RyuROI * ryuCreateROI(int xOffset, int yOffset, int width, int height);

void ryuSetImageROI(RyuImage* image, RyuRect rect);

RyuRect ryuGetImageROI(const RyuImage * image);

void ryuResetImageROI(RyuImage * image);

RyuImage * ryuCreateImageHeader(RyuSize size, int depth, int channels);

void * ryuInitImageHeader(RyuImage * image, RyuSize size, int depth, int channels);

RyuImage * ryuCreateImage(RyuSize size, int depth, int channels);

void ryuReleaseImageHeader( RyuImage ** image );

void ryuReleaseImage( RyuImage ** image );

void * ryuSetImage( RyuImage * image, RyuSize size, int depth, int channels );

void ryuZero( RyuImage * image );
*/
#endif
