#include "fftw3.h"
#include <stdlib.h>
#include <math.h>
#define PI 3.1415926536
extern int window_size;

double* vorbis_window_gen()
{
	static double* window;
	int i;
	double temppow;
	window=(double*)malloc(window_size*sizeof(double));
	for (i=0; i<window_size; ++i)
	{
		temppow=sin((i+0.5)/(double)window_size*PI);
		*(window+i)=(double)sin((double)(0.5*PI*temppow*temppow));
	}
	return window;
}

double* dct4(double *pcm, int length) /*DCT-IV*/
{
	int i;
	static double* dct_result;
	fftw_complex *in, *out;
	fftw_plan schedule;
	in=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	out=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	dct_result=(double*) malloc(length*sizeof(double));
	for (i=0; i<length; ++i)
	{
		in[i*2][0]=0;/*Real part*/
		in[i*2][1]=0;/*Imaginary part*/
		in[i*2+1][0]=0.25*pcm[i];
		in[i*2+1][1]=0;
	}
	in[2*length][0]=0;
	in[2*length][1]=0;
	for (i=2*length-1; i>0; --i)
	{
		in[2*length+i][0]=(-1.0)*in[2*length-i][0];
		in[2*length+i][1]=(-1.0)*in[2*length-i][1];
	}
	in[4*length][0]=0;
	in[4*length][1]=0;
	for (i=4*length-1; i>0; --i)
	{
		in[4*length+i][0]=in[4*length-i][0];
		in[4*length+i][1]=in[4*length-i][1];
	}
	schedule=fftw_plan_dft_1d(length*8, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(schedule);
	for (i=0; i<length; ++i)
	{
		*(dct_result+i)=out[i*2+1][0];
	}
	fftw_destroy_plan(schedule);
	fftw_free(in);
	fftw_free(out);
	return dct_result;
}

double* mdct(double *pcm, int pcm_length)
{
	int i;
	double *pcm_shorten;
	static double* dct4_result;
	pcm_shorten=(double*)calloc(pcm_length/2,sizeof(double));
	dct4_result=(double*)calloc(pcm_length/2,sizeof(double));
	for (i=0; i<(pcm_length>>2); ++i)/*MDCT는 input 수가 2N개인 상황에서 특정 방식으로 합해 N개로 줄인 상태에서 수행하는 DCT.*/
	{
		pcm_shorten[i]=(-1.0)*(pcm[3*(pcm_length>>2)-1-i]+pcm[3*(pcm_length>>2)+i]);
		pcm_shorten[i+(pcm_length>>2)]=pcm[i]-pcm[(pcm_length>>1)-1-i];
	}
	dct4_result=dct4(pcm_shorten, pcm_length>>1);
	for (i=0; i<(pcm_length>>1); ++i)
	{
		dct4_result[i]*=2; /*Normalize 상수는 MDCT에서 2, IMDCT에서 2/N*/
	}
	free(pcm_shorten);
	return dct4_result;
}

double* imdct(double *dct_value, int mdct_length)
{
	int i;
	static double *temp_idct;
	double *tempsave, *tempsave_long, normalize;
	int length2x;
	length2x=mdct_length*2;
	temp_idct=(double*)calloc(length2x,sizeof(double));
	tempsave_long=(double*)calloc(length2x,sizeof(double));
	/*for(i=0; i<mdct_length*2; ++i)
	{
		for (j=0; j<mdct_length; ++j)
		{
			temp_idct[i]+=dct_value[j]*cos(PI*(2*i+1+mdct_length)*(2*j+1)/(2.0*(double)mdct_length));
		}
	}*/
	tempsave=dct4(dct_value,mdct_length);
	normalize=((double)1.0)/mdct_length; /*MDCT의 normalize를 위한 상수가 여기에 포함.*/
	for (i=0; i<mdct_length; ++i)
	{
		tempsave_long[i]=normalize*tempsave[i];
	}
	for (i=mdct_length; i<length2x; ++i)
	{
		tempsave_long[i]=-1*normalize*tempsave[length2x-1-i];
	}
	for (i=(mdct_length>>1); i<length2x; ++i)
	{
		temp_idct[i-(mdct_length>>1)]=tempsave_long[i];
	}
	for (i=0; i<(mdct_length>>1); ++i)
	{
		temp_idct[i+3*(mdct_length>>1)]=-1*tempsave_long[i];
	}
	free(tempsave_long);
	free(tempsave);
	return temp_idct;
}