/*
 * Decoder_I2of5.h
 *
 *  Created on: 2016-12-6
 *      Author: Ryu
 */

#ifndef DECODER_I2OF5_H_
#define DECODER_I2OF5_H_

#include "DspCommon.h"
#include "RyuCore.h"

int RecgCodeI2of5(int * decode_arr, int arr_count, char * code_result, int * code_digit,
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);

#endif /* DECODER_I2OF5_H_ */
