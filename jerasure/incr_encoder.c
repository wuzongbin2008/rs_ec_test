/* Examples/encoder.c
 * Catherine D. Schuman, James S. Plank
 * https://github.com/SarabjotKhangura/Simple-Regenerating-Codes

Jerasure - A C/C++ Library for a Variety of Reed-Solomon and RAID-6 Erasure Coding Techniques
Copright (C) 2007 James S. Plank

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

James S. Plank
Department of Electrical Engineering and Computer Science
University of Tennessee
Knoxville, TN 37996
plank@cs.utk.edu
*/

/*
 * $Revision: 1.2 $
 * $Date: 2008/08/19 17:53:34 $
 */

/*

This program takes as input an inputfile, k, m, a coding
technique, w, and packetsize.  It creates k+m files from
the original file so that k of these files are parts of
the original file and m of the files are encoded based on
the given coding technique. The format of the created files
is the file name with "_k#" or "_m#" and then the extension.
(For example, inputfile test.txt would yield file "test_k1.txt".)

 */
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "jerasure_incr.h"
#include "reed_sol.h"
#include "galois.h"
#include "cauchy.h"
#include "liberation.h"

#define N 10

enum Coding_Technique {Reed_Sol_Van, Reed_Sol_R6_Op, Cauchy_Orig, Cauchy_Good, Liberation, Blaum_Roth, Liber8tion, RDP, EVENODD, No_Coding};

char *Methods[N] = {"reed_sol_van", "reed_sol_r6_op", "cauchy_orig", "cauchy_good", "liberation", "blaum_roth", "liber8tion", "no_coding"};

/* Global variables for signal handler */
int readins, n;
enum Coding_Technique method;

/* Function prototypes */
int is_prime(int w);
void ctrl_bs_handler(int dummy);

int jfread(void *ptr, int size, int nmembers, FILE *stream)
{
    int nd;
    int *li, i;
    if (stream != NULL) return fread(ptr, size, nmembers, stream);

    nd = size/sizeof(int);
    li = (int *) ptr;
    for (i = 0; i < nd; i++) li[i] = mrand48();
    return size;
}


