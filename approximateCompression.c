#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "approximateCompression_internal.h"
#include "bitUtils.h"
#include "bucket.h"
#include "uint8.h"

// Command to compile: gcc -std=gnu99 -c approximateCompression.c

#define VERBOSE 0
#define DEBUG 0

// BLOCK_SIZE is the maximum size of data that can be compressed
// without divided into multiple blocks
#define BLOCK_SIZE (256 * 1024)

// Compressed FP array structure
// Size of the compressed array in bytes uint32_t
// meta data, for example size (16/32/64), maximum err (1/0.5/0.25 etc) uint32_t
// Number of elements N uint32_t
// Number of batches n uint32_t
// Repeated n times
//   Number of elements in this batch uint16_t
//   Max and Min for this batch 32|64 bit
//   Type of encoding used in this batch uint8_t
//   Number of bytes in this batch uint16_t
//   Encoded bit representation for each element

/*
** This function accepts as input a floating point array
** and returns an opaque structure (array of bytes) containing
** the compressed array. The first 4 bytes contain the length
** N followed by N bytes. It can be uncompressed by calling
** function uncompress_float and passing on the pointer to the
** opaque structure.
**
** In case of any error, a NULL pointer is returned
**
** The compression is approximate and the accuracy is specified by
** the third parameter. Possible values are ACCURACY_HALF_PERCENT,
** ACCURACY_QUARTER_PERCENT and ACCURACY_ONE_TENTH_PERCENT. The
** average error is 0.5% / 0.25% / 0.1% and the maximum error is 
** guaranteed to be lower that 1% / 0.5% /0.2% respectively.
**
** High level algorithm:
**	- Divide the input floating point numbers into batches.
**	  A batch is a sequence of numbers such that maximum
**	  number is equal to or less than 2 X minimum number
**  - The range of the batch is then sub divided into buckets.
**	  All numbers in a bucket are approximated with the mid
**	  point of the bucket. The bucket width is chosed such that
**	  the maximum error is slightly lower than 1%.
**	- In next step, the input floating point array is bucketized
**    that is each number is replaced by the bucket number to
**	  which it belongs. Bucket numbers are 8 bit unsigned integers.
**	- The final stage is to reduce the memory required by using
**	  delta encoding. Each bucket number within a batch is replaced
**	  with the delta, that is difference with respect to previous
**	  bucket number. The delta is encoded representation of the
**	  difference, typically encoded using 1 to 4 bits. Potentially
**	  each batch of numbers can be encoded differently
*/
compressed_array
approximate_compress(uint32_t elem_count, uint8_t precision, uint8_t accuracy, float *input)
{
uint8_t *bucketized_array;
float min;
float max;
float *p_float;
uint8_t *batch_ptr;
uint8_t encoded_bucket[UINT16_MAX];
uint8_t output_bucket[BLOCK_SIZE];
uint8_t *output;
uint8_t batch_encode_key;
uint16_t encoded_size;
uint16_t batch_size;
uint16_t batch_count;
uint32_t metadata;
uint32_t elem_processed;
uint32_t start;
uint32_t end;
uint32_t output_size;
uint32_t byte_count;
uint16_t *p_val16;
uint32_t *p_val32;

	// Compressed FP array structure
	// Size of the compressed array in bytes uint32_t
	// Meta data, for example size (16/32/64), maximum err (1/0.5/0.25 etc) uint32_t
	// Number of elements N uint32_t
	// Number of batches n uint32_t
	// Repeated n times
	//   Number of elements in this batch uint16_t
	//   Max and Min for this batch 32|64 bit
	//   Type of encoding used in this batch uint8_t
	//   Number of bytes in this batch uint16_t
	//   Encoded bit representation for each element

	// The first four elements of compressed buffer to be filled later with the size
	// of the compressed structure, metadata, number of elements and number of batches

	p_val32 = (uint32_t *) output_bucket;
	*p_val32++ = 0; 
	*p_val32++ = 0;
	*p_val32++ = 0; 
	*p_val32++ = 0; 

	// batch_ptr will be used to fillup the output bucket
	batch_ptr = (uint8_t *) p_val32;

	start = 0;
	batch_count = 0;

	// Loop through all batches. A batch is a sequence such that
	// all numbers are within a range of min .. 2 * min

	do {
		// Started processing a new batch
		batch_count++;

		// Initialize batch_size, in case all the numbers are within
		// the range min .. 2 * min. This happens for the last batch
		// or if there is only one batch. Needed because the for loop
		// below does not initialize the batch_size, if all the remaining
		// numbers are within the range min .. 2 * min

		batch_size = elem_count - start; 

		// Take care of some special cases now

		// No element left after the last batch
		if (batch_size == 0) 
			break;

		// Just one element left after the last batch
		// or the next element is 0.0, which forms a
		// batch of its own
		if (batch_size == 1) {
			// In this case, there will be no encoding the
			// lone element will be put in place of max
			p_val16 = (uint16_t *) batch_ptr;
			*p_val16 = 1;
			batch_ptr = batch_ptr + sizeof(uint16_t);
			p_float = (float *) batch_ptr;
			*p_float++ = input[start];
			batch_ptr = (uint8_t *) p_float;

			if (VERBOSE)
				printf("Batch # %d has one element = %.9f\n", (batch_count - 1), input[start]);

			break;
		}

		// Just two elements left after the last batch
		if (batch_size == 2) {
			// Create a mini batch of size two
			// In this case, there will be no encoding, the
			// two elements will be put in place of max and min
			p_val16 = (uint16_t *) batch_ptr;
			*p_val16 = 2;
			batch_ptr = batch_ptr + sizeof(uint16_t);
			p_float = (float *) batch_ptr;
			*p_float++ = input[start];
			*p_float++ = input[start + 1];
			batch_ptr = (uint8_t *) p_float;

			if (VERBOSE)
				printf("Batch # %d has two elements, %.9f, %.9f\n", (batch_count - 1), input[start], input[start + 1]);

			break;
		}

		// RESOLVE: Do not terminate a batch due to presence of 0.0
		// Have a special bucket for 0.0

		if (input[start] == 0.0) {
			// In this case, there will be no encoding the
			// lone element will be put in place of max
			p_val16 = (uint16_t *) batch_ptr;
			*p_val16 = 1;
			batch_ptr = batch_ptr + sizeof(uint16_t);
			p_float = (float *) batch_ptr;
			*p_float++ = 0.0;
			batch_ptr = (uint8_t *) p_float;
			start += 1;

			if (VERBOSE)
				printf("Batch # %d has one element = %.9f\n", (batch_count - 1), input[start]);

			continue;
		} else if (input[start + 1] == 0.0) {
			// Create a mini batch of size two
			// In this case, there will be no encoding, 0.0 and the
			// other element will be put in place of max and min
			p_val16 = (uint16_t *) batch_ptr;
			*p_val16 = 2;
			batch_ptr = batch_ptr + sizeof(uint16_t);
			p_float = (float *) batch_ptr;
			*p_float++ = input[start];
			*p_float++ = input[start + 1];
			batch_ptr = (uint8_t *) p_float;
			start += 2;

			if (VERBOSE)
				printf("Batch # %d has two elements, %.9f, %.9f\n", (batch_count - 1), input[start], input[start + 1]);

			continue;
		}

		if (input[start] > input[start + 1]) {
			max = input[start];
			min = input[start + 1];
		} else {
			max = input[start + 1];
			min = input[start];
		}

		// RESOLVE: Change hard coded 2 to number of elements that need to be skipped
		// to find non-zero max and min
		for (int i = start + 2; i < elem_count; i++) {
			// A batch can not be more than 65536
			if (i >= start + UINT16_MAX) {
				batch_size = i - start;
				break;
			}

			if (input[i] == 0.0) {
				batch_size = i - start;
				break;
			}

			if (input[i] > max) {
				if (input[i] >= 2.0 * min) {
					batch_size = i - start;
					break;
				}
				max = input[i];
			}
			if (input[i] < min) {
				if (input[i] <= 0.5 * max) {
					batch_size = i - start;
					break;
				}
				min = input[i];
			}
		}

		// At this pointed we have just identified a batch. In some
		// case, the entire input array may be a batch, but a batch 
		// may not have more than 65536 (Max uint16_t) elements
		
		if (VERBOSE)
			printf("Batch # %d has %d elements, max = %.9f, min = %.9f\n", (batch_count - 1), batch_size, max, min);

		p_val16 = (uint16_t *) batch_ptr;
		*p_val16 = batch_size;
		batch_ptr = batch_ptr + sizeof(uint16_t);

		p_float = (float *) batch_ptr;
		*p_float++ = max;
		*p_float++ = min;
		batch_ptr = (uint8_t *) p_float;

		// RESOLVE: The parameter precision is not used by bucketize, set to zero
		bucketized_array = bucketize(batch_size, input + start, max, min, 0, accuracy);
		batch_encode_key = bucket_analyze(batch_size, bucketized_array);

		*batch_ptr++ = batch_encode_key;

		if (batch_encode_key == 0) {
			// Delta encoding is not possible, copy the bucketized array
			for (int i = 0; i < batch_size; i++)
				batch_ptr[i] = bucketized_array[i];

			batch_ptr += batch_size;
		} else {
			uint8_encode(batch_encode_key, batch_size, bucketized_array, encoded_bucket);

			// The first two bytes of the encoded buffer contains the size.
			// first retrieve the size and then the rest of the buffer

			p_val16 = (uint16_t *) encoded_bucket;
			encoded_size = *p_val16;
			if (DEBUG)
				printf("encoded size = %d\n", encoded_size);
			// Check for error
			if (encoded_size == 0) {
				free(bucketized_array);
				return NULL;
			}
				
			for (int i = 0; i < encoded_size; i++)
				batch_ptr[i] = encoded_bucket[i];

			batch_ptr += encoded_size;
		}

		if (DEBUG)
			printf("start = %d\tend = %d\tsize = %d\n", start, start + batch_size - 1, batch_size);

		start += batch_size;

		// Release the bucketized array to prevent memory leakage
		free(bucketized_array);

	} while (start < elem_count);

	output_size = batch_ptr - output_bucket;

	if (VERBOSE)
		printf("Compressed %d elements in %d batches, output size = %d bytes\n", elem_count, batch_count, output_size);

	// Finally patch the header of the compressed structure
	// with the size of the encoded buffer, total number of
	// floating point elements in the input array and the 
	// number of batches 
	
	metadata = (precision << 3) |accuracy;

	if (DEBUG)
		printf("precision = 0x%X, accuracy = 0x%X, metadata = 0x%X\n", precision, accuracy, metadata);

	p_val32 = (uint32_t *) output_bucket;
	*p_val32++ = output_size;
	*p_val32++ = metadata;
	*p_val32++ = elem_count;
	*p_val32 = batch_count;

	output = malloc(output_size);
	if (output == NULL)
		return NULL;

	memcpy(output, output_bucket, output_size);

	return (compressed_array) output;
}

