#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "approximateCompression.h"

// For handling larger input files, increase the constant below
#define MAX_SIZE 65536

/*
** This program reads an input file containing double precision 
** floating point numbers and generates a compressed file using the 
** function compress_double. It is a lossy compression with an average
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
char *input_file;
char *output_file;
double val;
double *p_val;
double input[MAX_SIZE];
compressed_array compressed_buffer;
uint8_t *batch_ptr;
uint8_t accuracy;
uint32_t val32;
uint32_t *p_val32;
uint32_t elem_count;
uint32_t start;
uint32_t end;
uint32_t output_size;
uint32_t byte_count;

	if (argc == 3) {
		accuracy = ACCURACY_HALF_PERCENT;
		input_file = argv[1];
		output_file = argv[2];
	}

	else if (argc == 4) {
		if (strcmp(argv[1], "-L") == 0) 
			accuracy = ACCURACY_HALF_PERCENT;
		else if (strcmp(argv[1], "-M") == 0)
			accuracy = ACCURACY_QUARTER_PERCENT;
		else if (strcmp(argv[1], "-H") == 0)
			accuracy = ACCURACY_ONE_TENTH_PERCENT;
		else {
			fprintf(stderr, "Usage: compressFloat [-L|M|N] <double precision floating point file> <compressed binary file>\n");
			fprintf(stderr, "\t -L : Maximum error < 1%, Average error < 0.5%\n");
			fprintf(stderr, "\t -M : Maximum error < 0.5%, Average error < 0.25%\n");
			fprintf(stderr, "\t -H : Maximum error < 0.1%, Average error < 0.05%\n");
			exit(EXIT_FAILURE);
		}

		input_file = argv[2];
		output_file = argv[3];

	} else {
		fprintf(stderr, "Usage: compressFloat [-L|M] <double precision floating point file> <compressed binary file>\n");
		fprintf(stderr, "\t -L : Maximum error < 1%, Average error < 0.5%\n");
		fprintf(stderr, "\t -M : Maximum error < 0.5%, Average error < 0.25%\n");
		fprintf(stderr, "\t -H : Maximum error < 0.1%, Average error < 0.05%\n");
		exit(EXIT_FAILURE);
	}

	fp1 = fopen(input_file, "rb");
	if (fp1 == NULL) {
		fprintf(stderr, "Could not open input file %s\n", input_file);
		exit(EXIT_FAILURE);
	}

	fp2 = fopen(output_file, "wb");
	if (fp2 == NULL) {
		fprintf(stderr, "Could not open output file %s\n", output_file);
		exit(EXIT_FAILURE);
	}

	p_val = &val;

	elem_count = 0;

	while (fread((void *) p_val, sizeof(double), 1, fp1) == 1) {
		input[elem_count++] = val;
	}

	printf("Input file %s has %d double precision floating point numbers\n", input_file, elem_count);

	compressed_buffer = compress_double(elem_count, accuracy,  input);
	if (compressed_buffer == NULL) {
		fprintf(stderr, "Internal error: Compression failed\n");
		exit(EXIT_FAILURE);
	}

    // Extract the length of the opaque object
    output_size = get_compressed_length(compressed_buffer);

	// Now flush the output buffer to fp2
	if ((byte_count = fwrite(compressed_buffer , sizeof(uint8_t), output_size, fp2)) != output_size) {
		fprintf(stderr, "Internal error: write to output file failed %d bytes written\n", byte_count); 
		exit(EXIT_FAILURE);
	};

	printf("Sucessfully generated compressed output file %s of size %d bytes\n", output_file, output_size);

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}

