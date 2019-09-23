
void uint8_encode(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_1_2_3(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_4_5(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_6_7_8_9(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_10_11_12_13(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_10_11_12_13(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_14_15_16_17(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);
void uint8_encode_18(uint8_t encode_key, uint16_t len, uint8_t *buf, uint8_t *encoded_buf);

int uint8_decode(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_1_2_3(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_4_5(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_6_7_8_9(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_10_11_12_13(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_14_15_16_17(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
int uint8_decode_18(uint8_t encode_key, uint16_t batch_size, uint8_t *encoded_buffer, uint8_t *decoded_buffer);
