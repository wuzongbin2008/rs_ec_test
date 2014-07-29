#ifndef EC_SRC_H_INCLUDED
#define EC_SRC_H_INCLUDED

#include "jerasure/galois.h"
#include "jerasure/reed_sol.h"
#include "jerasure/jerasure.h"

/* data structure for generating Lookup Table for the code*/
typedef struct {
	int chunkIndex;
	long start;
	long Nbytes;
} chunkInfoType;

typedef struct {
	chunkInfoType chunkInfo;
} lookupTable;

long adjust_buffersize(int w,long buffersize);
int find_src_chunksize(int f,int k,int w,int filesize,long buffersize);
void create_lookup_tables(int n,int f,int chunkSize,lookupTable **LookupTable);

#endif // EC_SRC_H_INCLUDED
