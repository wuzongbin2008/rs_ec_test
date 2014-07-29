#ifndef VAN_ENCODE_H_INCLUDED
#define VAN_ENCODE_H_INCLUDED

#include "galois.h"
#include "reed_sol.h"
#include "jerasure.h"

#define N 10

enum Coding_Technique {Reed_Sol_Van, Reed_Sol_R6_Op, Cauchy_Orig, Cauchy_Good, Liberation, Blaum_Roth, Liber8tion, RDP, EVENODD, No_Coding};
extern enum Coding_Technique method;
/* Global variables for signal handler */
extern int readins;

/* Function prototypes */
int check_argv(int argc,char *argv[]);
int determine_proper_buffersize(int buffersize,int packetsize,int w,int k);
enum Coding_Technique set_coding_technique(char *argv[],int k,int w,int m,int packetsize);
int get_size_file(char *argv[]);
int determine_newsize_by_packetsize(int size,int packetsize,int buffersize,int k,int w);
int determine_readins_number(int size,int newsize,int buffersize,char *block,int *blocksize,int k);
int read_inputfile_buffersize(char *block,int size,int buffersize,FILE *fp);
int jfread(void *ptr, int size, int nmembers, FILE *stream);
int write_data_to_files(FILE *fp,int k,char **data,int blocksize,char *fname,int n);

int jfread(void *ptr, int size, int nmembers, FILE *stream);
int is_prime(int w);
void ctrl_bs_handler(int dummy,int n,int readins,enum Coding_Technique method);

#endif // VAN_ENCODE_H_INCLUDED
