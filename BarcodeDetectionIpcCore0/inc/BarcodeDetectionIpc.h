/*
 * BarcodeDetectionIpc.h
 *
 *  Created on: 2015-12-1
 *      Author: windlyu
 */

#ifndef BARCODEDETECTIONIPC_H_
#define BARCODEDETECTIONIPC_H_

#include <c6x.h>
#include <xdc/std.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>

#define  MAX_BARCODE_COUNT			(128)
#define  MAX_SLAVE_BARCODE_COUNT	(18)

#define NTHREADS 				(7)
#define NCORENUM 				(NTHREADS+1)

// �㷨�������ýṹ��
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// ��ʶ��������: 0-������, >0-�涨����(��С��MAX_BARCODE_COUNT����ֵ����ǰΪ128)

	int nCodeSymbology;		// ��������: 0-������, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-����25, (1<<4)-EAN13;
							//   ʹ��"��λ��"����϶�������, ��((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// �������ַ���λ��: 0-������,�㷨�Խ��λ��(<=32)���޶�;
							//    >0-�㷨�Խ��λ��(<=32)�����޶������֧��32�ֲ�ͬ��λ��(1~32)
							//    (1<<(n-1)) = ֧�ֽ���ַ���λ��Ϊn���������
							//    ʹ��"��λ��"����϶�������
							//    ��������������޶�Ϊ1λ��12λ��32λ���֣�����nCodeDgtNum=(1<<0) | (1<<11) | (1<<31)

	int nCodeDgtNumExt;		// �������ַ���λ�����䣬��[nCodeDgtNum]�ڴ���32λʱ�����䣬��ǰ�޹��ܣ�Ԥ�����ã�Ĭ��Ϊ0

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

	int nCodeValidityExt;  	// �ַ���Ч�����䣬��ǰ�޹��ܣ�Ԥ�����ã�Ĭ��Ϊ0

	int nMultiPkgDetect;  	// �����Ԥ������: 0-�ر�; ��0-����

} AlgorithmParamSet;

