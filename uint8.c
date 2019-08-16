#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "uint8.h"

#define DEBUG 0

// This file contains two key functions encode and decode used to compress
// and decompress a uint8_t array and few helper functions. The encode 
// function typically encodes a uint8_t number to 2-4 bits. The decode 
// function maps those bits to the original value. This works when the 
// difference between successive values (delta) are within +20 or -20. 
// Best result is obtained when the delta is smaller, for example less 
// than 5. There are about 20 different encoding strategies used. Before
// encoding, one of them is chosen depending on the statistical property
// of the input array. It is not the most optimal encoding strategy but 
// usually is close to the optimal. The advantage of this approach is that
// no mapping table is needed and therefore small batches can be encoded
//
// Command to compile: gcc -std=gnu99 -c uint8.c

// Explanation of encode key values
//
// 0: delta encoding not profitable
// 1: delta values 0, +1, -1 
//    Encoding 0, 10, 11
// 2: delta values 0, +1, -1, +2, -2 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 1110, 1111
// 3: delta values 0, +1, -1, +2, -2 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 1110, 1111
// 4: delta values 0, +1, -1, +2, -2, +3, -3 [count of +1 >= count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 11110, 11111
// 5: delta values 0, +1, -1, +2, -2, +3, -3 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 11110, 11111
// 6: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 111110, 111111
// 7: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 111110, 111111
// 8: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111
// 9: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111
// A: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 11110XX, 11111XX
// B: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 11110XX, 11111XX
// C: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 11110XXX, 11111XXX
// D: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 11110XXXX, 11111XXXX
// E: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
// F: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
// G: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of +1 > count of -1]
//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX
// H: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of -1 > count of +1]
//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX
// I: delta values 0, +1, -1, +2, -2, +3, -3, +4 .. +19, -4 .. -19
//    Encoding 000, 001, 010, 011, 100, 101, 110, 111, 1110XXXX, 1111XXXX
//

#define DELTA_HIGH 20

//
//  Input:
// 	len, buf: length and location of the array that has to be encoded.
// 	encode_key: Decides the encoding strategy to be used
// 	Output:
// 	encoded buffer: The encoded is returned at the location pointed by
// 	encoded_buff. First two bytes contain the length followed by encoded
// 	bits. In case of any error, the length is set to zero. It is the 
// 	responsibility of the caller to provide space for the buffer and free it.
//
// Calls a number of lower level functions for doing the actual encoding
void
uint8_encode(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
	switch(encode_key){
		case  1: 
		case  2: 
		case  3: uint8_encode_1_2_3(encode_key, len, buf, encoded_buf);
				return;
		case  4: 
		case  5: uint8_encode_4_5(encode_key, len, buf, encoded_buf);
				return;
		case  6:
		case  7:
		case  8: 
		case  9: uint8_encode_6_7_8_9(encode_key, len, buf, encoded_buf);
				return;
		case 10:
		case 11:
		case 12: 
		case 13: uint8_encode_10_11_12_13(encode_key, len, buf, encoded_buf);
				return;
		case 14:
		case 15:
		case 16: 
		case 17: uint8_encode_14_15_16_17(encode_key, len, buf, encoded_buf);
				return;
		default: 
				printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
				// Set the length to zero to indicate that error happened
				encoded_buf[0] = 0;
				encoded_buf[1] = 0;
				return;
		}
}

