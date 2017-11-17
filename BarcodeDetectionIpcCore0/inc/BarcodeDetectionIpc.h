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

// 算法参数设置结构体
typedef struct tagAlgorithmParamSet
{
	int nFlag;
	int nCodeCount;  		// 待识别条码数: 0-无限制, >0-规定个数(须小于MAX_BARCODE_COUNT给定值，当前为128)

	int nCodeSymbology;		// 条码类型: 0-无限制, (1<<0)-code128, (1<<1)-code39, (1<<2)-code93, (1<<3)-交插25, (1<<4)-EAN13;
							//   使用"按位或"来组合多种类型, 如((1<<0) | (1<<1) | (1<<4)) = (ode128 + code39 + EAN13)

	int nCodeDgtNum;		// 解码结果字符串位数: 0-无限制,算法对结果位数(<=32)无限定;
							//    >0-算法对结果位数(<=32)进行限定，最多支持32种不同的位数(1~32)
							//    (1<<(n-1)) = 支持结果字符串位数为n的条码输出
							//    使用"按位或"来组合多种类型
							//    如设置输出条码限定为1位、12位及32位三种，可令nCodeDgtNum=(1<<0) | (1<<11) | (1<<31)

	int nCodeDgtNumExt;		// 解码结果字符串位数扩充，是[nCodeDgtNum]在大于32位时的扩充，当前无功能，预留待用，默认为0

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

	int nCodeValidityExt;  	// 字符有效性扩充，当前无功能，预留待用，默认为0

	int nMultiPkgDetect;  	// 多包裹预警开关: 0-关闭; 非0-开启

} AlgorithmParamSet;

