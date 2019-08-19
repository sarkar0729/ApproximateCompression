#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "bitUtils.h"
#include "bucket.h"
#include "bucketArray.h"

#define DEBUG 0

// Command to compile: gcc -std=gnu99 -c bucket.c

// This file contains functions related to bucketization.
// A bucket represents a range of floating point numbers
// The range 1.0 .. 2.0 is divided between 35 to 100
// buckets. More buckets are used when higher accuracy
// is sought. When a floating point number is converted
// to a bucket number and then re-converted back to a
// floating point number, there is an error, the maximum
// error is half the width of the bucket

// The function value_to_bucket takes as input a floating point
// in the range 1.0 .. 2.0 and returns the bucket number
uint8_t
value_to_bucket(float value)
{
uint8_t bucket;

	for (bucket = 0; bucket < BUCKET_COUNT; bucket++){

		if (value < bucket_arr[bucket])
			return bucket;
	}

	if (DEBUG)
		printf("Internal error at file %s line %d: input value out of range %f\n", __FILE__, __LINE__, value); 

	return INVALID_BUCKET;
}

// The function bucket_to_value takes as input a bucket number 
// and returns the mid point of the bucket
float
bucket_to_value(uint8_t bucket)
{
float prev;
float next;
float average;

	if (bucket == 0) {
		prev = 1.0;
		next = bucket_arr[0];
	}
	else {
		prev = bucket_arr[bucket - 1];
		next = bucket_arr[bucket];
	}

	average = (prev + next) / 2.0;
    return(average);
}

// The function bucketize converts a floating point array
// to an integer array, where each element represents the
// bucket number. The input array can contain numbers in 
// any range but the maximum number can not be larger than
// 2.0 * minimum number. THe elements of the array are
// mapped into the range 1.0 .. 2.0 by dividing with minimum
// and then converted using function value_to_bucket
uint8_t *
bucketize(uint32_t batch_size, float *input, float max, float min)
{
uint8_t bucket;
uint8_t *bucketized_array;
float val;
float val2;
float val3;
float err_percent;

	// RESOLVE: Print bucket delta statistics

	bucketized_array = malloc(batch_size * sizeof(uint8_t));
	if (bucketized_array == NULL) {
		if (DEBUG)
			printf("Internal error at file %s line %d: memory allocation failed\n",  __FILE__, __LINE__);
		return NULL;
	}

	// Sanity check
	if (max > (2.0 * min)) {
		if (DEBUG)
			printf("Internal error at file %s line %d: input of bucketize out of range\t",  __FILE__, __LINE__);
		return NULL;
	}

	for (int i = 0; i < batch_size; i++) {

		val = input[i] / min;
		if (val >= 2.0) {
			val = 1.99999999;
		}

		if (val < 1.0) {
			if (DEBUG) {
				printf("Internal error at file %s line %d: input of bucketize out of range\t",  __FILE__, __LINE__);
				printf("i = %d, val = %f, input = %f, min = %f\t", i, val, input[i], min);
			}
			return NULL;
		}

		bucket = value_to_bucket(val);
		if (bucket == INVALID_BUCKET)
			return NULL;

		bucketized_array[i] = bucket;

		if (DEBUG) {
			val2 = bucket_to_value(bucket);
			val3 = val2 * min;
			err_percent = fabs(((val3 - input[i]) * 100.0) / input[i]);

			printf("input[%d] = %.9f\t bucket = %d\tcompressed value = %.9f\terr_percent = %.9f\%\n", 
					i, bucket, input[i], val3, err_percent);
		}
	}

	return bucketized_array;
}

// If two successive numbers are more than DELTA_HIGH, no encoding is done
// Numbers are simply copied
#define DELTA_HIGH 20

