#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// To compile: gcc -std=gnu99 -o convertDouble convertDouble.c 

int
main(int argc, char **argv)
{
FILE * fp1;
FILE * fp2;
double val;
double *p_val;
uint64_t val64;
uint64_t *p_val64;

	if (argc !=3) {
		fprintf(stderr, "Must provide two arguments, input file and output files\n");
		exit(EXIT_FAILURE);
	}

	p_val = &val;
	p_val64 = (uint64_t *) p_val;

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

	while ((fscanf(fp1, "%lf", &val)) != EOF) {
		printf("%lf\t0x%016llx\n", val, *p_val64);
		fwrite((void *) p_val64, sizeof(double), 1, fp2);
	} 

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}