// �㷨���н������ṹ��
typedef struct tagAlgorithmResult
{
	int  nFlag;				// ʶ���־
	int  nCodeSymbology;	// �������ͣ������������ա��㷨�������ýṹ�塿��˵��
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
	int  nCodeModuleWid;	// �滻Ԥ��3�����뵥λ���*1024
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

typedef struct tagCalibrateResult
{
	int  nFlag;				// ��־λ				-- ���ڱ�ʶ�˴α궨����Ƿ���Ч��1-��Ч������-��Ч
	// ��ע������������������1�������������Ƚ�����2����������ԱȶȽ�����3��ȡֵ��Χ��Ϊ0~255
	int  nCurrQuality;		// ��ǰ��������			-- ����������������1����ǰֵ
	int  nLmaxQuality;		// ���ε��������������ֵ	-- ����������������1�����ֵ������λ��
	int  nSignQuality;		// ���������ı䷽��		-- ����������������1����ɫ��-1Ϊ��ɫ��1Ϊ��ɫ
	int  nCurrLuminance;	// ��ǰ��������			-- ���������Ƚ�����2����ǰֵ
	int  nCurrContrast;		// ��ǰ����Աȶ�		-- ������ԱȶȽ�����3����ǰֵ
	int  nMaxGreyLevel;		// ���Ҷ�ֵ			-- �����Ҷ�ֵ��
	int  nMinGreyLevel;		// ��С�Ҷ�ֵ			-- ����С�Ҷ�ֵ��
	int  nWhiteLevel;		// �����ɫ���ͻҶ�ֵ		-- ����ɫ����ֵ��
	int  nBlackLevel;		// �����ɫ���ͻҶ�ֵ		-- ����ɫ����ֵ��
	int  reserve1;  		// Ԥ��1
	int  reserve2;			// Ԥ��2
	int  reserve3;  		// Ԥ��3
	int  reserve4;  		// Ԥ��4
	int  reserve5;  		// Ԥ��5
	int  reserve6;	 		// Ԥ��6
	int  reserve7;  		// Ԥ��7
	int  reserve8;  		// Ԥ��8
} CalibrateResult;

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
    unsigned char *				uData1;
    unsigned char *				uData2;
    unsigned char *				uData3;
    unsigned char *				uData4;
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

// ���ܣ���������궨����
// ˵������ÿ���궨���̿�ʼǰ���������궨��ť�󣩣����д����ú���
void BarcodeDetect_calibration_reset();

// ���ܣ�����궨�㷨
// ˵��������궨���̿�ʼ������ɼ�ͼ��������֡������㷨���㷨�����궨���������������궨���棬�Թ���У��Ա�ο��������
// ˵�������ڸ��㷨����ʱ��϶�(<0.1ms)���ʲ����ж�˷ַ����㷨ֱ����core0����
// ˵�������㷨ռ��core0��̬Ƭ���ڴ�Լ32�ֽڣ�����ʱ��̬����Ƭ���ڴ�Լ2kb
// ����������ṹ��������㷨�����߶��岢�����ڴ棬����ʱ������ָ�봫���㷨
int BarcodeDetect_calibration(unsigned char * img_data, int img_wid, int img_hei, CalibrateResult * results);

int BarcodeDetect_init_ipcMasterProc0(IHeap_Handle heap_id, int img_max_wid, int img_max_hei);

barcode_detect_proc_info_t ** BarcodeDetect_getIpcProcInfo();

int BarcodeDetect_init_ipcMasterProc1();

void BarcodeDetect_release();

int BarcodeDetect_setparams(AlgorithmParamSet * paramset);

int BarcodeDetect_getparams(AlgorithmParamSet * paramset);

void BarcodeDetect_run_ipcSlaveProc(barcode_detect_proc_info_t * proc_info);

// ���ܣ���ͼ��ָ�롢�����������Core1��Core1��ͼ�����ݷֿ顢���������׼���ַ���Core1~7
// ˵�����˽�������ʱ��Core2~7��������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc0(int lrning_flag, unsigned char * img_data, int img_wid, int img_hei);

// ���ܣ�����һ�׶ηֿ�õ�ͼ�񣬷ַ���Core1~7�����С���������λ�㷨step1��
// ��������λ�㷨step1������ͼ�񣬼��㵥λ�����ڵ��ݶ�����ֵ
// ˵�����˽�������ʱ��Core1~7�Դ�������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc1();

// ���ܣ����ռ���Core1~7���з��ؽ����������Core1��Core1�Խ�����й鼯�ʹ�������������λ�㷨step2������Ȼ��������ٻ��֣�׼���ַ���Core1~7
// ��������λ�㷨step2���ϲ��ݶ�����ֱ��ͼ�������ݶ�������ֵ
// ˵�����˽�������ʱ��Core2~7��������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc2();

// ���ܣ�����һ�׶λ��ֺõ����ݣ��ַ���Core1~7�����С���������λ�㷨step3��
// ��������λ�㷨step3��������ֵɸѡ����������ȡ�������������������ݶȷ����
// ˵�����˽�������ʱ��Core1~7�Դ�������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc3();

// ���ܣ����ռ���Core1~7���з��ؽ����������Core1��Core1�Խ�����й鼯�ʹ�������������λ�㷨step4������Ȼ��������ٻ��֣�׼���ַ���Core1~7
// ��������λ�㷨step4������������࣬�γɺ�ѡ��������
// ˵�����˽�������ʱ��Core2~7��������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc4();

// ���ܣ������ֺõ����ݣ���ѡ�������򣩣��ַ���Core1~7�����С�����ָ��㷨step1��
// ����ָ��㷨step1����Ը���ѡ��������ȥ��������Ϣ�������ܴ����ؽ�����ָ����
// ˵�����˽�������ʱ��Core1~7�е����ɴ�������״̬�������Ӻ�ѡ������������������������߳�
int BarcodeDetect_run_ipcMasterProc5();

// ���ܣ����ռ���Core1~7���з��ؽ����������Core1��Core1�Խ�����й鼯�ʹ���������ָ��㷨step2������Ȼ��������ٻ��֣�׼���ַ���Core1~7
// ����ָ��㷨step2�����Ϸָ����룬����
// ˵�����˽�������ʱ��Core2~7��������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc6();

// ���ܣ������ֺõ����ݣ�����ָ�ͼ�񣩣��ַ���Core1~7�����С�����ʶ���㷨��
// ����ʶ���㷨������ͼ��У����Ԥ��������ʶ��
// ˵�����˽�������ʱ��Core1~7�е����ɴ�������״̬������������ָ�����������������߳�
int BarcodeDetect_run_ipcMasterProc7();

// ���ܣ����ռ���Core1~7���з��ؽ����������Core1��Core1�Խ�����й鼯�ʹ�����������ʶ���������С����������㷨��
// ˵�����˽�������ʱ��Core2~7��������״̬�������߳�
int BarcodeDetect_run_ipcMasterProc8();

// ���ܣ�������ʶ��������
int BarcodeDetect_run_ipcMasterProc9(unsigned char ** results);

#ifdef DEBUG_OUTPUT_IMAGE
void getDebugOutputImage(unsigned char ** output_images, int * output_wid, int * output_hei, int * output_flags);
#endif


#endif /* BARCODEDETECTIONIPC_H_ */
