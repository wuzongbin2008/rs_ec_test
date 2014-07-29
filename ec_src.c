#include <stdlib.h>
#include <sys/errno.h>

#include "ec_src.h"

/* adjust buffersize */
long adjust_buffersize(int w,long buffersize){
    /* temp variable for adjusting buffersize*/
	int up;
	int down;

    if (buffersize%(sizeof(int)*w) != 0) {
        up = buffersize;
        down = buffersize;
        while (up%(sizeof(int)*w) != 0 && down%(sizeof(int)*w) != 0) {
            up++;
            down--;
        }
        if (up%(sizeof(int)*w) == 0) {
            buffersize = up;
        }
        else {
            buffersize = down;
        }
    }

    return buffersize;
}

int find_src_chunksize(int f,int k,int w,int filesize,long buffersize){
    int chunkSize = 0;

	/* find size of the chunks in SRC */
	chunkSize = filesize/(f*k);
	if (buffersize != 0) {
		while ((filesize%(k*f*w*sizeof(int)) != 0)||(chunkSize%buffersize !=0)||(filesize%(buffersize) != 0)) {
				filesize++;
				chunkSize = filesize/(f*k);
        }
	}

	return chunkSize;
}

void create_lookup_tables(int n,int f,int chunkSize,lookupTable **LookupTable){
    int i,j;
    /* used in generating look up table*/
	int temp_map = 0;

    /* create a look up table for placement of chunks into 'n' encoded files*/
	temp_map = 0;
	LookupTable = (lookupTable**)malloc(sizeof(lookupTable*)*(n)*(f+1));
	for(i=0;i<(n)*(f+1);i++)
		LookupTable[i] = (lookupTable*)malloc(sizeof(lookupTable));

	temp_map = n;
	for(i = 0; i<f+1; i++){
		for(j=0;(j<n);j++){
			(LookupTable[i+(f+1)*j]->chunkInfo).chunkIndex = (j+temp_map) % n;
			(LookupTable[i+(f+1)*j]->chunkInfo).Nbytes = chunkSize;
			(LookupTable[i+(f+1)*j]->chunkInfo).start = i*chunkSize;
        }
        temp_map--;
	}
	temp_map = 0;
}

