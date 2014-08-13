/* Examples/decoder.c
 * Catherine D. Schuman, James S. Plank

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
This program takes as input an inputfile, k, m, a coding
technique, w, and packetsize.  It is the companion program
of encoder.c, which creates k+m files.  This program assumes
that up to m erasures have occurred in the k+m files.  It
reads in the k+m files or marks the file as erased. It then
recreates the original file and creates a new file with the
suffix "decoded" with the decoded contents of the file.

This program does not error check command line arguments because
it is assumed that encoder.c has been called previously with the
same arguments, and encoder.c does error check.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include "jerasure.h"
#include "reed_sol.h"
#include "galois.h"
#include "cauchy.h"
#include "liberation.h"

#define N 10

enum Coding_Technique {Reed_Sol_Van, Reed_Sol_R6_Op, Cauchy_Orig, Cauchy_Good, Liberation, Blaum_Roth, Liber8tion, RDP, EVENODD, No_Coding};

char *Methods[N] = {"reed_sol_van", "reed_sol_r6_op", "cauchy_orig", "cauchy_good", "liberation", "blaum_roth", "liber8tion", "rdp", "evenodd", "no_coding"};

/* Global variables for signal handler */
enum Coding_Technique method;
int readins, n;

/* Function prototype */
void ctrl_bs_handler(int dummy);

