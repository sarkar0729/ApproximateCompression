#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>


// Command to compile: gcc -std=gnu99 -o compareFloat compareFloat.c

int
main(int argc, char **argv)
{
FILE * fp1;
FILE * fp2;
double val;
double val1;
double val2;
double err_percent;
double err_percent_max;
double err_percent_total;
double input[UINT16_MAX];
double input2[UINT16_MAX];
uint32_t elem_count;
uint32_t elem_count2;
uint8_t verbose;

	if ((argc < 3) || (argc > 4)) {
		fprintf(stderr, "Usage: compareFloat [-v] <file 1> <file 2>\n");
		exit(EXIT_FAILURE);
	}

	if (argc == 3) {
		fp1 = fopen(argv[1], "rb");
		if (fp1 == NULL) {
			fprintf(stderr, "Could not open first input file\n");
			exit(EXIT_FAILURE);
		}

		fp2 = fopen(argv[2], "rb");
		if (fp2 == NULL) {
			fprintf(stderr, "Could not open second input file\n");
			exit(EXIT_FAILURE);
		}

		verbose = 0;
	}

	if (argc == 4) {
		if (strcmp(argv[1], "-v") != 0) {
			fprintf(stderr, "Usage: compareFloat [-v] <file 1> <file 2>\n");
			exit(EXIT_FAILURE);
		}

		verbose = 1;

		fp1 = fopen(argv[2], "r");
		if (fp1 == NULL) {
			fprintf(stderr, "Could not open first input file\n");
			exit(EXIT_FAILURE);
		}

		fp2 = fopen(argv[3], "r");
		if (fp2 == NULL) {
			fprintf(stderr, "Could not open second input file\n");
			exit(EXIT_FAILURE);
		}
	}
	elem_count = 0;

	while (fread((void *) &val, sizeof(double), 1, fp1) == 1) {
		input[elem_count++] = val;
	}

	elem_count2 = 0;

	while (fread((void *) &val, sizeof(double), 1, fp2) == 1) {
		input2[elem_count2++] = val;
	}

	if (elem_count != elem_count2) {
		fprintf(stderr, "Warning!! Size of input files mismatch %d %d\n", elem_count, elem_count2);
		if (elem_count2 < elem_count)
			elem_count = elem_count2;
	}

	printf("Both files contain %d numbers\n", elem_count);

	err_percent_max = 0.0;
	err_percent_total = 0.0;

	for (int i = 0; i < elem_count; i++) {
		val1 = input[i];
		val2 = input2[i];
		if (val1 == 0.0) {
			if (val2 == 0.0) {
			val = 0.0;
			err_percent = 0.0;
			} else {
				printf("Mismatch: input[%d] = %.9lf\tcompressed value = %.9lf\n", i, val1, val2);
				continue;
			}
		} else {
			val = fabs(val1 - val2) * 100.0 / val1;
			err_percent = fabs(val1 - val2) * 100.0 / val1;
		}

		if (err_percent > err_percent_max)
			err_percent_max = err_percent;

		err_percent_total += err_percent;
		if (verbose == 1) {
			printf("input[%d] = %lf\tcompressed value = %lf\terr_percent = %.9f\%\n", i, val1, val2, val);
			printf("input[%d] = %lf\tcompressed value in hex = 0x%016llx\n", i, val1, (uint64_t) val2);
		}
	}

	printf("Average err_percent = %.9f\%\tMaximum err_percent = %.9f\%\n", err_percent_total/elem_count, err_percent_max);

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}

