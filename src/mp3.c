#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "mp3.h"

mp3_file* mp3_create_file(const char* fname)
{
	mp3_file *mf = (mp3_file*) malloc(sizeof *mf);
	if(mf) {
		int fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if(fd) {
			memset(mf, 0, sizeof *mf);
			mf->Fd = fd;
			printf("info: file %s (%d) successfully opened \n",
					fname,
					fd);
		}
		else {
			printf("error: cannot open file\n");
			free(mf);
			mf = NULL;
		}
	}
	else 
		printf("error: malloc failed\n");
	return mf;
}

int mp3_write(mp3_file* mf, char* buf, int size)
{
	int res = 0;
	if(mf) {
		res = write(mf->Fd, buf, size);
		if(res<=0) {
			printf("error: could not write %d\n", res);
		}
		else 
			mf->WriteOff+=res;
	}
	return res;
}

void mp3_close_file(mp3_file* mf)
{
	if(mf) {
		printf("info: file %d wrote %zu closed\n", 
			mf->Fd, mf->WriteOff);
		if(mf->Fd) 
			close(mf->Fd);
		free(mf);
	}
}



