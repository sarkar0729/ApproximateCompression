
#define ACCURACY_HALF_PERCENT		1
#define ACCURACY_QUARTER_PERCENT	2
#define ACCURACY_ONE_TENTH_PERCENT	3

typedef struct compressed_array_structure *compressed_array;

compressed_array compress_float(uint32_t elem_count, uint8_t accuracy, float *input);
compressed_array compress_double(uint32_t elem_count, uint8_t accuracy, double *input);
uint8_t * decompress_float(compressed_array  input);
uint8_t * decompress_double(compressed_array  input);
uint32_t get_compressed_length(compressed_array c);
