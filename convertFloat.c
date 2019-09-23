#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int
main(int argc, char **argv)
{
FILE * fp1;
FILE * fp2;
float val;
float val_orig;
float *p_val;
uint32_t val32;
uint32_t *p_val32;

	if (argc !=3) {
		fprintf(stderr, "Must provide two arguments, input file and output files\n");
		exit(EXIT_FAILURE);
	}

	p_val = &val;
	p_val32 = (uint32_t *) p_val;

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

	while ((fscanf(fp1, "%f", &val)) != EOF) {
		printf("%f\t0x%X\n", val, *p_val32);
		fwrite((void *) p_val32, sizeof(float), 1, fp2);
	} 

	fclose(fp1);
	fclose(fp2);

	exit(EXIT_SUCCESS);
}
