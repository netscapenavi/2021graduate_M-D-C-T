#include "sndfile.h"
#include <stdlib.h>
#include <string.h>
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
int str_to_int(char* argument)
{
	int i, j, input_length, conversion_number=0, temp_num;
	input_length=strlen(argument);
	for (i=input_length-1; i>=0; --i)
	{
		temp_num=((int)(*(argument+i)-'0'));
		for (j=0; j<(input_length-1)-i; ++j)
		{
			temp_num*=10;
		}
		conversion_number+=temp_num;
	}
	return conversion_number;
}