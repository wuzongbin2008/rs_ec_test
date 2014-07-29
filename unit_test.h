#ifndef UNIT_TEST_H_INCLUDED
#define UNIT_TEST_H_INCLUDED

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define nf_align(type, size, align) (((size)+(align)-1) & ((type)~((align)-1)))

void get_param(int argc,char *argv[]);
void print_file_Statfs();
off_t get_file_offset();
int notfs_hash();

#endif // UNIT_TEST_H_INCLUDED