// Bucket values are compressed using a delta approach, that is a number
// is represented as delta with respect to the previous value. The delta
// is encoded using variable number of bits. One of about 20 encoding
// schemes are considered and one of them is chosen by this function
uint8_t
bucket_analyze(uint16_t len, uint8_t *buf)
{
int n;
int abs_n;
int retval;
int delta_zero;
int delta_plus[DELTA_HIGH + 1];
int delta_minus[DELTA_HIGH + 1];
int max_delta_plus;
int max_delta_minus;
int max_delta;

	delta_zero = 0;
	for (int i = 0; i < DELTA_HIGH + 1; i++) {
		delta_plus[i] = 0;
		delta_minus[i] = 0;
	}

	for (int i = 1; i < len; i++) {
		n = buf[i] - buf[i - 1];
		abs_n = abs(n);
		if (abs_n > DELTA_HIGH) {
			if (DEBUG) {
				printf("bucket_analyze: Found delta = %d at position %d, ", n, i);
				printf("encode key = 0\n");
			}
			return 0;
		}

		if (n == 0)
			delta_zero++;
		else if (n > 0)
			delta_plus[abs_n]++;
		else
			delta_minus[abs_n]++;
	}

	// Find highest +ve delta
	max_delta_plus = 0;
	for (int i = DELTA_HIGH; i > 0; i--) {
		if (delta_plus[i] > 0) {
			max_delta_plus = i;
			break;
		}
	}

	// Find highest -ve delta
	max_delta_minus = 0;
	for (int i = DELTA_HIGH; i > 0; i--) {
		if (delta_minus[i] > 0) {
			max_delta_minus = i;
			break;
		}
	}

	if (DEBUG) {
		printf("Highest positive delta = %d\n", max_delta_plus);
		printf("Highest negative delta = %d\n", max_delta_minus);
	}

	if (max_delta_plus >= max_delta_minus)
		max_delta = max_delta_plus;
	else
		max_delta = max_delta_minus;

	// Print count of different values: 0, +ve and -ve
	if (DEBUG) {
		printf("delta = 0, count = %d\n", delta_zero);
		for (int i = 1; i < max_delta + 1; i++) {
			printf("delta = +%d, count = %d\n", i, delta_plus[i]);
			printf("delta = -%d, count = %d\n", i, delta_minus[i]);
		}
	}

	// Delta statistics has been collected, now decide which encode
	// key value would be optimal for this sequence
	//
	// Explanation of encode key values
	//
	//  0: delta encoding not profitable
	//  1: delta values 0, +1, -1 
	//    Encoding 0, 10, 11
	//  2: delta values 0, +1, -1, +2, -2 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 1110, 1111
	//  3: delta values 0, +1, -1, +2, -2 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 1110, 1111
	//  4: delta values 0, +1, -1, +2, -2, +3, -3 [count of +1 >= count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 11110, 11111
	//  5: delta values 0, +1, -1, +2, -2, +3, -3 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 11110, 11111
	//  6: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 111110, 111111
	//  7: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 111110, 111111
	//  8: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111
	//  9: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110, 1111111
	// 10: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 11110XX, 11111XX
	// 11: delta values 0, +1, -1, +2, -2, +3 .. +6, -3 .. -6 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 11110XX, 11111XX
	// 12: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 11110XXX, 11111XXX
	// 13: delta values 0, +1, -1, +2, -2, +3 .. +10, -3 .. -10 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 11110XXXX, 11111XXXX
	// 14: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
	// 15: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +12, -5 .. -12 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXX, 1111111XXX
	// 16: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of +1 > count of -1]
	//    Encoding 0, 10, 110, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX
	// 17: delta values 0, +1, -1, +2, -2, +3, -3, +4, -4, +5 .. +20, -5 .. -20 [count of -1 > count of +1]
	//    Encoding 0, 110, 10, 11100, 11101, 111100, 111101, 1111100, 1111101, 1111110XXXX, 1111111XXXX
	// 18: delta values 0, +1, -1, +2, -2, +3, -3, +4 .. +19, -4 .. -19
	//    Encoding 000, 001, 010, 011, 100, 101, 110, 111, 1110XXXX, 1111XXXX
	//

	retval = 0; // Initialize, just in case

	if (max_delta_plus < 2 && max_delta_minus < 2) {
		retval = 1;
	}
	else if ((max_delta_plus == 2 && max_delta_minus <= 2) ||
			(max_delta_minus == 2 && max_delta_plus <= 2)) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 2;
			else
				retval = 3;
	}
	else if ((max_delta_plus == 3 && max_delta_minus <= 3) ||
			(max_delta_minus == 3 && max_delta_plus <= 3)) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 4;
			else
				retval = 5;
	}
	else if ((max_delta_plus == 4 && max_delta_minus <= 4) ||
			(max_delta_minus == 4 && max_delta_plus <= 4)) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 6;
			else
				retval = 7;
	}
	else if ((max_delta_plus == 5 && max_delta_minus <= 5) ||
			(max_delta_minus == 5 && max_delta_plus <= 5)) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 8;
			else
				retval = 9;
	}
	else if (max_delta_plus < 7 && max_delta_minus < 7) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 10;
			else
				retval = 11;
	}
	else if (max_delta_plus < 11 && max_delta_minus < 11) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 12;
			else
				retval = 13;
	}
	else if (max_delta_plus < 13 && max_delta_minus < 13) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 14;
			else
				retval = 15;
	}
	else if (max_delta_plus < 21 && max_delta_minus < 21) {
			if (delta_plus[1] >= delta_minus[1])
				retval = 16;
			else
				retval = 17;
	}

	if (DEBUG)
		printf("encode key = %d\n", retval);

	return retval;
}

