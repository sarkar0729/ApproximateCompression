#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "bitUtils.h"

#define DEBUG 0

// This file contains a set of bit manipulation functions
//
// Command to compile: gcc -std=gnu99 -c bitUtils.c

// Returns 0 or 1 depending on whether the bit at bit_pos is 0 or 1
int 
get_bit(uint8_t *bit_array, int bit_pos)
{
uint8_t n;

	n = bit_array[bit_pos / 8];
	n = n & (1 << (bit_pos % 8));
	if (n)
		return 1;
	else
		return 0;
}

// Set the bit at bit_pos to 1
void 
set_bit(uint8_t *bit_array, int bit_pos)
{
uint8_t n;

	n = bit_array[bit_pos / 8];
	n = n | (1 << (bit_pos % 8));
	bit_array[bit_pos / 8] = n;
}

// Set the bit at bit_pos to 0
void 
clear_bit(uint8_t *bit_array, int bit_pos)
{
uint8_t n;
uint8_t mask;

	mask = ~(1 << (bit_pos % 8));
	n = bit_array[bit_pos / 8];
	n = n & mask;
	bit_array[bit_pos / 8] = n;
}

// Returns the number of 1 bits in this_byte
int
cnt_1_bits_from_byte(uint8_t this_byte)
{
int cnt_1_bits;
int bit_pos;
uint8_t mask;
uint8_t n;

	cnt_1_bits = 0;
	mask = 1;

	for (bit_pos = 0; bit_pos < 8; bit_pos++) {
		n = this_byte;
		n = n & mask;
		if (n)
			cnt_1_bits++;
		mask = mask << 1;
		}

	return cnt_1_bits;
}

// Returns the number of 0 bits in this_byte
int
cnt_0_bits_from_byte(uint8_t this_byte)
{
int cnt_0_bits;
int bit_pos;
uint8_t mask;
uint8_t n;

	cnt_0_bits = 0;
	mask = 1;

	for (bit_pos = 0; bit_pos < 8; bit_pos++) {
		n = this_byte;
		n = n & mask;
		if (n == 0)
			cnt_0_bits++;
		mask = mask << 1;
	}

	return cnt_0_bits;
}

// 
// Copies bit_count bits from vals to bit_array starting at offset bit_pos
// The bit 0 of val is copied to offset bit_count, bit 1 is copied to offset 
// bit_count + 1, bit 1 is copied to offset bit_count + 2 and so on.
//
// Returns the number of bits copied, in case of error returns -1
//
int
write_bitstream(uint8_t *bit_array, int bit_pos, int bit_count, uint64_t val)
{
uint64_t mask64;
uint8_t n;

	if (bit_count > 64 || bit_pos < 0)
		return (-1);

	if (bit_array == NULL)
		return (-1);

	mask64 = 1;
  
	for (int i = 0; i < bit_count; i++) {
		if (val & mask64)
			set_bit(bit_array, bit_pos + i);
		else
			clear_bit(bit_array, bit_pos + i);

		mask64 = mask64 << 1;    
	}
 
	return (bit_count);
}

// 
// Reads bit_count bits from bit_array starting at offset bit_pos to location 
// pointed by ptr_val.  The bit 0 is copied from offset bit_count, bit 1 is copied
// from offset bit_count + 1, bit 1 is copied from offset bit_count + 2 and so on.
//
// Returns the number of bits read, in case of error returns -1
//
int
read_bitstream(uint8_t *bit_array, int bit_pos, int bit_count, uint64_t *ptr_val)
{
uint64_t mask64;
uint64_t n;

	if (bit_count > 64 || bit_pos < 0)
		return (-1);

	if (bit_array == NULL || ptr_val == NULL)
		return (-1);

	*ptr_val = 0;
	mask64 = 1;
	n = 0;
  
	for (int i = 0; i < bit_count; i++) {
		if (get_bit(bit_array, bit_pos + i))
			n = n | mask64;

		mask64 = mask64 << 1;    
	}
 
	*ptr_val = n;
	return bit_count;
}

// Useful for debugging
void
print_bits_from_byte(uint8_t this_byte)
{
int bit_pos;
uint8_t mask;
uint8_t n;

	mask = 1;

	for (bit_pos = 0; bit_pos < 8; bit_pos++) {
		n = this_byte & mask;
		if (n)
			printf("1");
		else
			printf("0");

		mask = mask << 1;
	}

	printf(" ");

}

