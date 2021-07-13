void file_initialize(SF_INFO*);
char* copy_csvfile_name(char*);
struct csv_dec_package *coeff_from_csv(FILE*);
void make_output_audio_file(double*, char*, unsigned int, unsigned int);
