#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "approximateCompression.h"

// For handling larger input files, increase the constant below
#define MAX_SIZE 65536

uint8_t *uncompress_float(uint8_t *input);

/*
** This program reads a compressed file previously generated using
** program compressFloat and generates approximate version of the
** original file containing floating point numbers using the function
** uncompress_float. It is a lossy compression with an average error
** of 0.5%. The maximum error is guaranteed to be below one percent. 
** Typical size for time series data is about 6% of the original size,
** in other words about 2 bits per floating point number (32 bit).
**
** Command to compile: gcc -std=gnu99 -o uncompressFloat uncompressFloatMain.c 
**                         uncompressFloatApproximate.o bitUtils.o bucket.o uint8.o
** Usage:    ./uncompressFloatMain compressed_file uncompressed_file
**
** The accuracy of compression can be checked using a program compareFloat.
*/

int
main(int argc, char **argv)
{
FILE * fp1;
FILE * fp2;
uint8_t input[MAX_SIZE];
uint32_t input_size;
uint8_t *output;
uint32_t output_size;
uint16_t batch_size;
uint32_t batch_count;
uint32_t elem_count;
uint8_t val;
uint32_t *p_val32;
float *p_float;

	fp1 = fopen(argv[1], "r");
	if (fp1 == NULL) {
		fprintf(stderr, "Could not open input file\n");
		exit(EXIT_FAILURE);
	}

	fp2 = fopen(argv[2], "w");
	if (fp2 == NULL) {
		fprintf(stderr, "Could not open output file\n");
		exit(EXIT_FAILURE);
	}

	input_size = 0;

	while (fread((void *) &val, sizeof(uint8_t), 1, fp1) == 1) {
		input[input_size++] = val;
	}

	printf("Compressed file contains %d bytes\n", input_size);

	output = uncompress_float(input);
	if (output == NULL) {
		fprintf(stderr, "Error uncompressing input file\n");
		exit(EXIT_FAILURE);
	}

	p_val32 = (uint32_t *) output;
	output_size = *p_val32; 
	if (output_size == 0) {
		fprintf(stderr, "Error uncompressing input file\n");
		exit(EXIT_FAILURE);
	}

	// printf("Uncompressed BLOB contains %d bytes\n", output_size);

	elem_count = output_size / sizeof(float);

	// printf("Uncompressed BLOB contains %d elements\n", elem_count);

	// Skip the first four bytes before writing
	output += sizeof(uint32_t);
	p_float = (float *) output;

	if (fwrite((void *) p_float, sizeof(float), elem_count, fp2) != elem_count) {
		fprintf(stderr, "Error writing output file\n");
		exit(EXIT_FAILURE);
	}

	printf("Uncompression successful, wrote %d floating point numbers to the output file\n", elem_count);

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}
