/* This is wav files to Mp3 files encoder
 *
 * Authored by: Alexander Trofimov <molochmail@gmail.com>
 * TODO: 
 * 1. Implement more strict input file format check
 *
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <lame/lame.h>

#include "wav.h"
#include "mp3.h"

static int do_lame_encode(wav_file* wf, mp3_file* mf)
{
	int err = -1;
	lame_global_flags* lame = lame_init();
	if(lame) {
		const int chunk = 1152;
		uint8_t wav_chunk[8*1152]; // reserv max space
		uint8_t mp3_chunk[1152];
		int r = 0;
		int e = 0;
	
		uint32_t sample_rate = wav_get_sample_rate(wf);
		uint32_t channels = wav_get_channels_number(wf);
		uint32_t bytespersample = wav_get_bytes_per_sample(wf);
		printf(
				"info: sample=%u "
				"channels=%u "
				"bytespersample=%u\n",
				sample_rate,
				channels,
				bytespersample);
		// setup lame
		lame_set_in_samplerate(lame, sample_rate);
		lame_set_out_samplerate(lame, sample_rate);
		lame_set_num_channels(lame, channels);
		lame_set_quality(lame, 2);
		lame_init_params(lame);

		// now we are ready for encode, lets do reading chunk, encode
		// and store to mp3 file
		do {
			r = wav_read_samples(wf, wav_chunk, bytespersample*chunk);
			if(r>0) {
				e = lame_encode_buffer_interleaved(
						lame,
						(uint16_t*)wav_chunk,
						r/bytespersample,
						mp3_chunk,
						chunk);
				if(e>0) {
					r = mp3_write(mf, mp3_chunk, e);
				}
				else {
					printf("error: encode call failed %d\n",
							e);
					break;
				}
			}
			else {
				printf("warning: file ended or error %d\n", r);
			}
		} while(r>0);

		e = lame_encode_finish(lame, wav_chunk, 8*chunk);
		if(e>0) {
			r = mp3_write(mf, wav_chunk, e);
			if(r!=e) 
				printf("error: could not write flushed data %d\n", r);
		}
		else 
			printf("error: could not finish encode %d\n", e);
		
		lame_close(lame);
		err = 0;
	}
	else {
		printf("error: could not init lame\n");
	}	
	return err;
}

static int do_encode(
		char* fpath) 
{
	int err = -1;
	wav_file* wf = wav_load_file(fpath);
	if(wf) {
		char outfile[256];
		int len = strlen(fpath)-3;
		mp3_file* mf;
		// form mp3 file extension
		// we do len check at previous functions so
		// it is always >=3
		memcpy(outfile, fpath, len);
		outfile[len] = 'm';
		outfile[len+1] = 'p';
		outfile[len+2] = '3';
		outfile[len+3] = '\0';
		mf = mp3_create_file(outfile);

		if(mf) {
			do_lame_encode(wf, mf);
			mp3_close_file(mf);
		}
		wav_unload_file(wf);
	}
	else {
		printf("error: could not process file %s\n", fpath);
		
	}

	return err;
}

static const char WAV_EXT[] = ".wav";
static inline int check_file_is_wav(const char* fpath) 
{
	int err = -1;
	size_t len = strlen(fpath);
	// check ".wav" at the end of file name
	if(len>=4) {
		size_t off = len-4;
		char* tmp = strcasestr(((char*)fpath)+off, WAV_EXT);
		if(tmp) {
			err = 0;
		}
	}
	return err;
}

static inline size_t atomic_add(size_t* ptr, size_t v) 
{
	size_t oldval;
	int b;
	do {
		oldval = *ptr;
		b = __sync_bool_compare_and_swap(ptr, oldval, oldval+v);
	} while(!b);	
	return oldval;
}

static inline size_t atomic_inc(size_t* ptr) 
{	
	return atomic_add(ptr, 1);
}

static inline size_t atomic_dec(size_t* ptr) 
{	
	return atomic_add(ptr, -1);
}

static size_t ref_counter = 0;

static void* encode_worker(void* data)
{
	char* full_path = (char*) data;
					
	do_encode(full_path);	
	
	// do not forget free data allocated in main thread
	free(data);
	atomic_dec(&ref_counter);

	pthread_exit(NULL);
}

static unsigned int get_cpus_number(void)
{
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;	
#else
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
		
}

// this is private finction iterating through files in directory
// and calling encoder to make wav to mp3 encode step
static int encode_dir(const char* path) 
{
	int err = -2;
	if(path) {
		DIR* d;
		struct dirent* dir;

		d = opendir(path);
		if(d) {
			unsigned int cpu_nr = get_cpus_number();
			printf("found %u cpus\n", cpu_nr);
			while( (dir=readdir(d)) != NULL ) {
				int iswav = check_file_is_wav(dir->d_name);
				if(!iswav) {					
					char* full_path = (char*)malloc(256);
					int e;
					pthread_t t;
					while(ref_counter>=cpu_nr) {
						usleep(1000);
					}						
					printf("info: found file %s\n", dir->d_name);
					snprintf(full_path, 256, "%s/%s", path, dir->d_name);
					atomic_inc(&ref_counter);
					e = pthread_create(&t, NULL, encode_worker, full_path);
					if(e) {
						printf("error: could not allocate more threads %d", e);
						break;
					}
					else {
						printf("info: allocated worker thread ref=%zu\n", ref_counter);
					}
					
				}
			}
			while(ref_counter>0) {
				usleep(1000);
			}
			usleep(1000);			
			err = 0;
			closedir(d);
		}
		else 
			printf("error: could not opendir()\n");
	}
	else {
		printf("error: bad args\n");
	}
	return err;
}

int main(int argc, char* argv[]) 
{
	int err = -1;
	const char* workdir;
	if(argc == 2) {
		struct stat st;
		err = stat(argv[1], &st);
		if(!err) {
			int isdir = S_ISDIR(st.st_mode);
			if(isdir) {
				err = encode_dir(argv[1]);
			}
			else 
				printf("error: the path specified is not a directory\n");
		}
		else 
			printf("error: could not stat %s\n", argv[1]);
	}
	else {
		printf("info: wav to mp3 encoding tool\n");
		printf("info: syntax $%s dir_path\n", argv[0]);
	}
	return err;
}

