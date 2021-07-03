#define CHANNEL_NUM 2
struct csv_dec_package{
	int block_count;
	int window_size_pack;
	int charac_count;
	float *coeff_bundle;
};
