void file_initialize(SF_INFO*);
char* copy_csvfile_name(char*);
char* extract_filename_from_path(const char*);
void exclude_filename_from_path(char*);
struct csv_dec_package *coeff_from_csv(FILE*);
void make_output_audio_file(float*, char*, unsigned int);
