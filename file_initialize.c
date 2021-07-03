#include "sndfile.h"
#include "main_start.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int window_size;

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
	int i,j,k,half_block;
	char tempch='d';
	char *imdct_before;
	static struct csv_dec_package for_imdct;
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
	for_imdct.coeff_bundle=(float*)malloc(half_block*sizeof(float*));
	imdct_before=(char*)malloc(half_block*sizeof(char));
	j=0;k=0;
	while (i<for_imdct.charac_count)
	{
		tempch=(char)fgetc(csvfile);
		if (tempch==',') {
			*(imdct_before+j)='\n';
            *(for_imdct.coeff_bundle+for_imdct.block_count*half_block+k)=atof(imdct_before);
			free(imdct_before);	
			imdct_before=(char*)malloc(half_block*sizeof(char));
			++k;j=0;
		} else if (tempch=='\n') {
			*(imdct_before+j)='\n';
            *(for_imdct.coeff_bundle+for_imdct.block_count*half_block+k)=atof(imdct_before);
			++for_imdct.block_count;
			for_imdct.coeff_bundle=(float*)realloc((void*)for_imdct.coeff_bundle,(half_block*(for_imdct.block_count+1))*sizeof(float));
			free(imdct_before);	
			imdct_before=(char*)malloc(half_block*sizeof(char));
			j=0;k=0;
		} else {
			*(imdct_before+j)=tempch;
			++j;
		}
		++i;
	}
	for_imdct.block_count/=2;
	free(imdct_before);
	return &for_imdct;
}
void make_output_audio_file(float* pcm[], char* filepath, unsigned int pcm_length)
{
	SNDFILE *newfile;
	SF_INFO *sf_info;
	int i;
	sf_info->frames=pcm_length;
	sf_info->channels=CHANNEL_NUM;
	sf_info->format=(SF_FORMAT_WAV|SF_FORMAT_PCM_16);
	sf_info->samplerate=44100;
	newfile=sf_open(filepath,SFM_WRITE,sf_info);
}