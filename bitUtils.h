#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Function declarations */

int get_bit(uint8_t *bit_array, int bit_pos);
void set_bit(uint8_t *bit_array, int bit_pos);
void clear_bit(uint8_t *bit_array, int bit_pos);
int cnt_1_bits_from_byte(uint8_t this_byte);
int cnt_0_bits_from_byte(uint8_t this_byte);
int write_bitstream(uint8_t *bit_array, int bit_pos, int bit_count, uint64_t val);
int read_bitstream(uint8_t *bit_array, int bit_pos, int bit_count, uint64_t *ptr_val);
void print_bits_from_byte(uint8_t this_byte);
