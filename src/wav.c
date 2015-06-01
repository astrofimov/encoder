#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "wav.h"

wav_file* wav_load_file(const char* fname)
{
	wav_file *wf = (wav_file*) malloc(sizeof *wf);
	if(wf) {
		int fd = open(fname, O_RDONLY);
		memset(wf, 0, sizeof *wf);
		if(fd) {
			int off = 0;
			int res;
			// first read riff header
			res = read(fd, &wf->Riff, sizeof(wf->Riff));
			if( (res == sizeof(wf->Riff)) &&
					(strncmp(wf->Riff.Header.ChunkID, 
							 RIFF_HEADER, ID_LEN) == 0) &&
					(strncmp(wf->Riff.RiffType, 
							 RIFF_TYPE, ID_LEN) == 0)) {
				// Cool this is WAV file do further steps
				off+=res;
				// secondly read forma data
				res = read(fd, &wf->Fmt, sizeof(wf->Fmt));
				if( (res == sizeof(wf->Fmt)) &&
							(strncmp(wf->Fmt.Header.ChunkID, 
							 FMT_HEADER, ID_LEN) == 0)) {
					// fmt header successfully loaded
					off+=res;
					// finally read data chunks header
					res = read(fd, &wf->Data, sizeof(wf->Data));
					if( (res == sizeof(wf->Data)) &&
								(strncmp(wf->Data.Header.ChunkID, 
								 DATA_HEADER, ID_LEN) == 0)) {
						off+= res;
						wf->Off = off; // keeping data offset
						wf->Fd = fd; // keeping file descriptor
						printf(
								"info: file %s (%d) successfully loaded\n",
								fname, wf->Fd);
					}
					else {
						// some error in file structure
						close(fd);
						free(wf);
						wf = NULL;
						printf("error: file %s could hot read data header\n",
								fname);
					}
				}
				else {
					// some error in file structure
					close(fd);
					free(wf);
					wf = NULL;
					printf("error: file %s could hot read fmt header\n",
							fname);
				}
			}
			else {
				// Sadly not a WAV
				close(fd);
				free(wf);
				wf = NULL;
				printf("error: file %s could hot read riff header\n",
						fname);
			}
		}
		else {
			free(wf);
			wf = NULL;
			printf("error: file %s could hot open\n",
					fname);
		}
	}
	return wf;
}

int wav_read_samples(wav_file* wf, uint8_t* buf, size_t size)
{
	int res = 0;
	if(wf) {
		int available_data = wf->Data.Header.ChunkDataSize - wf->ReadOff;
		int data4copy = (size<available_data)?size:available_data;
		res = read(wf->Fd, buf, data4copy);
		if(res<=0) {
			printf("warning: no more data or error %d\n", res);
		}
		else 
			wf->ReadOff+=res;
	}
	return res;
}

void wav_unload_file(wav_file* wf)
{
	if(wf) {
		printf("info: file %d read %zu Bytes closed\n", 
			wf->Fd, wf->ReadOff);
		if(wf->Fd) 
			close(wf->Fd);
		free(wf);
	}
}

uint32_t wav_get_sample_rate(wav_file* wf)
{
	if(wf)	
		return wf->Fmt.SampleRate;
	return 0;
}

uint32_t wav_get_channels_number(wav_file* wf)
{
	if(wf)
		return wf->Fmt.ChannelNumbers;
	return 0;
}

uint32_t wav_get_bytes_per_sample(wav_file* wf)
{
	if(wf)
		return wf->Fmt.BytesPerSample;
	return 0;
}


