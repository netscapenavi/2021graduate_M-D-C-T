#include "fftw3.h"
#include <stdlib.h>
#include <math.h>
#define PI 3.1415926536
extern int window_size;

float* vorbis_window_gen()
{
	static float* window;
	int i;
	float temppow;
	window=(float*)malloc(window_size*sizeof(float));
	for (i=0; i<window_size; ++i)
	{
		temppow=sin((i+0.5)*PI/(float)window_size);
		*(window+i)=(float)sin((float)(0.5*PI*temppow*temppow));
	}
	return window;
}

float* dct4(float *pcm, int length) /*DCT-IV*/
{
	int i;
	static float* dct_result;
	fftw_complex *in, *out;
	fftw_plan schedule;
	in=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	out=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	dct_result=(float*) malloc(length*sizeof(float));
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
		*(dct_result+i)=2*out[i*2+1][0];  /*Normalize 상수는 MDCT에서 2, IMDCT에서 2/N*/
	}
	fftw_destroy_plan(schedule);
	fftw_free(in);
	fftw_free(out);
	return dct_result;
}

float* mdct(float *pcm, int pcm_length)
{
	int i;
	float *pcm_shorten;
	static float* dct4_result;
	pcm_shorten=(float*)calloc(pcm_length/2,sizeof(float));
	dct4_result=(float*)calloc(pcm_length/2,sizeof(float));
	for (i=0; i<pcm_length/4; ++i)/*MDCT는 input 수가 2N개인 상황에서 특정 방식으로 합해 N개로 줄인 상태에서 수행하는 DCT.*/
	{
		pcm_shorten[i]=(-1.0)*(pcm[3*pcm_length/4-1-i]+pcm[3*pcm_length/4+i]);
		pcm_shorten[i+pcm_length/4]=pcm[i]-pcm[pcm_length/2-1-i];
	}
	dct4_result=dct4(pcm_shorten, pcm_length/2);
	free(pcm_shorten);
	return dct4_result;
}

void imdct(float *dct_value, int mdct_length)
{
	int i, j;
	float* temp_idct;
	temp_idct=(float*)calloc(window_size,sizeof(float));
	for(i=0; i<mdct_length*2; ++i)
	{
		for (j=0; j<mdct_length; ++j)
		{
			temp_idct[i]+=dct_value[j]*cos(PI*(2*i+1+mdct_length)*(2*j+1)/(2.0*(float)mdct_length));
		}
	}
}