#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include "main_start.h"
#include "file_initialize.h"
#define MAX_PATH_LENGTH 260

int window_size=0;
int main(int argc, char** argv)
{
	FILE *csvfile;
	struct csv_dec_package *dec_set;
	float *imdct_after, *pcm[CHANNEL_NUM], *window;
	char filepath[MAX_PATH_LENGTH];
	unsigned char tempch;
	unsigned int pcm_entire_length;
	int i,j, block_count, charac_count, comma_count=1;
	if (argc>2){
		printf("Only the name of the CSV file is necessary for input.\n");
		return -1;
	}
	++argv;
	csvfile=fopen(*argv,"r");
	dec_set=coeff_from_csv(csvfile);
	fclose(csvfile);
	window_size=dec_set->window_size_pack;
	imdct_after=(float*)malloc((window_size>>1)*sizeof(float));
	window=(float*)malloc(window_size*sizeof(float));
	window=vorbis_window_gen();
	pcm_entire_length=(window_size>>1)*(dec_set->block_count+1);
	for (i=0; i<CHANNEL_NUM; ++i)
	{
		float *tempflt;
		int k, tempcompare;
		tempflt=(float*)malloc(window_size);
		pcm[i]=(float*)malloc(pcm_entire_length*sizeof(float));
		tempcompare=(dec_set->block_count);
		for (k=0; k<tempcompare; ++k)
		{
			for (j=0; j<(window_size>>1); ++j)
			{	
				*(imdct_after+j)=(dec_set->coeff_bundle)[j];
			}
			tempflt=imdct(imdct_after,window_size>>1);
			for (j=0; j<window_size; ++j)
			{
				tempflt[j]*=window[j];
				pcm[i][j+k*(window_size>>1)]=tempflt[j];
			}
			free(tempflt);
		}
	}
	free(window);
	strncpy(filepath,*argv,MAX_PATH_LENGTH-1);
	exclude_filename_from_path(filepath);
	make_output_audio_file(pcm, filepath, pcm_entire_length);
	free(dec_set->coeff_bundle);
	free(dec_set);
	free(imdct_after);
	free(filepath);
	for (i=0; i<CHANNEL_NUM;++i)
	{
		free(pcm[i]);
	}
	return 0;
}