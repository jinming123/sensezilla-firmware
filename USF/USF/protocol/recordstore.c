/*
 * recordstore.c
 *
 * Created: 10/15/2013 4:45:55 PM
 *  Author: kweekly
 */ 
#include "devicedefs.h"

#ifdef USE_RECORDSTORE
#include <avrincludes.h>
#include "protocol/sensor_packet.h"
#include "recordstore.h"

#define MAX_RECORD_TYPES 16
#define RECORDSTORE_MEMSIZE (100*120L)
//#define RECORDSTORE_MEMSIZE 0

uint8_t * first_record_pointers[MAX_RECORD_TYPES];
uint8_t first_record_pos;

uint8_t memblock_raw[RECORDSTORE_MEMSIZE];
uint8_t * memblock;
uint8_t * memblock_pos;

uint8_t * oldmemblock_end;

uint16_t stat_records;
uint16_t stat_uncompressed_size;
uint16_t stat_compressed_size;

void recordstore_init() {
	recordstore_clear();
}

void recordstore_clear() {
	uint16_t off;
	// set default MT and BT
	off = packet_construct_recordstore_header(memblock_raw);
	memblock = memblock_raw + off;
	
	oldmemblock_end = NULL;
	memblock_pos = memblock;
	first_record_pos = 0;
	for ( size_t c = 0; c < MAX_RECORD_TYPES; c++ ) {
		first_record_pointers[c] = NULL;
	}
	stat_records = stat_compressed_size = stat_uncompressed_size = 0;
}

void recordstore_insert(uint8_t * data, uint8_t len) {
	size_t c;
	uint8_t bdiff[16];
	
	if ( len & 0x80 ) {
		kputs("Record size too big");
		return;
	}
	if ( (memblock_pos - memblock_raw) + len + 1 >= RECORDSTORE_MEMSIZE ) {
		kputs("Memblock overflow");
		return;
	}
	
	for ( c = 0; c < first_record_pos; c++ ) {
		if (first_record_pointers[c] != NULL && 
				(0x80 ^ *(first_record_pointers[c])) == len) {
			break;
		}
	}
	//printf_P(PSTR("Inserting %d byte record:"),len);
	stat_uncompressed_size += len;
	stat_records ++ ;
	
	if ( c == first_record_pos ) { // new first record
		first_record_pointers[c] = memblock_pos;
		memblock_pos[0] = 0x80 | len;
		memcpy(memblock_pos+1,data,len);
		memblock_pos += len + 1;
		first_record_pos++;
		//printf_P(PSTR(" New record created 1 Byte overhead"));
		stat_compressed_size += len + 1;
	} else {
		// look for differences in bytes
		uint8_t * first_record = first_record_pointers[c] + 1;
		uint8_t bdiff_size = (len + 7) / 8;
		uint8_t * data_insert = memblock_pos + bdiff_size + 1;
		uint8_t bytes_saved = 0;
		uint8_t bytes_total = bdiff_size + 1;
		
		memset(bdiff,0,sizeof(bdiff));
		for ( size_t b = 0; b < len; b++ ) {
			if ( first_record[b] != data[b] ) {
				bdiff[b/8] |= (1<<(b%8));
				*(data_insert++) = data[b];
				bytes_total++;
			} else {
				bytes_saved++;
			}
		}
		// len
		memblock_pos[0] = len;
		// bdiff
		memcpy(memblock_pos+1,bdiff,bdiff_size);
		// data already inserted
		// ready for next record
		memblock_pos = data_insert;
		stat_compressed_size += bytes_total;
		//printf_P(PSTR("%d bytes overhead, %d bytes saved, %d bytes total"),1+bdiff_size,bytes_saved,bytes_total);
	}
	//printf_P(PSTR("Recordstore stats: Records %d Uncompressed %6d Compressed %6d Efficiency %2d%%\n"),stat_records,stat_uncompressed_size,stat_compressed_size,100*stat_compressed_size/stat_uncompressed_size);
	//printf_P(PSTR(" recordstore %d/%d bytes\n"),memblock_pos-memblock_raw,RECORDSTORE_MEMSIZE);
}

uint8_t * recordstore_dump(uint16_t * len) {
	stat_compressed_size = stat_uncompressed_size = stat_records = 0;
	*len = (uint16_t)(memblock_pos - memblock_raw);
	return memblock_raw;
}

void recordstore_start_new_block() {
	first_record_pos = 0;
	for ( size_t c = 0; c < MAX_RECORD_TYPES; c++ ) {
		first_record_pointers[c] = NULL;
	}	
	oldmemblock_end = memblock_pos;
}

void recordstore_flush_sent_block() {
	if (oldmemblock_end) {
		uint16_t offset =  (oldmemblock_end - memblock);
		for ( size_t c = 0; c < MAX_RECORD_TYPES; c++ ) {
			if ( first_record_pointers[c] ) {
				first_record_pointers[c] -= offset;
			}
		}
		for ( uint8_t * m = memblock; m < oldmemblock_end; m++) {
			*m = *(m+offset);
		}
		memblock_pos -= offset;
	}
}

#endif