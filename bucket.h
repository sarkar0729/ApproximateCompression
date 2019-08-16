
#define BUCKET_COUNT 36
#define INVALID_BUCKET 255

uint8_t value_to_bucket(float value);
float bucket_to_value(uint8_t bucket);
uint8_t *bucketize(uint32_t batch_size, float *input, float max, float min);
uint8_t bucket_analyze(uint16_t len, uint8_t *buf);

