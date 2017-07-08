/*
 * gdbstub-internal.h
 *
 *  Created on: Oct 1, 2016
 *      Author: vlad
 */

#ifndef GDBSTUB_INTERNAL_H_
#define GDBSTUB_INTERNAL_H_

#include <stdint.h>

void gdb_packet_start();
void gdb_packet_char(char c);
void gdb_packet_str(const char * c);
void gdb_packet_end();
void gdb_packet_hex(int val, int bits);

static inline uint32_t bswap32(uint32_t i) {
	uint32_t r;
	r = ((i >> 24) & 0xff);
	r |= ((i >> 16) & 0xff) << 8;
	r |= ((i >> 8) & 0xff) << 16;
	r |= ((i >> 0) & 0xff) << 24;
	return r;
}

#endif /* GDBSTUB_INTERNAL_H_ */