/*
** This function accepts as input an opaque structure 
** (array of bytes) containing a compressed array, previously
** generated using uncompress_float function. The first 
** 4 bytes contain the length N followed by N bytes. It returns
** the uncompressed array, the first 4 bytes contain the length 
** N followed by N floating point numbers.
**
** In case of any error, a NULL pointer is returned
**
** High level algorithm:
**  - From the header of the compressed structure, retrieve
**    the total number of floating point numbers present
**	  and the number of batches
**	- For each batch perform following steps:
**		- Decode the encoded bits to get the bucket numbers
**		- Convert the bucket numbers to floating point numbers
**		- Append the numbers to the uncompressed array
*/

uint8_t *
decompress_float(compressed_array input)
{
uint8_t encoded_buffer[UINT16_MAX];
uint8_t decoded_buffer[UINT16_MAX];
uint8_t uncompressed_buffer[BLOCK_SIZE];
float min;
float max;
uint16_t batch_size;
uint32_t batch_size_in_bytes;
uint16_t total_size;
uint16_t encoded_buffer_size;
uint8_t encode_key;
uint32_t batch_count;
uint32_t input_size;
uint32_t elem_count;
uint32_t metadata;
uint8_t precision;
uint8_t accuracy;
uint8_t *input_ptr;
uint8_t *output_ptr;
float *output_ptr_float;
double *output_ptr_double;
double tmp_double;
uint8_t *output;
uint32_t output_size;
uint32_t *p_val32;;
uint16_t *p_val16;;
float *p_float;;
int status;

	input_ptr = (uint8_t *)input; // Make a copy of the input pointer
	output_ptr = uncompressed_buffer;

	// Compressed FP array structure
	// Size of the compressed array in bytes uint32_t
	// Meta data, for example size (16/32/64), maximum err (1/0.5/0.25 etc) uint32_t
	// Number of elements N uint32_t
	// Number of batches n uint32_t
	// Repeated n times
	//   Number of elements in this batch uint16_t
	//   Max and Min for this batch 32|64 bit
	//   Encoding key for this batch uint8_t
	//   Number of bytes in this batch uint16_t
	//   Encoded bit representation for each element

	p_val32 = (uint32_t *) input_ptr;
	input_size = *p_val32++;
	metadata = *p_val32++;
	elem_count = *p_val32++;
	batch_count = *p_val32++;
	input_ptr = (uint8_t *) p_val32;

	accuracy = metadata & 0b111;
	precision = (metadata >> 3) & 0b111;

	// Validate accuracy and precision
	if ((precision != PRECISION_SINGLE) && (precision != PRECISION_DOUBLE)) {
		if (DEBUG)
			printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
		return NULL;
	}

	if ((accuracy != ACCURACY_HALF_PERCENT) && (accuracy != ACCURACY_QUARTER_PERCENT) 
			&& (accuracy != ACCURACY_ONE_TENTH_PERCENT)) {
		if (DEBUG)
			printf("Internal error: %s at line %d\n", __FILE__, __LINE__);
		return NULL;
	}

	if (DEBUG) {
		printf("Compressed file: input_size =%d metadata = %d elem_count = %d ", 
				input_size, metadata, elem_count);
		printf("batch_count = %d precision = %d accuracy = %d\n", 
				batch_count, precision, accuracy);
	}

	total_size = 0;

	// Loop through all batches. A batch is a sequence such that
	// all numbers are within a range of min .. 2 * min

	for (int i = 0; i < batch_count; i++) {
		// Started processing a new batch
		p_val16 = (uint16_t *) input_ptr;
		batch_size = *p_val16++;
		input_ptr = (uint8_t *) p_val16;

		// Take care of the special case when the batch has
		// Just one or two elements. Nothing to be decoded
		if (batch_size == 1 || batch_size == 2) {
			// Write as float or double
			p_float = (float *) input_ptr;
			if (precision == PRECISION_SINGLE) {
				output_ptr_float = (float *) output_ptr;

				*output_ptr_float++ = *p_float++;
				if (batch_size == 2) {
					*output_ptr_float++ = *p_float++;
				}

				output_ptr = (uint8_t *) output_ptr_float;
			} else {
				output_ptr_double = (double *) output_ptr;

				tmp_double = *p_float++;
				*output_ptr_double++ = tmp_double;

				if (batch_size == 2) {
					tmp_double = *p_float++;
					*output_ptr_double++ = tmp_double;
				}

				output_ptr = (uint8_t *) output_ptr_double;
			}

			input_ptr = (uint8_t *) p_float;

			// Update the number of elements processed so far
			total_size += batch_size;

			if (VERBOSE)
				printf("Batch #%d has %d elements\n", i, batch_size);

			continue;
		}

		// Update the number of elements processed so far
		total_size += batch_size;

		p_float = (float *) input_ptr;
		max = *p_float++;
		min = *p_float++;
		input_ptr = (uint8_t *) p_float;

		encode_key = *input_ptr++;

		if (VERBOSE)
			printf("Batch #%d has %d elements, max = %.9f, min = %.9f\n", i, batch_size, max, min);
		if (DEBUG)
			printf("Batch #%d encoded using encode key = %d\n", i, encode_key);

		// Validate batch_size, just in case
		if (batch_size == 0) {
			if (DEBUG)
				printf("Batch #%d has size zero\n", i);
			return NULL;
		}

		if (encode_key == 0) {
			memcpy(decoded_buffer, input_ptr, batch_size);
			input_ptr += batch_size;
		} else {
			p_val16 = (uint16_t *) input_ptr;
			encoded_buffer_size = *p_val16++;
			input_ptr = (uint8_t *) p_val16;

			if (DEBUG)
				printf("encoded size in bytes = %d\n", encoded_buffer_size);

			p_val16 = (uint16_t *) encoded_buffer;
			*p_val16 = batch_size;

			encoded_buffer_size = encoded_buffer_size - 2;

			memcpy(encoded_buffer + 2, input_ptr, encoded_buffer_size);
			input_ptr += (encoded_buffer_size);

			status = uint8_decode(encode_key, batch_size, encoded_buffer, decoded_buffer);
			if (status == (-1))
				return NULL;
		}

		unbucketize(batch_size, decoded_buffer, (uint8_t *)output_ptr, min, precision, accuracy);

		if (precision == PRECISION_SINGLE)
			batch_size_in_bytes = batch_size * sizeof(float);
		else // PRECISION_DOUBLE
			batch_size_in_bytes = batch_size * sizeof(double);

		output_ptr += batch_size_in_bytes;
	}

	// The size specified in the encoded buffer should match
	// the number of elements found during decoding
	if (total_size == elem_count) {
		if (precision == PRECISION_SINGLE)
			output_size = elem_count * sizeof(float);
		else // PRECISION_DOUBLE
			output_size = elem_count * sizeof(double);


		output = malloc(output_size + sizeof(uint32_t));
		// Quit if can not allocate memory
		if (output == NULL)
			return NULL;

		// Copy the size followed by the float array
		p_val32 = (uint32_t *) output;
		*p_val32 = output_size;
		memcpy(output + sizeof(uint32_t), uncompressed_buffer, output_size);

		return output;
	} else
		// Some thing went wrong
		if (DEBUG)
			printf("mismatch in total_size (%d) and elem_count (%d)\n", total_size, elem_count);
		return NULL;

}

compressed_array
compress_float(uint32_t elem_count, uint8_t accuracy, float *input)
{
	return(approximate_compress(elem_count, PRECISION_SINGLE, accuracy, input));
}

compressed_array
compress_double(uint32_t elem_count, uint8_t accuracy, double *input)
{
float *input2;

	input2 = malloc(elem_count * sizeof(float));
	if (input2 == NULL)
		return NULL;

	for (int i = 0; i < elem_count; i++) {
		input2[i] = (float) input[i];
	}

	// RESOLVE: Free input2
	return (approximate_compress(elem_count, PRECISION_DOUBLE, accuracy, input2));
}

uint32_t
get_compressed_length(compressed_array c)
{
uint32_t *p;

	p = (uint32_t *) c;

    if (p == NULL)
		return 0;

    return(*p);
}

