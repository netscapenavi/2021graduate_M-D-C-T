#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <math.h>
#include "dct.h"
#include "file_initialize.h"
#define MAX_PATH_LENGTH 260

int window_size;

int main(int argc, char** argv)
{
	float* dct_result;
	FILE *mdct_container;
	int i, j, k;
	char filename[MAX_PATH_LENGTH];
	char *newfilename;
	float *pcmaud, *block_for_dct, *window;
	float *pcmch[2];/*left channel: pcmch[0][], right channel: pcmch[1][]*/
	SF_INFO soundinfo;
	int length_for_process, total_block_num;
	SNDFILE *opened_file;
	if (argc<3){
		printf("Not enough arguments. First, type in the window size. The name of the sound file should follow.\n");
		printf("Format)) MDCT_converter (window size) (file name)\n");
		return -3;
	}
	++argv;
	window_size=atoi(*argv);
	if (window_size%4) {
		printf("Window size must be a multiple of 4.\n");
		return -2;
	} 
	if (window_size >= 100000 ) {
		printf("Window size must be smaller than 100000.\n");
	}
	argv++;
	if (*argv=='\0') {
		printf("No input file is selected.\n");
		return -1;
	}
	strncpy(filename,*argv,MAX_PATH_LENGTH-1);
	file_initialize(&soundinfo);
	opened_file=sf_open(filename, SFM_READ, &soundinfo);/*SNDFILE 형식 변수에 오디오 파일의 PCM 신호 입력. SF_INFO 형식 변수에 파일의 기타 정보 입력*/
	newfilename=copy_csvfile_name(filename);
	mdct_container=fopen(newfilename, "w");
	free(newfilename);
	printf("Number of samples per channel: %d\n", soundinfo.frames);/*읽은 신호의 채널 당 총 샘플 수.*/
	printf("Number of channels: %d\n", soundinfo.channels);/*읽은 신호의 채널 수.*/
	printf("Sampling rate: %d\n", soundinfo.samplerate);/*샘플링 레이트.*/
	if (window_size>soundinfo.frames) {
		printf("Samples should be longer than or equal to the window length.\n");
		return -4;
	}
	window=vorbis_window_gen();
	pcmaud = (float*) calloc(soundinfo.frames*soundinfo.channels, sizeof(float));
	length_for_process=(window_size/2)*(soundinfo.frames/(window_size/2))+window_size;
	for (i=0; i<soundinfo.channels; ++i)
	{
		pcmch[i]=(float*) calloc(length_for_process,sizeof(float));
	}
	sf_readf_float(opened_file,pcmaud,soundinfo.frames);/*float 형식으로 PCM 신호를 변환*/
	for (i=0; i<soundinfo.frames; ++i)/*입력받은 PCM 신호를 각 채널로 분리.*/
	{
		for (j=0; j<soundinfo.channels; ++j)
		{
			pcmch[j][i+(window_size>>1)]=pcmaud[(i<<1)+j];
		}
	}
	sf_close(opened_file);
	free(pcmaud);
	block_for_dct=(float*)calloc(window_size,sizeof(float));
	dct_result=(float*)calloc(window_size/2,sizeof(float));
	total_block_num=length_for_process/(window_size/2)-1;
	for (k=0; k<total_block_num; ++k)
	{
		for (i=0; i<soundinfo.channels; ++i)
		{
			for (j=0; j<window_size; ++j)
			{
				block_for_dct[j]=pcmch[i][k*(window_size>>1)+j];
				block_for_dct[j]*=window[j];
			}
			dct_result=mdct(block_for_dct,window_size);
			for (j=0; j<(window_size>>1)-1; ++j)
			{
				fprintf(mdct_container, "%f, ", dct_result[j]);
				/*CSV 파일에 MDCT coeffcient 기록 순서:
				L, 1st window_size sample
				R, 1st window_size sample
				L, 2nd window_size sample
				R, 2nd window_size sample
				L, 3rd window_size sample
				R, 3rd window_size sample
				.
				.
				*/
			}
			fprintf(mdct_container, "%f\n", dct_result[window_size/2-1]);
			free(dct_result);
		}
	}
	free(block_for_dct);
	free(window);
	for (i=0; i<soundinfo.channels; ++i)
	{
		free(pcmch[i]);
	}
	fclose(mdct_container);
	printf("MDCT conversion of %s is complete.\n",filename);
	return 0;
}