// Encode input buffer when the encode key is 1 | 2 | 3
void
uint8_encode_1_2_3(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
uint32_t encoded_buf_idx;
uint16_t encoded_buf_len;
uint8_t *final_encoded_buf;
uint64_t value;
int16_t delta;
uint16_t *p_val16;

	// Set the length of encoded buffer uint16_t to zero
	// Will be filled up later with correct values
	encoded_buf[0] = 0;
	encoded_buf[1] = 0;

	// Set the following byte with the first bucket value
	encoded_buf[2] = buf[0];

	encoded_buf_idx = 24; // Points to the next bit position 

	// 2: delta values 0, +1, -1, +2, -2 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 1110, 1111
	// 3: delta values 0, +1, -1, +2, -2 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 1110, 1111

	for (int i = 1; i < len; i++) {
		delta = buf[i] - buf[i - 1];
		switch(delta){
			case 0: clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 1: 
					if (encode_key == 1 || encode_key == 2) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else { 
						// encode_key == 3
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case (-1): 
					if (encode_key == 1) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else if (encode_key == 2) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else {
						// encode_key == 3
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case 2: // Append bits 1110
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case (-2): // Append bits 1111
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			default: 
					printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
					return;
		}
	}

	if (DEBUG)
		printf("Encoded bit count = %d\n", encoded_buf_idx);

	// Check whether the last byte of encoded_buf is partially filled
	// and clear the unfilled portion 
	while ((encoded_buf_idx % 8) != 0) {
		clear_bit(encoded_buf, encoded_buf_idx);
		encoded_buf_idx++;
	}

	encoded_buf_len = encoded_buf_idx / 8;

	if (DEBUG)
		printf("Encoded buffer length = %d\n", encoded_buf_len);

	// Patch the first word with the length of the buffer
	p_val16 = (uint16_t *) encoded_buf;
	*p_val16 = encoded_buf_len;
}

// Encode input buffer when the encode key is 4 | 5 
void
uint8_encode_4_5(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
uint32_t encoded_buf_idx;
uint16_t encoded_buf_len;
uint8_t *final_encoded_buf;
uint64_t value;
int16_t delta;
uint16_t *p_val16;

	// Set the length of encoded buffer uint16_t to zero
	// Will be filled up later with correct values
	encoded_buf[0] = 0;
	encoded_buf[1] = 0;

	// Set the following byte with the first bucket value
	encoded_buf[2] = buf[0];

	encoded_buf_idx = 24; // Points to the next bit position 

	// 4: delta values 0, +1, -1, +2, -2, +3, -3 [count of +1 >= count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 11110, 11111
	// 5: delta values 0, +1, -1, +2, -2, +3, -3 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 11110, 11111

	for (int i = 1; i < len; i++) {
		delta = buf[i] - buf[i - 1];
		switch(delta){
			case 0: clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 1: 
					if (encode_key == 4) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else { 
						// encode_key == 5
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case (-1): 
					if (encode_key == 4) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else {
						// encode_key == 5
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case 2: // Append bits 11100
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case (-2): // Append bits 11101
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 3: // Append bits 11110
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case (-3): // Append bits 11111
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			default: 
					printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
					return;
		}
	}

	if (DEBUG)
		printf("Encoded bit count = %d\n", encoded_buf_idx);

	// Check whether the last byte of encoded_buf is partially filled
	// and clear the unfilled portion 
	while ((encoded_buf_idx % 8) != 0) {
		clear_bit(encoded_buf, encoded_buf_idx);
		encoded_buf_idx++;
	}

	encoded_buf_len = encoded_buf_idx / 8;
	if (DEBUG)
		printf("Encoded buffer length = %d\n", encoded_buf_len);

	// Patch the first word with the length of the buffer
	p_val16 = (uint16_t *) encoded_buf;
	*p_val16 = encoded_buf_len;
}

// Encode input buffer when the encode key is 6 | 7 | 8 | 9 
void
uint8_encode_6_7_8_9(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
uint32_t encoded_buf_idx;
uint16_t encoded_buf_len;
uint8_t *final_encoded_buf;
uint64_t value;
int16_t delta;
uint16_t *p_val16;

	// Set the length of encoded buffer uint16_t to zero
	// Will be filled up later with correct values
	encoded_buf[0] = 0;
	encoded_buf[1] = 0;

	// Set the following byte with the first bucket value
	encoded_buf[2] = buf[0];

	encoded_buf_idx = 24; // Points to the next bit position 

	// 6: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 111110, 111111
	// 7: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 111110, 111111
	// 8: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111
	// 9: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111

	for (int i = 1; i < len; i++) {
		delta = buf[i] - buf[i - 1];
		switch(delta){
			case 0: clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 1: 
					if (encode_key == 6 || encode_key ==8) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else { 
						// encode_key == 7 || encode_key == 9
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case (-1): 
					if (encode_key == 6 || encode_key ==8) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else {
						// encode_key == 7 || encode_key == 9
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case 2: // Append bits 11100
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case (-2): // Append bits 11101
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 3: // Append bits 111100
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case (-3): // Append bits 111101
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 4: 
					if (encode_key == 6 || encode_key == 7) {
					// Append bits 111110
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					} else {
					// encode_key == 8 || encode_key == 9
					// Append bits 1111100
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					}
					break;
			case (-4):
					if (encode_key == 6 || encode_key == 7) {
					// Append bits 111111
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					} else {
					// encode_key == 8 || encode_key == 9
					// Append bits 1111101
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					}
					break;
			case 5: 
					// Append bits 1111110
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case -5: 
					// Append bits 1111111
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					set_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			default: 
					printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
					return;
		}
	}

	if (DEBUG)
		printf("Encoded bit count = %d\n", encoded_buf_idx);

	// Check whether the last byte of encoded_buf is partially filled
	// and clear the unfilled portion 
	while ((encoded_buf_idx % 8) != 0) {
		clear_bit(encoded_buf, encoded_buf_idx);
		encoded_buf_idx++;
	}

	encoded_buf_len = encoded_buf_idx / 8;
	if (DEBUG)
		printf("Encoded buffer length = %d\n", encoded_buf_len);

	// Patch the first word with the length of the buffer
	p_val16 = (uint16_t *) encoded_buf;
	*p_val16 = encoded_buf_len;
}

// Encode input buffer when the encode key is 10 | 11 | 12 | 13 
void
uint8_encode_10_11_12_13(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
uint32_t encoded_buf_idx;
uint16_t encoded_buf_len;
uint8_t *final_encoded_buf;
uint64_t value;
int16_t delta;
uint16_t *p_val16;

	// Set the length of encoded buffer uint16_t to zero
	// Will be filled up later with correct values
	encoded_buf[0] = 0;
	encoded_buf[1] = 0;

	// Set the following byte with the first bucket value
	encoded_buf[2] = buf[0];

	encoded_buf_idx = 24; // Points to the next bit position 

	// 10: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of +1 > count of -1]
	//     Encoding 0, 10, 110, 11100, 11101, 11110XX, 11111XX
	// 11: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of -1 > count of +1]
	//     Encoding 0, 110, 10, 11100, 11101, 11110XX, 11111XX
	// 12: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of +1 > count of -1]
	//     Encoding 0, 10, 110, 11100, 11101, 11110XXX, 11111XXX
	// 13: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of -1 > count of +1]
	//     Encoding 0, 110, 10, 11100, 11101, 11110XXXX, 11111XXXX

	for (int i = 1; i < len; i++) {
		delta = buf[i] - buf[i - 1];
		switch(delta){
			case 0: clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 1: 
					if (encode_key == 10 || encode_key == 12) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else { 
						// encode_key == 11 or encode_key == 13
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case (-1): 
					if (encode_key == 10 || encode_key == 12) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else {
						// encode_key == 11 or encode_key == 13
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case 2: value = 0b00111;
					write_bitstream(encoded_buf, encoded_buf_idx, 5, value);
					encoded_buf_idx += 5;
					break;
			case (-2): value = 0b10111;
					write_bitstream(encoded_buf, encoded_buf_idx, 5, value);
					encoded_buf_idx += 5;
					break;
			default: 
					if (encode_key == 10 || encode_key == 11) {
						value = (delta > 0) ? 0b01111 : 0b11111;
						value += (abs(delta) - 3) << 5;
						write_bitstream(encoded_buf, encoded_buf_idx, 7, value);
						encoded_buf_idx += 7;
					} else {
						// encode_key == 12 or encode_key == 13
						value = (delta > 0) ? 0b01111 : 0b11111;
						value += (abs(delta) - 3) << 5;
						write_bitstream(encoded_buf, encoded_buf_idx, 8, value);
						encoded_buf_idx += 8;
					}
					break;
		}
	}

	if (DEBUG)
		printf("Encoded bit count = %d\n", encoded_buf_idx);

	// Check whether the last byte of encoded_buf is partially filled
	// and clear the unfilled portion 
	while ((encoded_buf_idx % 8) != 0) {
		clear_bit(encoded_buf, encoded_buf_idx);
		encoded_buf_idx++;
	}

	encoded_buf_len = encoded_buf_idx / 8;
	if (DEBUG)
		printf("Encoded buffer length = %d\n", encoded_buf_len);

	// Patch the first word with the length of the buffer
	p_val16 = (uint16_t *) encoded_buf;
	*p_val16 = encoded_buf_len;
}

// Encode input buffer when the encode key is 14 | 15 | 16 | 17 
void
uint8_encode_14_15_16_17(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf)
{
uint32_t encoded_buf_idx;
uint16_t encoded_buf_len;
uint8_t *final_encoded_buf;
uint64_t value;
int16_t delta;
uint16_t *p_val16;

	// Set the length of encoded buffer uint16_t to zero
	// Will be filled up later with correct values
	encoded_buf[0] = 0;
	encoded_buf[1] = 0;

	// Set the following byte with the first bucket value
	encoded_buf[2] = buf[0];

	encoded_buf_idx = 24; // Points to the next bit position 

	// E: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
	// F: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
	// G: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX
	// H: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX

	for (int i = 1; i < len; i++) {
		delta = buf[i] - buf[i - 1];
		switch(delta){
			case 0: clear_bit(encoded_buf, encoded_buf_idx);
					encoded_buf_idx++;
					break;
			case 1: 
					if (encode_key == 14 || encode_key == 16) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else { 
						// encode_key == 15 || encode_key == 17
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case (-1): 
					if (encode_key == 14 || encode_key == 16) {
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					} else {
						// encode_key == 15 || encode_key == 17
						set_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
						clear_bit(encoded_buf, encoded_buf_idx);
						encoded_buf_idx++;
					}
					break;
			case 2: value = 0b00111;
					write_bitstream(encoded_buf, encoded_buf_idx, 5, value);
					encoded_buf_idx += 5;
					break;
			case (-2): value = 0b10111;
					write_bitstream(encoded_buf, encoded_buf_idx, 5, value);
					encoded_buf_idx += 5;
					break;
			case 3: value = 0b001111;
					write_bitstream(encoded_buf, encoded_buf_idx, 6, value);
					encoded_buf_idx += 6;
					break;
			case (-3): value = 0b101111;
					write_bitstream(encoded_buf, encoded_buf_idx, 6, value);
					encoded_buf_idx += 6;
					break;
			case 4: value = 0b0011111;
					write_bitstream(encoded_buf, encoded_buf_idx, 7, value);
					encoded_buf_idx += 7;
					break;
			case (-4): value = 0b1011111;
					write_bitstream(encoded_buf, encoded_buf_idx, 7, value);
					encoded_buf_idx += 7;
					break;
						value = (delta > 0) ? 0b01111 : 0b11111;
						value += (abs(delta) - 3) << 5;
						write_bitstream(encoded_buf, encoded_buf_idx, 7, value);
						encoded_buf_idx += 7;
			default: 
					value = (delta > 0) ? 0b0111111 : 0b1111111;
					if (encode_key == 14 || encode_key == 15) {
						value += (abs(delta) - 5) << 7;
						write_bitstream(encoded_buf, encoded_buf_idx, 10, value);
						encoded_buf_idx += 10;
					} else {
						// encode_key == 16 || encode_key == 17
						value += (abs(delta) - 5) << 7;
						write_bitstream(encoded_buf, encoded_buf_idx, 11, value);
						encoded_buf_idx += 11;
					}

					break;
		}
	}

	if (DEBUG)
		printf("Encoded bit count = %d\n", encoded_buf_idx);

	// Check whether the last byte of encoded_buf is partially filled
	// and clear the unfilled portion 
	while ((encoded_buf_idx % 8) != 0) {
		clear_bit(encoded_buf, encoded_buf_idx);
		encoded_buf_idx++;
	}

	encoded_buf_len = encoded_buf_idx / 8;
	if (DEBUG)
		printf("Encoded buffer length = %d\n", encoded_buf_len);

	// Patch the first word with the length of the buffer
	p_val16 = (uint16_t *) encoded_buf;
	*p_val16 = encoded_buf_len;
}

//
//  Input:
// 	encoded buffer: First two bytes contain the length of encoded bits
// 	followed by encoded bits. 
// 	batch_size: number of elements in the encoded buffer
// 	encode_key: contains the same key that was used to encode
//
// 	Output:
// 	decoded buffer: 
// 	First two bytes contain the length N followed by N floating point
// 	numbers.
//
//  Returns:
//  0: Decoding successful
//  -1: Error
//
// Calls a number of lower level functions for doing the actual encoding
int
uint8_decode(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
	if (DEBUG)
		printf("uint8_decode: encode_key = %d\n", encode_key);

	switch(encode_key){
		case  1: 
		case  2: 
		case  3: return(uint8_decode_1_2_3(encode_key, batch_size, encoded_buffer, decoded_buffer));

		case  4: 
		case  5: return(uint8_decode_4_5(encode_key, batch_size, encoded_buffer, decoded_buffer));

		case  6: 
		case  7: 
		case  8: 
		case  9: return(uint8_decode_6_7_8_9(encode_key, batch_size, encoded_buffer, decoded_buffer));
				
		case 10: 
		case 11: 
		case 12: 
		case 13: return(uint8_decode_10_11_12_13(encode_key, batch_size, encoded_buffer, decoded_buffer));
				
		case 14: 
		case 15: 
		case 16: 
		case 17: return(uint8_decode_14_15_16_17(encode_key, batch_size, encoded_buffer, decoded_buffer));
		
		default: return (-1);
	}
}

// Decode encoded buffer when the encode key is 1 | 2 | 3
int
uint8_decode_1_2_3(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
uint32_t encoded_buffer_idx;
uint64_t val64;
uint8_t val8;
uint16_t *p_val16;
uint16_t encoded_buffer_size;
int delta;
int val;

	// Save the encoded length for diagnostics
	p_val16 = (uint16_t *)encoded_buffer;
	encoded_buffer_size = *p_val16++;
	encoded_buffer = (uint8_t *)p_val16;

	encoded_buffer_idx = 0;

	decoded_buffer[0] = *encoded_buffer;
	encoded_buffer_idx += 8;
	for (int i = 1; i < batch_size; i++) {
		if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
			// The first bit is 0, no need to go further
			decoded_buffer[i] = decoded_buffer[i - 1];
			encoded_buffer_idx++;
			continue;
		} else {
			// The first bit is 1, check next bit
			encoded_buffer_idx++;
			if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
				// The first two bits are 10 
				if (encode_key == 1 || encode_key == 2)
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				else {
					// encode_key == 3
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				}

				encoded_buffer_idx++;
				continue;
			} else {
				// The first two bits are 11
				if (encode_key == 1) {
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;

					encoded_buffer_idx++;
					continue;
				}

				encoded_buffer_idx++;
				if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
					// The first three bits are 110
					if (encode_key == 2) {
						val = decoded_buffer[i - 1];
						val--;
						decoded_buffer[i] = val;
					} else {
						// encode_key == 3
						decoded_buffer[i] = decoded_buffer[i - 1] + 1;
					}
					encoded_buffer_idx++;
					continue;
				} else {
					// The first three bits are 111, check next bit
					encoded_buffer_idx++;
					if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
						// The first four bits are 1110, delta = 2
						decoded_buffer[i] = decoded_buffer[i - 1] + 2;
					} else {
						// The first four bits are 1111, delta = (-2)
						val = decoded_buffer[i - 1];
						val--;
						val--;
						decoded_buffer[i] = val;
					}
					encoded_buffer_idx++;
					continue;
				}
			}
		}
	}

	return 0;
}

// Decode encoded buffer when the encode key is 1 | 2 | 3
int
uint8_decode_4_5(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
uint32_t encoded_buffer_idx;
uint64_t val64;
uint8_t val8;
uint16_t *p_val16;
uint16_t encoded_buffer_size;
int delta;
int val;
int bit3;
int bit4;

	// Save the encoded length for diagnostics
	p_val16 = (uint16_t *)encoded_buffer;
	encoded_buffer_size = *p_val16++;
	encoded_buffer = (uint8_t *)p_val16;

	encoded_buffer_idx = 0;

	decoded_buffer[0] = *encoded_buffer;
	encoded_buffer_idx += 8;
	for (int i = 1; i < batch_size; i++) {
		if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
			// The first bit is 0, no need to go further
			decoded_buffer[i] = decoded_buffer[i - 1];
			encoded_buffer_idx++;
			continue;
		} else {
			// The first bit is 1, check next bit
			encoded_buffer_idx++;
			if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
				// The first two bits are 10 
				if (encode_key == 4)
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				else {
					// encode_key == 5
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				}

				encoded_buffer_idx++;
				continue;
			} else {
				// The first two bits are 11, check next bit
				encoded_buffer_idx++;
				if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
					// The first three bits are 110
					if (encode_key == 4) {
						val = decoded_buffer[i - 1];
						val--;
						decoded_buffer[i] = val;
					} else {
						// encode_key == 5
						decoded_buffer[i] = decoded_buffer[i - 1] + 1;
					}
					encoded_buffer_idx++;
					continue;
				} else {
					// The first three bits are 111, check next bit
					encoded_buffer_idx++;
					bit3 = get_bit(encoded_buffer, encoded_buffer_idx);
					encoded_buffer_idx++;
					bit4 = get_bit(encoded_buffer, encoded_buffer_idx);
					encoded_buffer_idx++;
					if (bit3 == 0) {
						// The first four bits are 1110
						if (bit4 == 0) {
							// The first five bits are 11100
							decoded_buffer[i] = decoded_buffer[i - 1] + 2;
						} else {
							// The first five bits are 11101
							val = decoded_buffer[i - 1];
							val--;
							val--;
							decoded_buffer[i] = val;
						}
					} else {
						// The first four bits are 1111
						if (bit4 == 0) {
							// The first five bits are 11110
							decoded_buffer[i] = decoded_buffer[i - 1] + 3;
						} else {
							// The first five bits are 11111
							val = decoded_buffer[i - 1];
							val -= 3;
							decoded_buffer[i] = val;
						}
					}
					continue;
				}
			}
		}
	}
	return 0;
}

// Decode encoded buffer when the encode key is 6 | 7 | 8 | 9
int
uint8_decode_6_7_8_9(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
uint32_t encoded_buffer_idx;
uint64_t val64;
uint8_t val8;
uint16_t *p_val16;
uint16_t encoded_buffer_size;
int delta;
int val;
int bit3;
int bit4;
int bit5;
int bit6;
int bit7;

	// Save the encoded length in bytes for diagnostics
	p_val16 = (uint16_t *)encoded_buffer;
	encoded_buffer_size = *p_val16++;
	encoded_buffer = (uint8_t *)p_val16;

	encoded_buffer_idx = 0;

	decoded_buffer[0] = *encoded_buffer;
	encoded_buffer_idx += 8;
	for (int i = 1; i < batch_size; i++) {
		if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
			// The first bit is 0, no need to go further
			decoded_buffer[i] = decoded_buffer[i - 1];
			encoded_buffer_idx++;
			continue;
		} else {
			// The first bit is 1, check next bit
			encoded_buffer_idx++;
			if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
				// The first two bits are 10 
				if (encode_key == 6 || encode_key == 8)
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				else {
					// encode_key == 7 || encode_key == 9
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				}

				encoded_buffer_idx++;
				continue;
			} else {
				// The first two bits are 11, check next bit
				encoded_buffer_idx++;
				if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
					// The first three bits are 110
					if (encode_key == 6 || encode_key == 8) {
						val = decoded_buffer[i - 1];
						val--;
						decoded_buffer[i] = val;
					} else {
						// encode_key == 7 || encode_key == 9
						decoded_buffer[i] = decoded_buffer[i - 1] + 1;
					}
					encoded_buffer_idx++;
					continue;
				} else {
					// The first three bits are 111, check next two bits
					encoded_buffer_idx++;
					bit4 = get_bit(encoded_buffer, encoded_buffer_idx);
					encoded_buffer_idx++;
					bit5 = get_bit(encoded_buffer, encoded_buffer_idx);
					encoded_buffer_idx++;
					if (bit4 == 0) {
						// The first four bits are 1110
						if (bit5 == 0) {
							// The first five bits are 11100
							decoded_buffer[i] = decoded_buffer[i - 1] + 2;
						} else {
							// The first five bits are 11101
							val = decoded_buffer[i - 1];
							val--;
							val--;
							decoded_buffer[i] = val;
						}
					} else {
						// The first four bits are 1111
						bit6 = get_bit(encoded_buffer, encoded_buffer_idx);
						encoded_buffer_idx++;
						if (bit5 == 0) {
							// The first five bits are 11110
							if (bit6 == 0) {
								// The first six bits are 111100
								decoded_buffer[i] = decoded_buffer[i - 1] + 3;
							} else {
								// The first six bits are 111101
								val = decoded_buffer[i - 1];
								val -= 3;
								decoded_buffer[i] = val;
							}
						} else {
							// The first five bits are 11111
							if (encode_key == 6 || encode_key == 7) {
								// The first five bits are 11110
								if (bit6 == 0) {
									// The first six bits are 111110
									decoded_buffer[i] = decoded_buffer[i - 1] + 4;
								} else {
									// The first six bits are 111111
									val = decoded_buffer[i - 1];
									val -= 4;
									decoded_buffer[i] = val;
								}
							} else {
								// (encode_key == 8 || encode_key == 9)
								bit7 = get_bit(encoded_buffer, encoded_buffer_idx);
								encoded_buffer_idx++;
								if (bit6 == 0) {
									// The first six bits are 111110
									if (bit7 == 0) {
										// The first seven bits are 1111100
										decoded_buffer[i] = decoded_buffer[i - 1] + 4;
									} else {
										// The first seven bits are 1111101
										val = decoded_buffer[i - 1];
										val -= 4;
										decoded_buffer[i] = val;
									}
								} else {
									// The first six bits are 111111
									if (bit7 == 0) {
										// The first seven bits are 1111110
										decoded_buffer[i] = decoded_buffer[i - 1] + 5;
									} else {
										// The first seven bits are 1111111
										val = decoded_buffer[i - 1];
										val -= 5;
										decoded_buffer[i] = val;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

// Decode encoded buffer when the encode key is 10 | 11 | 12 | 13
int
uint8_decode_10_11_12_13(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
uint32_t encoded_buffer_idx;
uint64_t val64;
uint8_t val8;
uint16_t *p_val16;
uint16_t encoded_buffer_size;
int delta;
int val;

	// Save the encoded length for diagnostics
	p_val16 = (uint16_t *)encoded_buffer;
	encoded_buffer_size = *p_val16++;
	encoded_buffer = (uint8_t *)p_val16;

	encoded_buffer_idx = 0;

	// The first element is not encoded
	decoded_buffer[0] = *encoded_buffer;
	encoded_buffer_idx += 8;

	for (int i = 1; i < batch_size; i++) {
		if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
			// The first bit is 0, no need to go further
			decoded_buffer[i] = decoded_buffer[i - 1];
			encoded_buffer_idx++;
			continue;
		} else {
			// The first bit is 1, check next bit
			encoded_buffer_idx++;
			if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
				// The first two bits are 10 
				if (encode_key == 10 || encode_key == 12)
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				else {
					// encode_key == 11 || encode_key == 13
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				}

				encoded_buffer_idx++;
				continue;
			} else {
				// The first two bits are 11, check next bit
				encoded_buffer_idx++;
				if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
					// The first three bits are 110
				if (encode_key == 10 || encode_key == 12) {
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				} else {
					// encode_key == 11 || encode_key == 13
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				}
					encoded_buffer_idx++;
					continue;
				} else {
					// The first three bits are 111, check next bit
					encoded_buffer_idx++;
					if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
						// The first four bits are 1110, check next bit
						encoded_buffer_idx++;
						if (get_bit(encoded_buffer, encoded_buffer_idx) == 0)
							// Found 11100
							decoded_buffer[i] = decoded_buffer[i - 1] + 2;
						else {
							// Found 11101
							val = decoded_buffer[i - 1];
							val--;
							val--;
							decoded_buffer[i] = val;
						}
						encoded_buffer_idx++;
						continue;
					} else {
						// The first four bits are 1111, check next bit
						encoded_buffer_idx++;
						if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
							// The first five bits are 11110, depending on encode key
							// fetch next two or next three bits
							if (encode_key == 10 || encode_key == 11) {
								// Fetch next two bits
								encoded_buffer_idx++;
								read_bitstream(encoded_buffer, encoded_buffer_idx, 2, &val64);
								encoded_buffer_idx += 2;
							} else {
								// encode_key == 12 || encode_key == 13
								// Fetch next three bits
								encoded_buffer_idx++;
								read_bitstream(encoded_buffer, encoded_buffer_idx, 3, &val64);
								encoded_buffer_idx += 3;
							}

							val = val64;
							val += 3;
							val = decoded_buffer[i - 1] + val;
							decoded_buffer[i] = val;
						} else {
							// The first five bits are 11111, depending on encode key
							// fetch next two or next three bits
							if (encode_key == 10 || encode_key == 11) {
								// Fetch next two bits
								encoded_buffer_idx++;
								read_bitstream(encoded_buffer, encoded_buffer_idx, 2, &val64);
								encoded_buffer_idx += 2;
							} else {
								// encode_key == 12 || encode_key == 13
								// Fetch next three bits
								encoded_buffer_idx++;
								read_bitstream(encoded_buffer, encoded_buffer_idx, 3, &val64);
								encoded_buffer_idx += 3;
							}
							
							val = val64;
							val += 3;
							val = decoded_buffer[i - 1] - val;
							decoded_buffer[i] = val;
						}

						continue;
					}
				}
			}
		}
	}
	return 0;
}

// Decode encoded buffer when the encode key is 14 | 15 | 16 | 17
int
uint8_decode_14_15_16_17(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer)
{
uint32_t encoded_buffer_idx;
uint64_t val64;
uint8_t val8;
uint16_t *p_val16;
uint16_t encoded_buffer_size;
int delta;
int val;
int bit7;

	// Save the encoded length for diagnostics
	p_val16 = (uint16_t *)encoded_buffer;
	encoded_buffer_size = *p_val16++;
	encoded_buffer = (uint8_t *)p_val16;

	encoded_buffer_idx = 0;

	// The first element is not encoded
	decoded_buffer[0] = *encoded_buffer;
	encoded_buffer_idx += 8;

	for (int i = 1; i < batch_size; i++) {
		if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
			// The first bit is 0, no need to go further
			decoded_buffer[i] = decoded_buffer[i - 1];
			encoded_buffer_idx++;
			continue;
		} else {
			// The first bit is 1, check next bit
			encoded_buffer_idx++;
			if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
				// The first two bits are 10 
				if (encode_key == 14 || encode_key == 16)
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				else {
					// encode_key == 15 || encode_key == 17
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				}

				encoded_buffer_idx++;
				continue;
			} else {
				// The first two bits are 11, check next bit
				encoded_buffer_idx++;
				if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
					// The first three bits are 110
				if (encode_key == 14 || encode_key == 16) {
					val = decoded_buffer[i - 1];
					val--;
					decoded_buffer[i] = val;
				} else {
					// encode_key == 15 || encode_key == 17
					decoded_buffer[i] = decoded_buffer[i - 1] + 1;
				}
					encoded_buffer_idx++;
					continue;
				} else {
					// The first three bits are 111, check next bit
					encoded_buffer_idx++;
					if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
						// The first four bits are 1110, check next bit
						encoded_buffer_idx++;
						if (get_bit(encoded_buffer, encoded_buffer_idx) == 0)
							// Found 11100
							decoded_buffer[i] = decoded_buffer[i - 1] + 2;
						else {
							// Found 11101
							val = decoded_buffer[i - 1];
							val--;
							val--;
							decoded_buffer[i] = val;
						}
						encoded_buffer_idx++;
						continue;
					} else {
						// The first four bits are 1111, check next bit
						encoded_buffer_idx++;
						if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
							// The first five bits are 11110, check next bit
							encoded_buffer_idx++;
							if (get_bit(encoded_buffer, encoded_buffer_idx) == 0)
								// Found 111100
								decoded_buffer[i] = decoded_buffer[i - 1] + 3;
							else {
								// Found 111101
								val = decoded_buffer[i - 1];
								val--;
								val--;
								val--;
								decoded_buffer[i] = val;
							}
							encoded_buffer_idx++;
							continue;
						} else {
							// The first five bits are 11111, check next bit
							encoded_buffer_idx++;
							if (get_bit(encoded_buffer, encoded_buffer_idx) == 0) {
								// Found 111110
								encoded_buffer_idx++;
								if (get_bit(encoded_buffer, encoded_buffer_idx) == 0)
									// Found 1111100
									decoded_buffer[i] = decoded_buffer[i - 1] + 4;
								else {
									// Found 1111101
									val = decoded_buffer[i - 1];
									val--;
									val--;
									val--;
									val--;
									decoded_buffer[i] = val;
								}
								encoded_buffer_idx++;
								continue;
							} else {
								//  The first six bits are 111111, check next bit
								encoded_buffer_idx++;
								bit7 = get_bit(encoded_buffer, encoded_buffer_idx);
								// Depending on encode key, fetch next three or next four bits
								if (encode_key == 14 || encode_key == 15) {
									// Fetch next three bits
									encoded_buffer_idx++;
									read_bitstream(encoded_buffer, encoded_buffer_idx, 3, &val64);
									encoded_buffer_idx += 3;
								} else {
									// encode_key == 16 || encode_key == 17
									// Fetch next four bits
									encoded_buffer_idx++;
									read_bitstream(encoded_buffer, encoded_buffer_idx, 4, &val64);
									encoded_buffer_idx += 4;
								}

								if (bit7 == 0) {
									// The first seven bits are 1111110
									// Range is 5 .. 5 + 7  for encode_key == 14
									// Range is 5 .. 5 + 15 for encode_key == 14
									val = val64 + 5;
									decoded_buffer[i] = decoded_buffer[i - 1] + val;
								} else {
									// The first seven bits are 1111111
									// Range is -5 .. -5 - 7  for encode_key == 14
									// Range is -5 .. -5 - 15 for encode_key == 14
									val = val64 + 5;
									decoded_buffer[i] = decoded_buffer[i - 1] - val;
								}									
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

