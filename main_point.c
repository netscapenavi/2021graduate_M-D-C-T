#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <math.h>
#include <limits.h>
#include "dct.h"
#include "file_initialize.h"
#include "main_point.h"
float* dct_result;/*malloc�� heap error�� �ذ����� ���� �Ұ����ϰ� �������� ���.*/
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
	opened_file=sf_open(filename, SFM_READ, &soundinfo);/*SNDFILE ���� ������ ����� ������ PCM ��ȣ �Է�. SF_INFO ���� ������ ������ ��Ÿ ���� �Է�*/
	printf("%d\n", soundinfo.frames);/*���� ��ȣ�� ä�� �� �� ���� ��.*/
	printf("%d\n", soundinfo.channels);/*���� ��ȣ�� ä�� ��.*/
	printf("%d\n", soundinfo.samplerate);/*���ø� ����Ʈ.*/
	pcmaud = (float*) calloc(soundinfo.frames*soundinfo.channels, sizeof(float));
	length_for_process=WINDOW_SIZE*(soundinfo.frames/WINDOW_SIZE)+WINDOW_SIZE;
	for (i=0; i<soundinfo.channels; ++i)
	{
		pcmch[i]=(float*) calloc(length_for_process,sizeof(float));
	}
	sf_readf_float(opened_file,pcmaud,soundinfo.frames);/*float �������� PCM ��ȣ�� ��ȯ*/
	for (i=0; i<soundinfo.frames; ++i)/*�Է¹��� PCM ��ȣ�� �� ä�η� �и�.*/
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