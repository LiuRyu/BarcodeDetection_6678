/*
 * Decoder_code39.h
 *
 *  Created on: 2016-4-27
 *      Author: Ryu
 */

#ifndef DECODER_CODE39_H_
#define DECODER_CODE39_H_

#include "DspCommon.h"
#include "RyuCore.h"

int RecgCode39(int * decode_arr, int arr_count, char * code_result, int * code_digit,
	int * code_module, int * code_direct, int * code_idxL, int * code_idxR);


#endif /* DECODER_CODE39_H_ */
