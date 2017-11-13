#include "WaybillSegmentation.h"

/************************************************************************/
/* 主函数全局变量                                                        */
/************************************************************************/
int gnFloodFillMatMaxSize = 0, gnFloodFillStatMaxCount = 0;
int gnFloodFillMaxScale = 0;

unsigned char * gucFloodFillImage = 0;

RyuPoint * gptFloodFillSeeds = 0;

int * gnFloodFillLabelMat = 0;

FFN * gtFloodFillStatistics = 0;

/************************************************************************/
/* FindContour全局变量                                                   */
/************************************************************************/
int * gnFloodFillContourMat = 0;		// 轮廓标记数组
int * gnFloodFillContourFlag = 0;
RyuPoint * gptFloodFillContourPts = 0;

/************************************************************************/
/* ClassCluster全局变量                                                  */
/************************************************************************/
int * gnFloodFillClassSeeds = 0;
FFC * gtFloodFillClassClusters = 0;

CAN * gtFloodFillCodeAreas = 0;

int gnWbsegInitFlag = 0;


int WaybillSegment_init(IHeap_Handle heap_id, RyuSize sz, int max_count, BarcodeGlobalPointers ** globalPtrs)
{
	int i = 0;
	if(gnWbsegInitFlag) {
		return -12319001;
	}

	if(sz.width <= 0 || sz.height <= 0 || max_count <= 0) {
		return -12319002;
	}

	for(i = 0; i < NTHREADS; i++) {
		globalPtrs[i]->nFloodFillMatMaxSize = 0;
		globalPtrs[i]->nFloodFillStatMaxCount = 0;
		globalPtrs[i]->ucFloodFillImage = 0;
		globalPtrs[i]->ptFloodFillSeeds = 0;
		globalPtrs[i]->nFloodFillLabelMat = 0;
		globalPtrs[i]->tFloodFillStatistics = 0;
		globalPtrs[i]->nFloodFillContourMat = 0;
		globalPtrs[i]->nFloodFillContourFlag = 0;
		globalPtrs[i]->ptFloodFillContourPts = 0;
		globalPtrs[i]->nFloodFillClassSeeds = 0;
		globalPtrs[i]->tFloodFillClassClusters = 0;
		globalPtrs[i]->tFloodFillCodeAreas = 0;
	}

	gnFloodFillMaxScale = RYUMAX(sz.width, sz.height);
	gnFloodFillMatMaxSize = sz.width * sz.height;
	gnFloodFillStatMaxCount = max_count;

	gucFloodFillImage = (unsigned char *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(unsigned char), MAX_CACHE_LINE, NULL);
	if(!gucFloodFillImage) {
		return -12319011;
	}

	gnFloodFillLabelMat = (int *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	if(!gnFloodFillLabelMat) {
		return -12319003;
	}

	gptFloodFillSeeds = (RyuPoint *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(RyuPoint), MAX_CACHE_LINE, NULL);
	if(!gptFloodFillSeeds) {
		return -12319004;
	}

	gtFloodFillStatistics = (FFN *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(FFN), MAX_CACHE_LINE, NULL);
	if(!gtFloodFillStatistics) {
		return -12319005;
	}

	gnFloodFillContourMat = (int *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	if(!gnFloodFillContourMat) {
		return -12319006;
	}

	gnFloodFillContourFlag = (int *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillMaxScale, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	if(!gnFloodFillContourFlag) {
		return -12319007;
	}

	gnFloodFillClassSeeds = (int *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(int), MAX_CACHE_LINE, NULL);
	if(!gnFloodFillClassSeeds) {
		return -12319008;
	}

	gtFloodFillClassClusters = (FFC *)Memory_alloc(heap_id,
			ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(FFC), MAX_CACHE_LINE, NULL);
	if(!gtFloodFillClassClusters) {
		return -12319009;
	}

	gtFloodFillCodeAreas = (CAN *)Memory_alloc(heap_id,
			32 * sizeof(CAN), MAX_CACHE_LINE, NULL);
	if(!gtFloodFillCodeAreas) {
		return -12319010;
	}

	gptFloodFillContourPts = gptFloodFillSeeds;

	globalPtrs[0]->nFloodFillMatMaxSize = gnFloodFillMatMaxSize;
	globalPtrs[0]->nFloodFillStatMaxCount = gnFloodFillStatMaxCount;
	globalPtrs[0]->ucFloodFillImage = gucFloodFillImage;
	globalPtrs[0]->ptFloodFillSeeds = (int *)gptFloodFillSeeds;
	globalPtrs[0]->nFloodFillLabelMat = gnFloodFillLabelMat;
	globalPtrs[0]->tFloodFillStatistics = (int *)gtFloodFillStatistics;
	globalPtrs[0]->nFloodFillContourMat = gnFloodFillContourMat;
	globalPtrs[0]->nFloodFillContourFlag = gnFloodFillContourFlag;
	globalPtrs[0]->ptFloodFillContourPts = (int *)gptFloodFillContourPts;
	globalPtrs[0]->nFloodFillClassSeeds = gnFloodFillClassSeeds;
	globalPtrs[0]->tFloodFillClassClusters = (int *)gtFloodFillClassClusters;
	globalPtrs[0]->tFloodFillCodeAreas = (int *)gtFloodFillCodeAreas;

	return 1;
}

int WaybillSegment_release(IHeap_Handle heap_id)
{
	if(gucFloodFillImage) {
		Memory_free(heap_id, gucFloodFillImage, ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(unsigned char));
		gucFloodFillImage = 0;
	}

	if(gnFloodFillLabelMat) {
		Memory_free(heap_id, gnFloodFillLabelMat, ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(int));
		gnFloodFillLabelMat = 0;
	}

	if(gptFloodFillSeeds) {
		Memory_free(heap_id, gptFloodFillSeeds, ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(RyuPoint));
		gptFloodFillSeeds = 0;
	}

	if(gtFloodFillStatistics) {
		Memory_free(heap_id, gtFloodFillStatistics, ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(FFN));
		gtFloodFillStatistics = 0;
	}

	if(gnFloodFillContourMat) {
		Memory_free(heap_id, gnFloodFillContourMat, ROUNDUP(gnFloodFillMatMaxSize, MAX_CACHE_LINE) * sizeof(int));
		gnFloodFillContourMat = 0;
	}

	if(gnFloodFillContourFlag) {
		Memory_free(heap_id, gnFloodFillContourFlag, ROUNDUP(gnFloodFillMaxScale, MAX_CACHE_LINE) * sizeof(int));
		gnFloodFillContourFlag = 0;
	}

	if(gnFloodFillClassSeeds) {
		Memory_free(heap_id, gnFloodFillClassSeeds, ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(int));
		gnFloodFillClassSeeds = 0;
	}

	if(gtFloodFillClassClusters) {
		Memory_free(heap_id, gtFloodFillClassClusters, ROUNDUP(gnFloodFillStatMaxCount, MAX_CACHE_LINE) * sizeof(FFC));
		gtFloodFillClassClusters = 0;
	}

	if(gtFloodFillCodeAreas) {
		Memory_free(heap_id, gtFloodFillCodeAreas, 32 * sizeof(CAN));
		gtFloodFillCodeAreas = 0;
	}

	gnWbsegInitFlag = 0;

	return 1;
}
