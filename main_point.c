#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <math.h>
#include <limits.h>
#include "dct.h"
#include "file_initialize.h"
#include "main_point.h"
float* dct_result;/*malloc의 heap error를 해결하지 못해 불가피하게 전역변수 사용.*/
int main(int argc, char** argv)
{
	FILE *mdct_container;
	int i, j, k;
	char filename[MAX_FILENAME_LEGNTH];
	float *pcmaud, *block_for_dct;
	float *pcmch[2];/*left channel: pcmch[0][], right channel: pcmch[1][]*/
	SF_INFO soundinfo;
	int length_for_process;
	SNDFILE *opened_file;
	argv++;
	if (*argv=='\0') {
		printf("Input file is not selected.\n");
		return -1;
	}
	strncpy(filename,*argv,MAX_FILENAME_LEGNTH-1);
	file_initialize(&soundinfo);
	mdct_container=fopen("MDCT_Result.csv", "a");
	opened_file=sf_open(filename, SFM_READ, &soundinfo);/*SNDFILE 형식 변수에 오디오 파일의 PCM 신호 입력. SF_INFO 형식 변수에 파일의 기타 정보 입력*/
	printf("%d\n", soundinfo.frames);/*읽은 신호의 채널 당 총 샘플 수.*/
	printf("%d\n", soundinfo.channels);/*읽은 신호의 채널 수.*/
	printf("%d\n", soundinfo.samplerate);/*샘플링 레이트.*/
	pcmaud = (float*) calloc(soundinfo.frames*soundinfo.channels, sizeof(float));
	length_for_process=WINDOW_SIZE*(soundinfo.frames/WINDOW_SIZE)+WINDOW_SIZE;
	for (i=0; i<soundinfo.channels; ++i)
	{
		pcmch[i]=(float*) calloc(length_for_process,sizeof(float));
	}
	sf_readf_float(opened_file,pcmaud,soundinfo.frames);/*float 형식으로 PCM 신호를 변환*/
	for (i=0; i<soundinfo.frames; ++i)/*입력받은 PCM 신호를 각 채널로 분리.*/
	{
		for (j=0; j<soundinfo.channels; ++j)
		{
			pcmch[j][i+WINDOW_SIZE/2]=pcmaud[i*2+j];
		}
	}
	block_for_dct=(float*)calloc(WINDOW_SIZE,sizeof(float));
	dct_result=(float*)calloc(WINDOW_SIZE/2,sizeof(float));
	for (i=0; i<soundinfo.channels; ++i)/*Fast DCT*/
	{
		for (k=0; k<length_for_process/WINDOW_SIZE; ++k)
		{
			for (j=0; j<WINDOW_SIZE; ++j)
			{
				block_for_dct[j]=pcmch[i][k*WINDOW_SIZE+j];
			}
			mdct(block_for_dct,WINDOW_SIZE);
			for (j=0; j<WINDOW_SIZE/2-1; ++j)
			{
				fprintf(mdct_container, "%f, ", dct_result[j]);
				dct_result[j]=0;
			}
			fprintf(mdct_container, "%f\n", dct_result[WINDOW_SIZE/2-1]);
			dct_result[WINDOW_SIZE/2-1]=0;
		}
	}
	sf_close(opened_file);
	free(pcmaud);
	free(block_for_dct);
	free(dct_result);
	for (i=0; i<soundinfo.channels; ++i)
	{
		free(pcmch[i]);
	}
	fclose(mdct_container);
	return 0;
}