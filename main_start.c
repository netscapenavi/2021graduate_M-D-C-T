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
	double *imdct_after, *pcm[CHANNEL_NUM], *window;
	char filepath[MAX_PATH_LENGTH], *filename;
	unsigned int pcm_entire_length;
	unsigned int i,j, comma_count=1;
	int filepath_length;
	char csv_ext[3]={'c','s','v'};
	if (argc>2){
		printf("Only the name of the CSV file is necessary for input.\n");
		return -1;
	} else if (argc<2) {
		printf("Enter the name of the CSV file.\n");
		return -1;
	}
	++argv;
	csvfile=fopen(*argv,"r");
	if (csvfile==0) {
		printf("No such file exists.\n");
		return -2;
	}
	for (i=0; i<3; ++i)
	{
		filepath_length=strlen(*argv);
		if (*(*(argv)+filepath_length+i-3)!=csv_ext[i]) {
			printf("Only CSV file can be read.\n");
			return -3;
		}
	}
	dec_set=coeff_from_csv(csvfile); /*dec_set에는 block number, block size (window size), float 형식의 MDCT coefficient, CSV file의 글자 수를 기록한다.*/
	fclose(csvfile);
	window_size=dec_set->window_size_pack;
	imdct_after=(double*)malloc((window_size>>1)*sizeof(double));
	window=(double*)malloc(window_size*sizeof(double));
	window=vorbis_window_gen();
	pcm_entire_length=(window_size>>1)*(dec_set->block_count+1);
	for (i=0; i<CHANNEL_NUM; ++i)
	{
		double *tempflt;
		int k;
		tempflt=(double*)malloc(window_size);
		pcm[i]=(double*)calloc(pcm_entire_length,sizeof(double));
		for (k=0; k<dec_set->block_count; ++k)
		{
			for (j=0; j<(window_size>>1); ++j) /*MDCT coefficient에 IMDCT를 수행하기 위해 window_size의 절반만큼씩 나누어 저장.*/
			{	
				*(imdct_after+j)=(dec_set->coeff_bundle)[j+k*window_size+i*(window_size>>1)];
			}
			tempflt=imdct(imdct_after,window_size>>1);
			for (j=0; j<window_size; ++j)
			{
				tempflt[j]*=window[j];
				pcm[i][j+k*(window_size>>1)]+=tempflt[j];
			}
			free(tempflt); /*imdct 함수의 static float를 free한다.*/
		}
	}
	free(window);
	strncpy(filepath,*argv,MAX_PATH_LENGTH-1);
	filename=copy_csvfile_name(filepath);
	change_filename_to_wav(filepath);
	make_output_audio_file(pcm, filepath, pcm_entire_length, window_size>>1);
	printf("IMDCT of %s is complete.\n",filename);
	free(filename);
	free(dec_set->coeff_bundle);
	free(imdct_after);
	for (i=0; i<CHANNEL_NUM;++i)
	{
		free(pcm[i]);
	}
	return 0;
}