int main (int argc, char **argv)
{
    FILE *fp;				// File pointer

    /* Jerasure arguments */
    char **data;
    char **coding;
    int *erasures;
    int *erased;
    /* Vandermonde matrix */
    int matrix[3][10] =
    {
        {1,1,1,1,1,1	,1,1	,1,1	},
        {1,147,138,73,93	,161,103,58,99,178},
        {1,220,166,123,82,143,245,40,167,122}
    };//*matrix;
    int *bitmatrix;

    /* Parameters */
    int k, m, w, packetsize, buffersize;
    enum Coding_Technique tech;
    char *c_tech;

    int i, j;				// loop control variables
    int blocksize;			// size of individual files
    int origsize,newsize;			// size of file before padding
    int total;				// used to write data, not padding to file
    struct stat status;		// used to find size of individual files
    int numerased;			// number of erased files

    /* Used to recreate file names */
    char *temp,tmp;
    char *cs1, *cs2;
    char *fname;
    int md,ret;
    char *curdir;

    /* Used to time decoding */
    struct timeval t1, t2, t3, t4;
    struct timezone tz;
    double tsec;
    double totalsec;
    char t;

    signal(SIGQUIT, ctrl_bs_handler);

    //matrix = NULL;
    bitmatrix = NULL;
    totalsec = 0.0;

    /* Start timing */
    gettimeofday(&t1, &tz);

    /* Error checking parameters */
    curdir = (char *)malloc(sizeof(char)*100);
    //getcwd(curdir, 100);
    curdir = "/project/c/rs_ec_test/van_encoder_incr/bin/Debug";

    /* Begin recreation of file names */
    cs1 = (char*)malloc(sizeof(char)*strlen(argv[1]));
    cs2 = strrchr(argv[1], '/');
    if (cs2 != NULL)
    {
        cs2++;
        strcpy(cs1, cs2);
    }
    else
    {
        strcpy(cs1, argv[1]);
    }
    cs2 = strchr(cs1, '.');
    if (cs2 != NULL)
    {
        *cs2 = '\0';
    }
    cs2 = (char*)malloc(sizeof(char)*strlen(argv[1]));
    fname = strchr(argv[1], '.');
    strcpy(cs2, fname);
    fname = (char *)malloc(sizeof(char*)*(100+strlen(argv[1])+10));

    /* Read in parameters from metadata file */
    sprintf(fname, "%s/coding/%s_meta.txt", curdir, cs1);
    fp = fopen(fname, "rb");
    temp = (char *)malloc(sizeof(char)*(strlen(argv[1])+10));
    fscanf(fp, "%s", temp);
    if (fscanf(fp, "%d", &origsize) != 1)
    {
        fprintf(stderr, "Original size is not valid\n");
        exit(0);
    }
    if (fscanf(fp, "%d", &newsize) != 1) {
		fprintf(stderr, "New size is not valid\n");
		exit(0);
	}
    if (fscanf(fp, "%d %d %d %d %d", &k, &m, &w, &packetsize, &buffersize) != 5)
    {
        fprintf(stderr, "Parameters are not correct\n");
        exit(0);
    }
    c_tech = (char *)malloc(sizeof(char)*(strlen(argv[1])+10));
    fscanf(fp, "%s", c_tech);
    fscanf(fp, "%d", &tech);
    method = tech;
    fscanf(fp, "%d", &readins);
    fclose(fp);

    /* Allocate memory */
    erased = (int *)malloc(sizeof(int)*(k+m));
    for (i = 0; i < k+m; i++)
        erased[i] = 0;
    erasures = (int *)malloc(sizeof(int)*(k+m));
    data = (char **)malloc(sizeof(char *)*k);
    coding = (char **)malloc(sizeof(char *)*m);
    if (buffersize != origsize)
    {
        for (i = 0; i < k; i++)
        {
            data[i] = (char *)malloc(sizeof(char)*(buffersize/k));
        }
        for (i = 0; i < m; i++)
        {
            coding[i] = (char *)malloc(sizeof(char)*(buffersize/k));
        }
        blocksize = buffersize/k;
    }

    sprintf(temp, "%d", k);
    md = strlen(temp);
    gettimeofday(&t3, &tz);

    /* Begin decoding process */
    total = 0;
    n = 1;
    tmp = (char *)calloc(33,sizeof(char));

    /* Start decoding */
    int er_file_no;
    readins = origsize/buffersize;
    if (buffersize == origsize)
    {
        blocksize = newsize;
    }
    while (n <= readins)
    {
        numerased = 0;

        /* Open files, check for erasures, read in data/coding */
        for (i = 1; i <= k; i++)
        {
            if(n == 1 && i == 1)
            {
                erased[i-1] = 1;
                erasures[numerased] = i-1;
                numerased++;
            }
            else
            {
                data[i-1] = (char *)calloc(blocksize,sizeof(char));
            }
        }
        for (i = 1; i <= m; i++)
        {
            sprintf(fname, "%s/coding/m%0*d%s", curdir,md, i, cs2);
            fp = fopen(fname, "rb");
            if (fp == NULL)
            {
                erased[k+(i-1)] = 1;
                erasures[numerased] = k+i-1;
                numerased++;
            }
            else
            {
                coding[i-1] = (char *)malloc(sizeof(char)*blocksize);
                fseek(fp, blocksize*(n-1), SEEK_SET);
                fread(coding[i-1], sizeof(char), blocksize, fp);

                fclose(fp);
            }
        }

        /* Finish allocating data/coding if needed */
        if (n == 1)
        {
            for (i = 0; i < numerased; i++)
            {
                if (erasures[i] < k)
                {
                    data[erasures[i]] = (char *)malloc(sizeof(char)*blocksize);
                }
                else
                {
                    coding[erasures[i]-k] = (char *)malloc(sizeof(char)*blocksize);
                }
            }
        }

        erasures[numerased] = -1;
        gettimeofday(&t3, &tz);

        /* Choose proper decoding method */
        if (tech == Reed_Sol_Van || tech == Reed_Sol_R6_Op)
        {
            i = jerasure_matrix_decode(k, m, w, matrix, 1, erasures, data, coding, blocksize);
        }
        else if (tech == Cauchy_Orig || tech == Cauchy_Good || tech == Liberation || tech == Blaum_Roth || tech == Liber8tion)
        {
            i = jerasure_schedule_decode_lazy(k, m, w, bitmatrix, erasures, data, coding, blocksize, packetsize, 1);
        }
        else
        {
            fprintf(stderr, "Not a valid coding technique.\n");
            exit(0);
        }
        gettimeofday(&t4, &tz);

        /* Exit if decoding was unsuccessful */
        if (i == -1)
        {
            fprintf(stderr, "Unsuccessful!\n");
            exit(0);
        }

        /* Create decoded file */
//		for(i=0;i<k;i++){
//            printf("n = %d\ndata[%d] = %s\nlen = %d\n\n",n,i,data[i],strlen(data[i]));
//		}

        /* Write decoded data to file */
        sprintf(fname, "%s/coding/%d_decoded%s", curdir, n, cs2);
        fp = fopen(fname, "wb");
        if ((ret = access(fname, R_OK|W_OK)) == 0)
        {
            fp = fopen(fname, "ab");
        }
        else
        {
            fp = fopen(fname, "wb");
        }
        for (i = 0; i < k; i++)
        {
            if (total+blocksize <= origsize)
            {
                fwrite(data[i], sizeof(char), blocksize, fp);
                total+= blocksize;
            }
            else
            {
                for (j = 0; j < blocksize; j++)
                {
                    if (total < origsize)
                    {
                        fprintf(fp, "%c", data[i][j]);
                        total++;
                    }
                    else
                    {
                        break;
                    }

                }
            }
        }
        fclose(fp);

        n++;
        tsec = 0.0;
        tsec += t4.tv_usec;
        tsec -= t3.tv_usec;
        tsec /= 1000000.0;
        tsec += t4.tv_sec;
        tsec -= t3.tv_sec;
        totalsec += tsec;
    }

    /* Free allocated memory */
    free(cs1);
    free(fname);
    free(data);
    free(coding);
    free(erasures);
    free(erased);

    /* Stop timing and print time */
    gettimeofday(&t2, &tz);
    tsec = 0;
    tsec += t2.tv_usec;
    tsec -= t1.tv_usec;
    tsec /= 1000000.0;
    tsec += t2.tv_sec;
    tsec -= t1.tv_sec;
    printf("Time taken to decode file  total size %d is %0.10f\n",origsize,totalsec);
    printf("Decoding (MB/sec): %0.10f\n", (origsize/1024/1024)/totalsec);
    printf("De_Total (MB/sec): %0.10f\n\n", (origsize/1024/1024)/tsec);
}

void ctrl_bs_handler(int dummy)
{
    time_t mytime;
    mytime = time(0);
    fprintf(stderr, "\n%s\n", ctime(&mytime));
    fprintf(stderr, "You just typed ctrl-\\ in decoder.c\n");
    fprintf(stderr, "Total number of read ins = %d\n", readins);
    fprintf(stderr, "Current read in: %d\n", n);
    fprintf(stderr, "Method: %s\n\n", Methods[method]);
    signal(SIGQUIT, ctrl_bs_handler);
}
