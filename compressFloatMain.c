#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "approximateCompression.h"

// For handling larger input files, increase the constant below
#define MAX_SIZE 65536

/*
** This program reads an input file containing floating point
** numbers and generates a compressed file using the function
** compress_float. It is a lossy compression with an average
** error of 0.5%. The maximum error is guaranteed to be below
** one percent. Typical size for time series data is about 6%
** of the original size
**
** Command to compile: gcc -std=gnu99 -o compressFloat compressFloatMain.c
**                         approximateCompression.o bitUtils.o bucket.o uint8.o
** Usage:    ./compressFloat <uncompressed file> <compressed file>
**
** There is another program uncompressFloat to generate approximate
** version of the original file. The accuracy of compression can
** be checked using a program compareFloat.
*/


int
main(int argc, char **argv)
{
FILE * fp1;
FILE * fp2;
float val;
float *p_val;
float input[MAX_SIZE];
uint8_t *compressed_buffer;
uint8_t *batch_ptr;
uint32_t val32;
uint32_t *p_val32;
uint32_t elem_count;
uint32_t start;
uint32_t end;
uint32_t output_size;
uint32_t byte_count;

	if (argc != 3) {
		fprintf(stderr, "Usage: compressFloat <floating point file> <compressed binary file>\n");
		exit(EXIT_FAILURE);
	}

	fp1 = fopen(argv[1], "rb");
	if (fp1 == NULL) {
		fprintf(stderr, "Could not open input file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	fp2 = fopen(argv[2], "wb");
	if (fp2 == NULL) {
		fprintf(stderr, "Could not open output file %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 65536; i++)
		input[i] = 0.0;

	p_val = &val;
	p_val32 = (uint32_t *) p_val;

	elem_count = 0;

	while (fread((void *) p_val32, sizeof(float), 1, fp1) == 1) {
		input[elem_count++] = val;
	}

	printf("Input file %s has %d floating point numbers\n", argv[1], elem_count);

	compressed_buffer = compress_float(elem_count, input);
	if (compressed_buffer == NULL) {
		fprintf(stderr, "Internal error: Compression failed\n");
		exit(EXIT_FAILURE);
	}

	p_val32 = (uint32_t *) compressed_buffer;
	output_size = *p_val32;

	// Now flush the output buffer to fp2
	if ((byte_count = fwrite(compressed_buffer , sizeof(uint8_t), output_size, fp2)) != output_size) {
		fprintf(stderr, "Internal error: write to output file failed %d bytes written\n", byte_count); 
		exit(EXIT_FAILURE);
	};

	printf("Sucessfully generated compressed output file %s of size %d bytes\n", argv[2], output_size);

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}

