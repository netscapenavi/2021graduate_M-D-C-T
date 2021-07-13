#include <limits.h>
#include <math.h>
#include <time.h>
#include "ditherrng.h"

unsigned int seedgen()
{
	time_t seedbase;
	unsigned int seed;
	seedbase=time(NULL);
	seed=clock()+seedbase;
	if (seed==0) { /*xorshift32���� seed�� 0�� ���� �� �ȴ�.*/
		seed=clock();
	}
	return seed;
}
unsigned int xorshift32(unsigned int *seed)
{
	unsigned int next= *(seed);
	next^=next<<13;
	next^=next>>17;
	next^=next<<5;
	*(seed)=next;
	return next;
}
double ditherdouble() /*TPDF���� dithering. RPDF�� ������� 2�� �� ���ϴ� ������� ����.*/
{
	static char seed_on=0;
	static unsigned int state;
	int signedstate;
	double answer;
	static double normalize=1.0/(0.5+LONG_MAX);
	if (seed_on==0) {
		state=seedgen();
		seed_on++;
	}
	state=xorshift32(&state);
	signedstate=(int)(state-1); /*Overflowed to negative value.*/
	answer=((double)signedstate+0.5)*normalize;
	state=xorshift32(&state);
	signedstate=(int)(state-1);
	answer+=((double)signedstate+0.5)*normalize;
	answer/=(MAX_INT16_WAV_VAL*2);
	return answer;
}
