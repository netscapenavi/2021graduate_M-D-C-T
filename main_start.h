#define CHANNEL_NUM 2
struct csv_dec_package{
	unsigned int block_count;
	unsigned int window_size_pack;
	unsigned int charac_count;
	double *coeff_bundle;
};
