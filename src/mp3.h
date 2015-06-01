/* This is set of mp3 file operation helpers for simple Wav to Mp3 encoder
 *
 * Authored by: Alexander Trofimov <molochmail@gmail.com>
 */
#pragma once
#include <inttypes.h>

typedef struct {
	int Fd;
	size_t WriteOff;
} mp3_file;

mp3_file* mp3_create_file(const char* fname);
int mp3_write(mp3_file* mf, char* buf, int size);
void mp3_close_file(mp3_file* mf);

