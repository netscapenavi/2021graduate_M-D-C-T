#include "sndfile.h"
#include "main_start.h"
#include "ditherrng.h"
#include "file_initialize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int window_size;
#define MAX_SMALL_CHUNK 24

void file_initialize(SF_INFO *info)
{
	info->format=0;
}
char *copy_csvfile_name(char input_charac[]) /*입력받은 파일 경로 중 마지막의 확장자를 '.csv' 확장자로 바꾼다. MDCT 프로그램에 사용.*/
{
	int i, input_length=0;
	char extension[4]={'c','s','v','\0'};
	static char* csv_name;
	while (input_charac[input_length]!='\0')
	{
		++input_length;
	}
	++input_length;
	csv_name=(char*)calloc(input_length,sizeof(char));
	for (i=0; i<input_length; ++i)
	{
		*(csv_name+i)=input_charac[i];
	}
	for (i=0; i<4; ++i)
	{
		*(csv_name+i+input_length-4)=extension[i];
	}
	return csv_name;
}
void change_filename_to_wav(char *path) /*파일의 이름에 '_re'를 추가하고 '.wav' 확장자 역시 추가한다. IMDCT 프로그램에 사용.*/
{
	int path_length, i, word_length=0;
	char wavname[8]={'_','r','e','.','w','a','v','\0'};
	path_length=strlen(path);
	while (path[path_length-1-word_length]!='.')
	{
		++word_length;
	}
	for (i=path_length-1-word_length; i<path_length+5; ++i)
	{
		path[i]=wavname[i-path_length+1+word_length];
	}
}
struct csv_dec_package *coeff_from_csv(FILE *csvfile) /*CSV 파일을 읽어 저장된 MDCT coefficient 및 block size, block number 등 필요한 값을 구해 출력한다. IMDCT 프로그램에서 사용.*/
{
	unsigned int i,j,k,l,half_block,temp_block_count;
	char tempch='d';
	char imdct_before[MAX_SMALL_CHUNK];
	static struct csv_dec_package for_imdct;
	double *temp_ptr=0;
	fseek(csvfile,0,SEEK_END);
	for_imdct.charac_count=ftell(csvfile);
	fseek(csvfile,0,SEEK_SET);
	for_imdct.block_count=0;
	temp_block_count=0;
	i=0;
	while (i<for_imdct.charac_count)
	{
		tempch=(char)fgetc(csvfile);
		if (tempch=='\n') {
			++for_imdct.block_count;
		}
		++i;
	}
	rewind(csvfile);
	while (tempch!='\n')
	{
		tempch=fgetc(csvfile);
		if (tempch==',') {
			++for_imdct.window_size_pack;
		}
	}
	++for_imdct.window_size_pack;
	half_block=for_imdct.window_size_pack;
	for_imdct.window_size_pack*=2;
	rewind(csvfile);
	for_imdct.coeff_bundle=(double*)malloc(half_block*(for_imdct.block_count+1)*sizeof(double));
	i=0;j=0;k=0;
	while (i<for_imdct.charac_count)
	{
		tempch=(char)fgetc(csvfile);
		if (tempch==',') {
			imdct_before[j]='\n';
            *(for_imdct.coeff_bundle+temp_block_count*half_block+k)=atof(imdct_before);
			l=0;
			while (l<MAX_SMALL_CHUNK)
			{
				imdct_before[l]=0;
				++l;
			}
			++k;
			j=0;
		} else if (tempch=='\n') {
			imdct_before[j]='\n';
            *(for_imdct.coeff_bundle+temp_block_count*half_block+k)=atof(imdct_before);
			++temp_block_count;
			l=0;
			while (l<MAX_SMALL_CHUNK)
			{
				imdct_before[l]=0;
				++l;
			}
			j=0;
			k=0;
		} else if (tempch>0){
			imdct_before[j]=tempch;
			++j;
		}
		++i;
	}
	for_imdct.block_count>>=1;
	return &for_imdct;
}
void make_output_audio_file(double *pcm[], char *filepath, unsigned int pcm_length, unsigned int half_window_size) /*PCM이 될 float 값을 dithering 후 16비트 int 형식 wav 파일로 변환한다. IMDCT 프로그램에서 사용.*/
{
	SNDFILE *newfile;
	SF_INFO sf_info;
	unsigned int i, j;
	int *pcm_for_file;
	float dither_limit;
	dither_limit=(MAX_INT16_WAV_VAL-1)/(float)MAX_INT16_WAV_VAL;
	pcm_for_file=(int*)malloc(CHANNEL_NUM*(pcm_length-half_window_size)*sizeof(int));
	for (i=0; i<CHANNEL_NUM; ++i)
	{
		j=half_window_size;
		while (j<pcm_length)
		{
			if (pcm[i][j]<=dither_limit && pcm[i][j]>=-1*dither_limit) { /*16비트 int로 표시 가능한 최대 및 최소값에서는 dithering 시 클리핑 발생 가능.*/
				pcm[i][j]+ditherdouble();
			}
			pcm_for_file[i+(j-half_window_size)*2]=(int)((pcm[i][j])*MAX_INT16_WAV_VAL)<<16;
			++j;
		}
	}
	sf_info.frames=pcm_length;
	sf_info.channels=CHANNEL_NUM;
	sf_info.format=(SF_FORMAT_WAV|SF_FORMAT_PCM_16);
	sf_info.samplerate=44100;
	newfile=sf_open(filepath,SFM_WRITE,&sf_info);
	sf_writef_int(newfile,pcm_for_file,pcm_length-half_window_size);
	free(pcm_for_file);
	sf_close(newfile);
}