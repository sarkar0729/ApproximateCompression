
uint8_t value_to_bucket(float value, uint8_t accuracy);
float bucket_to_value(uint8_t bucke, uint8_t accuracyt);
uint8_t *bucketize(uint32_t batch_size, float *input, float max, float min, uint8_t precision, uint8_t accuracy);
void unbucketize(uint32_t length, uint8_t *bucket_array, uint8_t *float_array, float min, uint8_t precision, uint8_t accuracy);
uint8_t bucket_analyze(uint16_t len, uint8_t *buf);

