#include "fftw3.h"
#include "main_point.h"
#include <stdlib.h>
#include <math.h>
#define PI 3.1415925636
extern float* dct_result;

void dct4(float *pcm, int length) /*DCT-IV*/
{
	int i;
	fftw_complex *in, *out;
	fftw_plan schedule;
	in=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	out=(fftw_complex*) fftw_malloc(8*length*sizeof(fftw_complex));
	for (i=0; i<length; ++i)
	{
		in[i*2][0]=0;/*Real part*/
		in[i*2][1]=0;/*Imaginary part*/
		in[i*2+1][0]=pcm[i];
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
}

void mdct(float *pcm, int pcm_length)
{
	int i;
	float *pcm_shorten;
	pcm_shorten=(float*)calloc(pcm_length/2,sizeof(float));
	for (i=0; i<pcm_length/2; ++i)
	{
		pcm_shorten[i]=(-1.0)*(pcm[3*pcm_length/2-1-i]+pcm[3*pcm_length/2+i]);
		pcm_shorten[i+pcm_length/2]=pcm[i]-pcm[pcm_length-1-i];
	}
	dct4(pcm_shorten, pcm_length/2);
	free(pcm_shorten);
}

void imdct(float *dct_value, int mdct_length)
{
	int i, j;
	float* temp_idct;
	temp_idct=(float*)calloc(WINDOW_SIZE,sizeof(float));
	for(i=0; i<mdct_length*2; ++i)
	{
		for (j=0; j<mdct_length; ++j)
		{
			temp_idct[i]+=dct_value[j]*cos(PI*(2*i+1+mdct_length)*(2*j+1)/(2.0*(float)mdct_length));
		}
	}
}