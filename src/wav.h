/* This is set of wav file operation helpers for simple Wav to Mp3 encoder
 *
 * Authored by: Alexander Trofimov <molochmail@gmail.com>
 */
#pragma once
#include <inttypes.h>

#define ID_LEN 4
typedef struct {
	char			ChunkID[ID_LEN];
	uint32_t		ChunkDataSize;
} chunk_header;

#define RIFF_HEADER "RIFF"
#define RIFF_TYPE	"WAVE"
typedef struct {
	chunk_header	Header; 
	char			RiffType[ID_LEN];	
} riff_header;

#define FMT_HEADER "fmt "
typedef struct {
	chunk_header	Header; 
	uint16_t		CompressionCode;// always 0x01
	uint16_t		ChannelNumbers; // 0x01 Mono, 0x02 Stereo
	uint32_t		SampleRate;		// Hz
	uint32_t		BytesPerSecond;
	uint16_t		BytesPerSample; // 1 8Bit Mono, 2 8Bit stereo or 
									// 16Bit mono, 4 16Bit stereo
	uint16_t		BitsPerSample;
} fmt_header;

#define DATA_HEADER "data"
typedef struct {
	chunk_header Header; 
//	uint8_t		 Data[0];
} data_header;

typedef struct {
	int			Fd;
	int			Off;
	size_t			ReadOff;
	riff_header		Riff;
	fmt_header		Fmt;
	data_header		Data;
} wav_file;

wav_file* wav_load_file(const char* fname);
int wav_read_samples(wav_file* wf, uint8_t* buf, size_t size);
void wav_unload_file(wav_file* wf);

uint32_t wav_get_sample_rate(wav_file* wf);
uint32_t wav_get_channels_number(wav_file* wf);
uint32_t wav_get_bytes_per_sample(wav_file* wf);

