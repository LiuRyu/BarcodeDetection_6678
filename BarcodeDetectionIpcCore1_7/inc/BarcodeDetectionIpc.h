/*
 * BarcodeDetectionIpc.h
 *
 *  Created on: 2015-12-1
 *      Author: windlyu
 */
#ifndef BARCODEDETECTIONIPC_H_
#define BARCODEDETECTIONIPC_H_

#include "DspCommon.h"
#include "RyuCore.h"


#define  MAX_BARCODE_COUNT			(128)
#define  MAX_SLAVE_BARCODE_COUNT	(18)

// 算法参数设置结构体
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// 待识别条码数: 0-无限制, >0-规定个数(须小于MAX_BARCODE_COUNT给定值)

	int nCodeSymbology;		// 条码类型: 0-无限制, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-交插25, (1<<4)-EAN13;
							//   使用"按位或"来组合多种类型, 如((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// 解码结果字符串位数: 0-无限制,算法对结果位数无限定;
							//    >0,算法对结果位数进行限定，最多支持4种不同的位数
							//    每个设置位数以byte的形式分别保存在nCodeDgtNum的31~24位，23~16位，15~8位，7~0位中
							//    如设置输出条码限定为10位、12位及13位，可令nCodeDgtNum=(10<<16) | (12<<8) | 13

	int nCodeValidity;	 	// 字符有效性: 0-无限制, (1<<0)-数字(ASCII 48~57), (1<<1)-小写字母(ASCII 97~122), (1<<2)-大写字母(ASCII 65~90)
							//   (1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
							//   (1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
							//   (1<<11)-"("和")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
							//   (1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
							//   (1<<19)-";"(ASCII 59), (1<<20)-"<"和">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
							//   (1<<23)-"@"(ASCII 64), (1<<24)-"["和"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
							//   (1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"和"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
							//   (1<<31)-"~"(ASCII 126)
							//   使用"按位或"来组合多种字符类型, 如((1<<0) | (1<<1) | (1<<2)) = 支持包含数字、小写字母以及大写字母的条码结果输出，其余都视为非法字符进行过滤

	int nCodeValidityExt;  	// 字符有效性扩充，预留待用，默认为0

	int nMultiPkgDetect;  	// 多包裹预警开关: 0-关闭, 非0-开启

	int reserve4;  			// 预留4
} AlgorithmParamSet;

// 结果输出节点结构体
typedef struct tagAlgorithmResult
{
	int  nFlag;				// 识别标志
	int  nCodeSymbology;	// 条码类型
	int  nCodeCharNum;		// 字符位数
	char strCodeData[128];	// 解码结果
	int  ptCodeCenter;		// 条码中心坐标(高16:X,低16:Y)
	int  ptCodeBound1;		// 条码顶点坐标1
	int  ptCodeBound2;		// 条码顶点坐标2
	int  ptCodeBound3;		// 条码顶点坐标3
	int  ptCodeBound4;		// 条码顶点坐标4
	int  nCodeOrient;		// 条码旋转角度
	//int  reserve1;  		// 预留1
	int  nCodeWidth;		// 替换预留1，条码宽度
	//int  reserve2;		// 预留2
	int  nCodeHeight;		// 替换预留2，条码高度
	//int  reserve3;  		// 预留3
	int  nCodeModuleWid;	// 替换预留3，条码单位宽度*1000
	//int  reserve4;  		// 预留4
	int  nCodeSeqNum;		// 替换预留4，条码序号
	//int  reserve5;  		// 预留5
	char *pOcrIm;			// 替换预留5，ocr识别截取图像
	//int  reserve6;	 	// 预留6
	int  nOcrImWid;			// 替换预留6，ocr识别截取图像宽
	//int  reserve7;  		// 预留7
	int	 nOcrImHei;			// 替换预留7，ocr识别截取图像高
	//int  reserve8;  		// 预留8
	int  nOcrExtra;			// 替换预留8，ocr识别额外信息
} AlgorithmResult;

typedef enum barcode_detect_proc_type {
	null_proc,
	mapBarcodeGlobalPtrs,
	main_proc0,
	slave_proc0,
	main_proc1,
	slave_proc1,
	main_proc2,
	slave_proc2,
	main_proc3,
	slave_proc3,
	main_proc4
} barcode_detect_proc_type_e;

typedef struct barcode_detect_proc_info {
	int							nFlag;
	int							core_id;
	barcode_detect_proc_type_e 	proc_type;
    unsigned char *			uData1;
    unsigned char *			uData2;
    unsigned char *			uData3;
    unsigned char *			uData4;
    int *						nData1;
    int *						nData2;
    int *						nData3;
    int *						nData4;
    int							nVari1;
    int							nVari2;
    int							nVari3;
    int							nVari4;
    int							nVari5;
    int							nVari6;
    int							nVari7;
    int							nVari8;
    int							nVari9;
    int							nVari10;
} barcode_detect_proc_info_t;


void BarcodeDetect_run_ipcSlaveProc(barcode_detect_proc_info_t * proc_info);


#endif /* BARCODEDETECTIONIPC_H_ */