// 算法运行结果输出结构体
typedef struct tagAlgorithmResult
{
	int  nFlag;				// 识别标志
	int  nCodeSymbology;	// 条码类型，结果对照请参照【算法参数设置结构体】的说明
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
	int  nCodeModuleWid;	// 替换预留3，条码单位宽度*1024
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

typedef struct tagCalibrateResult
{
	int  nFlag;				// 标志位				-- 用于标识此次标定结果是否有效，1-有效，否则-无效
	// 附注：【条码质量进度条1】、【条码亮度进度条2】、【条码对比度进度条3】取值范围皆为0~255
	int  nCurrQuality;		// 当前条码质量			-- 【条码质量进度条1】当前值
	int  nLmaxQuality;		// 本次调节条码质量最佳值	-- 【条码质量进度条1】最佳值，绿线位置
	int  nSignQuality;		// 条码质量改变方向		-- 【条码质量进度条1】颜色，-1为红色，1为绿色
	int  nCurrLuminance;	// 当前条码亮度			-- 【条码亮度进度条2】当前值
	int  nCurrContrast;		// 当前条码对比度		-- 【条码对比度进度条3】当前值
	int  nMaxGreyLevel;		// 最大灰度值			-- 【最大灰度值】
	int  nMinGreyLevel;		// 最小灰度值			-- 【最小灰度值】
	int  nWhiteLevel;		// 条码白色典型灰度值		-- 【白色典型值】
	int  nBlackLevel;		// 条码黑色典型灰度值		-- 【黑色典型值】
	int  reserve1;  		// 预留1
	int  reserve2;			// 预留2
	int  reserve3;  		// 预留3
	int  reserve4;  		// 预留4
	int  reserve5;  		// 预留5
	int  reserve6;	 		// 预留6
	int  reserve7;  		// 预留7
	int  reserve8;  		// 预留8
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

// 功能：重置相机标定进程
// 说明：在每个标定进程开始前（点击相机标定按钮后），运行此重置函数
void BarcodeDetect_calibration_reset();

// 功能：相机标定算法
// 说明：相机标定进程开始后，相机采集图像数据逐帧送入该算法，算法给出标定结果，呈现于相机标定界面，以供调校人员参考调节相机
// 说明：由于该算法运行时间较短(<0.1ms)，故不进行多核分发，算法直接在core0运行
// 说明：本算法占用core0静态片上内存约32字节，运行时动态申请片上内存约2kb
// 参数：结果结构体变量由算法调用者定义并分配内存，调用时将变量指针传入算法
int BarcodeDetect_calibration(unsigned char * img_data, int img_wid, int img_hei, CalibrateResult * results);

int BarcodeDetect_init_ipcMasterProc0(IHeap_Handle heap_id, int img_max_wid, int img_max_hei);

barcode_detect_proc_info_t ** BarcodeDetect_getIpcProcInfo();

int BarcodeDetect_init_ipcMasterProc1();

void BarcodeDetect_release();

int BarcodeDetect_setparams(AlgorithmParamSet * paramset);

int BarcodeDetect_getparams(AlgorithmParamSet * paramset);

void BarcodeDetect_run_ipcSlaveProc(barcode_detect_proc_info_t * proc_info);

// 功能：将图像指针、传入参数递至Core1，Core1将图像数据分块、参数打包，准备分发至Core1~7
// 说明：此进程运行时，Core2~7处于闲置状态，串行线程
int BarcodeDetect_run_ipcMasterProc0(int lrning_flag, unsigned char * img_data, int img_wid, int img_hei);

// 功能：将上一阶段分块好的图像，分发至Core1~7，运行‘条码区域定位算法step1’
// 条码区域定位算法step1：遍历图像，计算单位区域内的梯度特征值
// 说明：此进程运行时，Core1~7皆处于运行状态，并行线程
int BarcodeDetect_run_ipcMasterProc1();

// 功能：接收检验Core1~7运行返回结果，并递至Core1，Core1对结果进行归集和处理（‘条码区域定位算法step2’），然后对数据再划分，准备分发至Core1~7
// 条码区域定位算法step2：合并梯度特征直方图，计算梯度特征阈值
// 说明：此进程运行时，Core2~7处于闲置状态，串行线程
int BarcodeDetect_run_ipcMasterProc2();

// 功能：将上一阶段划分好的数据，分发至Core1~7，运行‘条码区域定位算法step3’
// 条码区域定位算法step3：根据阈值筛选特征区域，提取区域内其他特征，如梯度方向等
// 说明：此进程运行时，Core1~7皆处于运行状态，并行线程
int BarcodeDetect_run_ipcMasterProc3();

// 功能：接收检验Core1~7运行返回结果，并递至Core1，Core1对结果进行归集和处理（‘条码区域定位算法step4’），然后对数据再划分，准备分发至Core1~7
// 条码区域定位算法step4：特征区域聚类，形成候选条码区域
// 说明：此进程运行时，Core2~7处于闲置状态，串行线程
int BarcodeDetect_run_ipcMasterProc4();

// 功能：将划分好的数据（候选条码区域），分发至Core1~7，运行‘条码分割算法step1’
// 条码分割算法step1：针对各候选条码区域，去除冗余信息，尽可能纯净地将条码分割出来
// 说明：此进程运行时，Core1~7中的若干处于运行状态（数量视候选条码区域个数而定），并行线程
int BarcodeDetect_run_ipcMasterProc5();

// 功能：接收检验Core1~7运行返回结果，并递至Core1，Core1对结果进行归集和处理（‘条码分割算法step2’），然后对数据再划分，准备分发至Core1~7
// 条码分割算法step2：整合分割条码，排序
// 说明：此进程运行时，Core2~7处于闲置状态，串行线程
int BarcodeDetect_run_ipcMasterProc6();

// 功能：将划分好的数据（条码分割图像），分发至Core1~7，运行‘条码识别算法’
// 条码识别算法：条码图像校正，预处理，读码识别
// 说明：此进程运行时，Core1~7中的若干处于运行状态（数量视条码分割个数而定），并行线程
int BarcodeDetect_run_ipcMasterProc7();

// 功能：接收检验Core1~7运行返回结果，并递至Core1，Core1对结果进行归集和处理，整合条码识别结果，运行‘多包裹检测算法’
// 说明：此进程运行时，Core2~7处于闲置状态，串行线程
int BarcodeDetect_run_ipcMasterProc8();

// 功能：将条码识别结果传出
int BarcodeDetect_run_ipcMasterProc9(unsigned char ** results);

#ifdef DEBUG_OUTPUT_IMAGE
void getDebugOutputImage(unsigned char ** output_images, int * output_wid, int * output_hei, int * output_flags);
#endif


#endif /* BARCODEDETECTIONIPC_H_ */