int main (int argc, char **argv)
{
    FILE *fp, *fp2;				// file pointers
    char *memblock;		// reading in file
    char *block;				// padding file
    int size, newsize,mf_size,r_count;			// size of file and temp size
    struct stat status;			// finding file size

    enum Coding_Technique tech;		// coding technique (parameter)
    int k, m, w, packetsize;		// parameters
    int buffersize;					// paramter
    int i, j;						// loop control variables
    int blocksize;					// size of k+m files
    int total;
    int extra;

    /* Jerasure Arguments */
    char **data;
    char **coding;
    /* Vandermonde matrix */
    int matrix[3][10] =
    {
        {1,1,1,1,1,1	,1,1	,1,1	},
        {1,147,138,73,93	,161,103,58,99,178},
        {1,220,166,123,82,143,245,40,167,122}
    };//*matrix;
    int *bitmatrix;
    int **schedule;
    int *erasure;
    int *erased;

    /* Creation of file name variables */
    char temp[5];
    char *s1, *s2;
    char *fname;
    int md;
    char *curdir;

    /* Timing variables */
    struct timeval t1, t2, t3, t4;
    struct timezone tz;
    double tsec;
    double totalsec;
    struct timeval start, stop;

    /* Find buffersize */
    int up, down,ret;

    signal(SIGQUIT, ctrl_bs_handler);

    /* Start timing */
    gettimeofday(&t1, &tz);
    totalsec = 0.0;
    //matrix = NULL;
    bitmatrix = NULL;
    schedule = NULL;

    //test log
    char *matrix_data_file = ".matrix_data.txt";

    /* Error check Arguments*/
    if (argc != 8)
    {
        fprintf(stderr,  "usage: inputfile k m coding_technique w (packetsize) (buffersize)\n");
        fprintf(stderr,  "\nChoose one of the following coding techniques: \nreed_sol_van, \nreed_sol_r6_op, \ncauchy_orig, \ncauchy_good, \nliberation, \nblaum_roth, \nliber8tion");
        exit(0);
    }

    /* Conversion of parameters and error checking */
    if (sscanf(argv[2], "%d", &k) == 0 || k <= 0)
    {
        fprintf(stderr,  "Invalid value for k\n");
        exit(0);
    }
    if (sscanf(argv[3], "%d", &m) == 0 || m < 0)
    {
        fprintf(stderr,  "Invalid value for m\n");
        exit(0);
    }
    if (sscanf(argv[5],"%d", &w) == 0 || w <= 0)
    {
        fprintf(stderr,  "Invalid value for w.\n");
        exit(0);
    }
    if (argc == 6)
    {
        packetsize = 0;
    }
    else
    {
        if (sscanf(argv[6], "%d", &packetsize) == 0 || packetsize < 0)
        {
            fprintf(stderr,  "Invalid value for packetsize.\n");
            exit(0);
        }
    }
    if (argc != 8)
    {
        buffersize = 0;
    }
    else
    {
        if (sscanf(argv[7], "%d", &buffersize) == 0 || buffersize < 0)
        {
            fprintf(stderr, "Invalid value for buffersize\n");
            exit(0);
        }

    }

    /* Determine proper buffersize by finding the closest valid buffersize to the input value  */
    if (buffersize != 0)
    {
        if (packetsize != 0 && buffersize%(sizeof(int)*w*k*packetsize) != 0)
        {
            up = buffersize;
            down = buffersize;
            while (up%(sizeof(int)*w*k*packetsize) != 0 && (down%(sizeof(int)*w*k*packetsize) != 0))
            {
                up++;
                if (down == 0)
                {
                    down--;
                }
            }
            if (up%(sizeof(int)*w*k*packetsize) == 0)
            {
                buffersize = up;
            }
            else
            {
                if (down != 0)
                {
                    buffersize = down;
                }
            }
        }
        else if (packetsize == 0 && buffersize%(sizeof(int)*w*k) != 0)
        {
            up = buffersize;
            down = buffersize;
            while (up%(sizeof(int)*w*k) != 0 && down%(sizeof(int)*w*k) != 0)
            {
                up++;
                down--;
            }
            if (up%(sizeof(int)*w*k) == 0)
            {
                buffersize = up;
            }
            else
            {
                buffersize = down;
            }
        }
    }

    /* Setting of coding technique and error checking */
    if (strcmp(argv[4], "no_coding") == 0)
    {
        tech = No_Coding;
    }
    else if (strcmp(argv[4], "reed_sol_van") == 0)
    {
        tech = Reed_Sol_Van;
        if (w != 8 && w != 16 && w != 32)
        {
            fprintf(stderr,  "w must be one of {8, 16, 32}\n");
            exit(0);
        }
    }
    else
    {
        fprintf(stderr,  "Not a valid coding technique. Choose one of the following: reed_sol_van, reed_sol_r6_op, cauchy_orig, cauchy_good, liberation, blaum_roth, liber8tion, no_coding\n");
        exit(0);
    }

    /* Set global variable method for signal handler */
    method = tech;

    /* Get current working directory for construction of file names */
    curdir = (char*)malloc(sizeof(char)*1000);
    getcwd(curdir, 1000);

    /* Get size of file */
    if (argv[1][0] != '-')
    {
        /* Open file and error check */
        fp = fopen(argv[1], "rb");
        if (fp == NULL)
        {
            fprintf(stderr,  "Unable to open file.\n");
            exit(0);
        }

        /* Create Coding directory */
        i = mkdir("coding", S_IRWXU);
        if (i == -1 && errno != EEXIST)
        {
            fprintf(stderr, "Unable to create Coding directory.\n");
            exit(0);
        }

        /* Determine original size of file */
        stat(argv[1], &status);
        size = status.st_size;
        fclose(fp);
    }
    else
    {
        if (sscanf(argv[1]+1, "%d", &size) != 1 || size <= 0)
        {
            fprintf(stderr, "Files starting with '-' should be sizes for randomly created input\n");
            exit(1);
        }
        fp = NULL;
        srand48(time(0));
    }
    newsize = size;

    /* Find new size by determining next closest multiple */
    int test1=0,test2=0;
    if (packetsize != 0)
    {
        if (size%(k*w*packetsize*sizeof(int)) != 0)
        {
            while (newsize%(k*w*packetsize*sizeof(int)) != 0)
            {
                newsize++;
                test1++;
            }
        }
    }
    else
    {
        if (size%(k*w*sizeof(int)) != 0)
        {
            while (newsize%(k*w*sizeof(int)) != 0)
                newsize++;
        }
    }

    if (buffersize != 0)
    {
        while (newsize%buffersize != 0)
        {
            newsize++;
            test2++;
        }
    }
    //printf("test1 = %d\ntest2 = %d\n",test1,test2);

    /* Determine size of k+m files */
    blocksize = newsize/k;

    /* Allow for buffersize and determine number of read-ins */
    if (size > buffersize && buffersize != 0)
    {
        //判断有什么用？
        if (newsize%buffersize != 0)
        {
            readins = newsize/buffersize;
        }
        else
        {
            readins = newsize/buffersize;
        }
        block = (char *)malloc(sizeof(char)*buffersize);
        blocksize = buffersize/k;
    }
    else
    {
        readins = 1;
        buffersize = size;
        block = (char *)calloc(newsize,sizeof(char));
        blocksize = newsize;
    }

    /* Break inputfile name into the filename and extension */
    s1 = (char*)malloc(sizeof(char)*(strlen(argv[1])+10));
    s2 = strrchr(argv[1], '/');
    if (s2 != NULL)
    {
        s2++;
        strcpy(s1, s2);
    }
    else
    {
        strcpy(s1, argv[1]);
    }
    s2 = strchr(s1, '.');
    if (s2 != NULL)
    {
        *s2 = '\0';
    }
    fname = strchr(argv[1], '.');
    s2 = (char*)malloc(sizeof(char)*(strlen(argv[1])+5));
    if (fname != NULL)
    {
        strcpy(s2, fname);
    }

    /* Allocate for full file name */
    fname = (char*)malloc(sizeof(char)*(strlen(argv[1])+strlen(curdir)+100));
    sprintf(temp, "%d", k);
    md = strlen(temp);

    /* Allocate data and coding */
    data = (char **)malloc(sizeof(char*)*k);
    coding = (char **)malloc(sizeof(char*)*m);
    for (i = 0; i < m; i++)
    {
        //coding[i] = (char *)malloc(sizeof(char)*blocksize);

        sprintf(fname, "%s/coding/m%0*d%s", curdir,md, (i+1), s2);
        if ((ret = access(fname, R_OK|W_OK)) == 0)
        {
            fp = fopen(fname, "rb");
            if (fp == NULL)
            {
                fprintf(stderr,  "Invalid file for m%d\n",i);
                exit(0);
            }
            else
            {
                /* Determine original size of file */
                stat(fname, &status);
                mf_size = status.st_size;
                coding[i] = (char *)malloc(sizeof(char)*(blocksize));
                r_count = fread(coding[i], sizeof(char), mf_size, fp);
                if(r_count < mf_size)
                {
                    fprintf(stderr,  "read m%0*d failed\nmf_size = %d\nr_count = %d\n",md,i+1,mf_size,r_count);
                    exit(0);
                }
                printf("read m%0*d \nmf_size = %d\nr_count = %d\n",md,i+1,mf_size,r_count);
            }
            fclose(fp);
        }
        else
        {
            coding[i] = (char *)malloc(sizeof(char)*blocksize);
        }
    }

    /* Create coding matrix or bitmatrix and schedule */
    gettimeofday(&t3, &tz);
    gettimeofday(&start, &tz);
    gettimeofday(&t4, &tz);
    tsec = 0.0;
    tsec += t4.tv_usec;
    tsec -= t3.tv_usec;
    tsec /= 1000000.0;
    tsec += t4.tv_sec;
    tsec -= t3.tv_sec;
    totalsec += tsec;

    /* Read in data until finished */
    n = 1;
    total = 0;

    /* Start encoding */
    while (n <= readins)
    {
        /* Check if padding is needed, if so, add appropriate number of zeros */
        fp = fopen(argv[1], "rb");
        if (total < size && total+buffersize <= size)
        {
            total += jfread(block, sizeof(char), buffersize, fp);
        }
        else if (total < size && total+buffersize > size)
        {
            extra = jfread(block, sizeof(char), buffersize, fp);
            for (i = extra; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }
        else if (total == size)
        {
            for (i = 0; i < buffersize; i++)
            {
                block[i] = '0';
            }
        }

        /* Set pointers to point to file data */
        int file_no = atoi(s1);
        for (i = 0; i < k; i++)
        {
            if(i== (file_no - 1))
            {
                data[file_no - 1] = block;
            }
            else
            {
                data[i] = (char *)calloc(blocksize,sizeof(char));
            }
        }
        gettimeofday(&t3, &tz);

        /* Encode according to coding method */
        jerasure_matrix_encode(k, m, w, matrix, data, coding, blocksize,(file_no-1));
        gettimeofday(&t4, &tz);

        /* Write data to k files */
        sprintf(fname, "%s/coding/k%0*d%s", curdir, md,file_no, s2);
        if ((ret = access(fname, R_OK|W_OK)) == 0)
        {
            fp2 = fopen(fname, "ab");
        }
        else
        {
            fp2 = fopen(fname, "wb");
        }
        fwrite(data[file_no-1], sizeof(char), blocksize, fp2);
        fclose(fp2);

        /* Write encoded data to m files */
        for	(i = 1; i <= m; i++)
        {
            if (fp == NULL)
            {
                bzero(data[i-1], blocksize);
            }
            else
            {
                //sprintf(fname, "%s/Coding/%s_m%0*d%s", curdir, s1, md, i, s2);
                sprintf(fname, "%s/coding/m%0*d%s", curdir, md,i, s2);
                fp2 = fopen(fname, "wb");
//                if ((ret = access(fname, R_OK|W_OK)) == 0)
//                {
//                    fp2 = fopen(fname, "ab");
//                }
//                else
//                {
//                    fp2 = fopen(fname, "wb");
//                }
                fwrite(coding[i-1], sizeof(char), blocksize, fp2);
                fclose(fp2);
            }
        }

        /* Calculate encoding time */
        tsec = 0.0;
        tsec += t4.tv_usec;
        tsec -= t3.tv_usec;
        tsec /= 1000000.0;
        tsec += t4.tv_sec;
        tsec -= t3.tv_sec;
        totalsec += tsec;

        /* Create metadata file */
        fname = (char*)malloc(sizeof(char)*(strlen(s1)+strlen(curdir)+18));
        sprintf(fname, "%s/coding/%d_meta.txt", curdir, n);
        fp2 = fopen(fname, "wb");
        fprintf(fp2, "%s\n", argv[1]);
        fprintf(fp2, "%d\n", size);
        fprintf(fp2, "%d %d %d %d %d\n", k, m, w, packetsize, buffersize);
        fprintf(fp2, "%s\n", argv[4]);
        fprintf(fp2, "%d\n", tech);
        fprintf(fp2, "%d\n", readins);
        fclose(fp2);
        fclose(fp);

        n++;
    }

    /* Free allocated memory */
    free(s2);
    free(s1);
    free(fname);
    free(block);
    free(curdir);

    /* Calculate rate in MB/sec and print */
    gettimeofday(&t2, &tz);
    tsec = 0.0;
    tsec += t2.tv_usec;
    tsec -= t1.tv_usec;
    tsec /= 1000000.0;
    tsec += t2.tv_sec;
    tsec -= t1.tv_sec;
    printf("Time taken to encode file  total size %d is %0.10f\n",size,totalsec);
    printf("Encoding (MB/sec): %0.10f\n", (size/1024/1024)/totalsec);
    printf("En_Total (MB/sec): %0.10f\n", (size/1024/1024)/tsec);
}

/* is_prime returns 1 if number if prime, 0 if not prime */
int is_prime(int w)
{
    int prime55[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,
                     73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,
                     181,191,193,197,199,211,223,227,229,233,239,241,251,257
                    };
    int i;
    for (i = 0; i < 55; i++)
    {
        if (w%prime55[i] == 0)
        {
            if (w == prime55[i]) return 1;
            else
            {
                return 0;
            }
        }
    }
}

/* Handles ctrl-\ event */
void ctrl_bs_handler(int dummy)
{
    time_t mytime;
    mytime = time(0);
    fprintf(stderr, "\n%s\n", ctime(&mytime));
    fprintf(stderr, "You just typed ctrl-\\ in encoder.c.\n");
    fprintf(stderr, "Total number of read ins = %d\n", readins);
    fprintf(stderr, "Current read in: %d\n", n);
    fprintf(stderr, "Method: %s\n\n", Methods[method]);
    signal(SIGQUIT, ctrl_bs_handler);
}
