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

// �㷨�������ýṹ��
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// ��ʶ��������: 0-������, >0-�涨����(��С��MAX_BARCODE_COUNT����ֵ)

	int nCodeSymbology;		// ��������: 0-������, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-����25, (1<<4)-EAN13;
							//   ʹ��"��λ��"����϶�������, ��((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// �������ַ���λ��: 0-������,�㷨�Խ��λ�����޶�;
							//    >0,�㷨�Խ��λ�������޶������֧��4�ֲ�ͬ��λ��
							//    ÿ������λ����byte����ʽ�ֱ𱣴���nCodeDgtNum��31~24λ��23~16λ��15~8λ��7~0λ��
							//    ��������������޶�Ϊ10λ��12λ��13λ������nCodeDgtNum=(10<<16) | (12<<8) | 13

	int nCodeValidity;	 	// �ַ���Ч��: 0-������, (1<<0)-����(ASCII 48~57), (1<<1)-Сд��ĸ(ASCII 97~122), (1<<2)-��д��ĸ(ASCII 65~90)
							//   (1<<3)-"space"(ASCII 32), (1<<4)-"!"(ASCII 33), (1<<5)-'"'(ASCII 34), (1<<6)-"#"(ASCII 35),
							//   (1<<7)-"$"(ASCII 36), (1<<8)-"%"(ASCII 37), (1<<9)-"&"(ASCII 38), (1<<10)-"'"(ASCII 39),
							//   (1<<11)-"("��")"(ASCII 40~41), (1<<12)-"*"(ASCII 42), (1<<13)-"+"(ASCII 43), (1<<14)-","(ASCII 44),
							//   (1<<15)-"-"(ASCII 45), (1<<16)-"."(ASCII 46), (1<<17)-"/"(ASCII 47), (1<<18)-":"(ASCII 58),
							//   (1<<19)-";"(ASCII 59), (1<<20)-"<"��">"(ASCII 60,62), (1<<21)-"="(ASCII 61), (1<<22)-"?"(ASCII 63),
							//   (1<<23)-"@"(ASCII 64), (1<<24)-"["��"]"(ASCII 91,93), (1<<25)-"\"(ASCII 92), (1<<26)-"^"(ASCII 94),
							//   (1<<27)-"_"(ASCII 95), (1<<28)-"`"(ASCII 96), (1<<29)-"{"��"}"(ASCII 123,125), (1<<30)-"|"(ASCII 124),
							//   (1<<31)-"~"(ASCII 126)
							//   ʹ��"��λ��"����϶����ַ�����, ��((1<<0) | (1<<1) | (1<<2)) = ֧�ְ������֡�Сд��ĸ�Լ���д��ĸ����������������඼��Ϊ�Ƿ��ַ����й���

	int nCodeValidityExt;  	// �ַ���Ч�����䣬Ԥ�����ã�Ĭ��Ϊ0

	int nMultiPkgDetect;  	// �����Ԥ������: 0-�ر�, ��0-����

	int reserve4;  			// Ԥ��4
} AlgorithmParamSet;

// �������ڵ�ṹ��
typedef struct tagAlgorithmResult
{
	int  nFlag;				// ʶ���־
	int  nCodeSymbology;	// ��������
	int  nCodeCharNum;		// �ַ�λ��
	char strCodeData[128];	// ������
	int  ptCodeCenter;		// ������������(��16:X,��16:Y)
	int  ptCodeBound1;		// ���붥������1
	int  ptCodeBound2;		// ���붥������2
	int  ptCodeBound3;		// ���붥������3
	int  ptCodeBound4;		// ���붥������4
	int  nCodeOrient;		// ������ת�Ƕ�
	//int  reserve1;  		// Ԥ��1
	int  nCodeWidth;		// �滻Ԥ��1��������
	//int  reserve2;		// Ԥ��2
	int  nCodeHeight;		// �滻Ԥ��2������߶�
	//int  reserve3;  		// Ԥ��3
	int  nCodeModuleWid;	// �滻Ԥ��3�����뵥λ���*1000
	//int  reserve4;  		// Ԥ��4
	int  nCodeSeqNum;		// �滻Ԥ��4���������
	//int  reserve5;  		// Ԥ��5
	char *pOcrIm;			// �滻Ԥ��5��ocrʶ���ȡͼ��
	//int  reserve6;	 	// Ԥ��6
	int  nOcrImWid;			// �滻Ԥ��6��ocrʶ���ȡͼ���
	//int  reserve7;  		// Ԥ��7
	int	 nOcrImHei;			// �滻Ԥ��7��ocrʶ���ȡͼ���
	//int  reserve8;  		// Ԥ��8
	int  nOcrExtra;			// �滻Ԥ��8��ocrʶ�������Ϣ
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
