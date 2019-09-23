#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "approximateCompression.h"

// For handling larger input files, increase the constant below
#define MAX_SIZE 65536

/*
** This program reads a compressed file previously generated using
** program compressDouble and generates approximate version of the
** original file containing double precision floating point numbers 
** using the function decompress_double. It is a lossy compression 
** with an average error of 0.5%. The maximum error is guaranteed to be below one percent. 
** Typical size for time series data is about 6% of the original size,
** in other words about 2 bits per double precision floating point number (32 bit).
**
** Command to compile: gcc -std=gnu99 -o decompressDouble decompressDoubleMain.c 
**                         approximateCompression.o bitUtils.o bucket.o uint8.o
** Usage:    ./decompressDoubleMain compressed_file decompressed_file
**
** The accuracy of compression can be checked using a program compareDouble.
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
double *p_double;

	if (argc != 3) {
		fprintf(stderr, "Usage: decompressFloat <compressed binary file> <floating point file>\n");
		exit(EXIT_FAILURE);
	}

	fp1 = fopen(argv[1], "rb");
	if (fp1 == NULL) {
		fprintf(stderr, "Could not open input compressed file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	fp2 = fopen(argv[2], "wb");
	if (fp2 == NULL) {
		fprintf(stderr, "Could not open output file %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	input_size = 0;

	while (fread((void *) &val, sizeof(uint8_t), 1, fp1) == 1) {
		input[input_size++] = val;
	}

	printf("Compressed file %s contains %d bytes\n", argv[1], input_size);

	output = decompress_float((compressed_array) input);
	if (output == NULL) {
		fprintf(stderr, "Error decompressing input file\n");
		exit(EXIT_FAILURE);
	}

	// First four bytes of the returned BLOB contains the size
	// followed by the floating point numbers
	p_val32 = (uint32_t *) output;
	output_size = *p_val32; 
	if (output_size == 0) {
		fprintf(stderr, "Error decompressing input file\n");
		exit(EXIT_FAILURE);
	}

	elem_count = output_size / sizeof(double);

	// Skip the first four bytes before writing the
	// floating points to the output file
	output += sizeof(uint32_t);
	p_double = (double *) output;

	if (fwrite((void *) p_double, sizeof(double), elem_count, fp2) != elem_count) {
		fprintf(stderr, "Error writing output file\n");
		exit(EXIT_FAILURE);
	}

	printf("Decompression successful, wrote %d double precision floating point numbers to the file %s\n", elem_count, argv[2]);

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}
