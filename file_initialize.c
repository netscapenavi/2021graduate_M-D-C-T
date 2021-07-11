#include "sndfile.h"
#include "main_start.h"
#include "ditherrng.h"
#include "file_initialize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int window_size;
#define MAX_SMALL_CHUNK 24
#define MAX_INT_WAV_VAL 32767

void file_initialize(SF_INFO *info)
{
	info->format=0;
}
char *copy_csvfile_name(char input_charac[])
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
void exclude_filename_from_path(char *path)
{
	int path_length, word_length=0,i;
	path_length=strlen(path);
	while (path[path_length-1-word_length]!='\\')
	{
		path[path_length-1-word_length]='\0';
		++word_length;
	}
}
void change_filename_to_wav(char *path)
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
char *extract_filename_from_path(const char *path)
{
	static char *extracted_name;
	int path_length, word_length=0,i;
	path_length=strlen(path);
	while (path[path_length-1-word_length]!='\\')
	{
		++word_length;
	}
	extracted_name=(char*)malloc((word_length+1)*sizeof(char));
	for (i=0; i<word_length; ++i)
	{
		*(extracted_name+i)=*(path+i-word_length+path_length);
	}
	*(extracted_name+word_length)='\0';
	return extracted_name;
}
struct csv_dec_package *coeff_from_csv(FILE *csvfile)
{
	unsigned int i,j,k,l,half_block;
	char tempch='d';
	char imdct_before[MAX_SMALL_CHUNK];
	static struct csv_dec_package for_imdct;
	double *temp_ptr=0;;
	fseek(csvfile,0,SEEK_END);
	for_imdct.charac_count=ftell(csvfile);
	fseek(csvfile,0,SEEK_SET);
	for_imdct.block_count=0;
	i=0;
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
	for_imdct.coeff_bundle=(double*)malloc(half_block*sizeof(double));
	j=0;k=0;
	while (i<for_imdct.charac_count)
	{
		tempch=(char)fgetc(csvfile);
		if (tempch==',') {
			imdct_before[j]='\n';
            *(for_imdct.coeff_bundle+for_imdct.block_count*half_block+k)=atof(imdct_before);
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
            *(for_imdct.coeff_bundle+for_imdct.block_count*half_block+k)=atof(imdct_before);
			++for_imdct.block_count;
			temp_ptr=(double*)realloc((void*)for_imdct.coeff_bundle,(half_block*(for_imdct.block_count+1))*sizeof(double));
			for_imdct.coeff_bundle=temp_ptr;
			temp_ptr=0;
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
void make_output_audio_file(double *pcm[], char *filepath, unsigned int pcm_length, unsigned int half_window_size)
{
	SNDFILE *newfile;
	SF_INFO sf_info;
	unsigned int i, j;
	int *pcm_for_file;
	float dither_limit;
	dither_limit=(MAX_INT_WAV_VAL-1)/(float)MAX_INT_WAV_VAL;
	pcm_for_file=(int*)malloc(CHANNEL_NUM*(pcm_length-half_window_size)*sizeof(int));
	for (i=0; i<CHANNEL_NUM; ++i)
	{
		j=half_window_size;
		while (j<pcm_length)
		{
			if (pcm[i][j]<=dither_limit && pcm[i][j]>=-1*dither_limit) {
				pcm[i][j]+ditherdouble();
			}
			pcm_for_file[i+(j-half_window_size)*2]=(int)((pcm[i][j])*MAX_INT_WAV_VAL)<<16;
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