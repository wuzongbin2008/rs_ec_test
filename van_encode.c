#include <stdlib.h>
#include <sys/errno.h>
#include <signal.h>
#include <sys/stat.h>

#include "van_encode.h"

char *Methods[N] = {"reed_sol_van", "reed_sol_r6_op", "cauchy_orig", "cauchy_good", "liberation", "blaum_roth", "liber8tion", "no_coding"};

int check_argv(int argc,char *argv[]){
    int k, m, w, packetsize;		// parameters
	int buffersize;					// paramter
	int i, j;						// loop control variables
	int blocksize;					// size of k+m files

    /* Error check Arguments*/
	if (argc != 8) {
		fprintf(stderr,  "usage: inputfile k m coding_technique w (packetsize) (buffersize)\n");
		fprintf(stderr,  "\nChoose one of the following coding techniques: \nreed_sol_van, \nreed_sol_r6_op, \ncauchy_orig, \ncauchy_good, \nliberation, \nblaum_roth, \nliber8tion");
		exit(EXIT_SUCCESS);
	}

	/* Conversion of parameters and error checking */
	if (sscanf(argv[2], "%d", &k) == 0 || k <= 0) {
		fprintf(stderr,  "Invalid value for k\n");
		exit(0);
	}
	if (sscanf(argv[3], "%d", &m) == 0 || m < 0) {
		fprintf(stderr,  "Invalid value for m\n");
		exit(0);
	}
	if (sscanf(argv[5],"%d", &w) == 0 || w <= 0) {
		fprintf(stderr,  "Invalid value for w.\n");
		exit(0);
	}
	if (argc == 6) {
		packetsize = 0;
	}
	else {
		if (sscanf(argv[6], "%d", &packetsize) == 0 || packetsize < 0) {
			fprintf(stderr,  "Invalid value for packetsize.\n");
			exit(0);
		}
	}
	if (argc != 8) {
		buffersize = 0;
	}
	else {
		if (sscanf(argv[7], "%d", &buffersize) == 0 || buffersize < 0) {
			fprintf(stderr, "Invalid value for buffersize\n");
			exit(0);
		}
	}

    return EXIT_SUCCESS;
}

int determine_proper_buffersize(int buffersize,int packetsize,int w,int k){
    int up,down;

    /* Determine proper buffersize by finding the closest valid buffersize to the input value  */
    if (packetsize != 0 && buffersize%(sizeof(int)*w*k*packetsize) != 0) {
        up = buffersize;
        down = buffersize;
        while (up%(sizeof(int)*w*k*packetsize) != 0 && (down%(sizeof(int)*w*k*packetsize) != 0)) {
            up++;
            if (down == 0) {
                down--;
            }
        }
        if (up%(sizeof(int)*w*k*packetsize) == 0) {
            buffersize = up;
        }
        else {
            if (down != 0) {
                buffersize = down;
            }
        }
    }
    else if (packetsize == 0 && buffersize%(sizeof(int)*w*k) != 0) {
        up = buffersize;
        down = buffersize;
        while (up%(sizeof(int)*w*k) != 0 && down%(sizeof(int)*w*k) != 0) {
            up++;
            down--;
        }
        if (up%(sizeof(int)*w*k) == 0) {
            buffersize = up;
        }
        else {
            buffersize = down;
        }
    }

	return buffersize;
}

enum Coding_Technique set_coding_technique(char *argv[],int k,int w,int m,int packetsize){
    enum Coding_Technique tech;		// coding technique (parameter)

    /* Setting of coding technique and error checking */
	if (strcmp(argv[4], "no_coding") == 0) {
		tech = No_Coding;
	}
	else if (strcmp(argv[4], "reed_sol_van") == 0) {
		tech = Reed_Sol_Van;
		if (w != 8 && w != 16 && w != 32) {
			fprintf(stderr,  "w must be one of {8, 16, 32}\n");
			exit(0);
		}
	}
	else if (strcmp(argv[4], "reed_sol_r6_op") == 0) {
		if (m != 2) {
			fprintf(stderr,  "m must be equal to 2\n");
			exit(0);
		}
		if (w != 8 && w != 16 && w != 32) {
			fprintf(stderr,  "w must be one of {8, 16, 32}\n");
			exit(0);
		}
		tech = Reed_Sol_R6_Op;
	}
	else if (strcmp(argv[4], "cauchy_orig") == 0) {
		tech = Cauchy_Orig;
		if (packetsize == 0) {
			fprintf(stderr, "Must include packetsize.\n");
			exit(0);
		}
	}
	else if (strcmp(argv[4], "cauchy_good") == 0) {
		tech = Cauchy_Good;
		if (packetsize == 0) {
			fprintf(stderr, "Must include packetsize.\n");
			exit(0);
		}
	}
	else if (strcmp(argv[4], "liberation") == 0) {
		if (k > w) {
			fprintf(stderr,  "k must be less than or equal to w\n");
			exit(0);
		}
		if (w <= 2 || !(w%2) || !is_prime(w)) {
			fprintf(stderr,  "w must be greater than two and w must be prime\n");
			exit(0);
		}
		if (packetsize == 0) {
			fprintf(stderr, "Must include packetsize.\n");
			exit(0);
		}
		if ((packetsize%(sizeof(int))) != 0) {
			fprintf(stderr,  "packetsize must be a multiple of sizeof(int)\n");
			exit(0);
		}
		tech = Liberation;
	}
	else if (strcmp(argv[4], "blaum_roth") == 0) {
		if (k > w) {
			fprintf(stderr,  "k must be less than or equal to w\n");
			exit(0);
		}
		if (w <= 2 || !((w+1)%2) || !is_prime(w+1)) {
			fprintf(stderr,  "w must be greater than two and w+1 must be prime\n");
			exit(0);
		}
		if (packetsize == 0) {
			fprintf(stderr, "Must include packetsize.\n");
			exit(0);
		}
		if ((packetsize%(sizeof(int))) != 0) {
			fprintf(stderr,  "packetsize must be a multiple of sizeof(int)\n");
			exit(0);
		}
		tech = Blaum_Roth;
	}
	else if (strcmp(argv[4], "liber8tion") == 0) {
		if (packetsize == 0) {
			fprintf(stderr, "Must include packetsize\n");
			exit(0);
		}
		if (w != 8) {
			fprintf(stderr, "w must equal 8\n");
			exit(0);
		}
		if (m != 2) {
			fprintf(stderr, "m must equal 2\n");
			exit(0);
		}
		if (k > w) {
			fprintf(stderr, "k must be less than or equal to w\n");
			exit(0);
		}
		tech = Liber8tion;
	}
	else {
		fprintf(stderr,  "Not a valid coding technique. Choose one of the following: reed_sol_van, reed_sol_r6_op, cauchy_orig, cauchy_good, liberation, blaum_roth, liber8tion, no_coding\n");
		exit(0);
	}

    return tech;
}

int get_size_file(char *argv[]){
    int size,i;
    FILE *fp;
    struct stat status;			// finding file size

    /* Open file and error check */
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr,  "Unable to open file.\n");
        exit(0);
    }

    /* Create Coding directory */
    i = mkdir("Coding", S_IRWXU);
    if (i == -1 && errno != EEXIST) {
        fprintf(stderr, "Unable to create Coding directory.\n");
        exit(0);
    }

    /* Determine original size of file */
    stat(argv[1], &status);
    size = status.st_size;

    return size;
}

int determine_newsize_by_packetsize(int size,int packetsize,int buffersize,int k,int w){
    /* Find new size by determining next closest multiple */
	int test1=0,test2=0,newsize;

	if (packetsize != 0) {
		if (size%(k*w*packetsize*sizeof(int)) != 0) {
			while (newsize%(k*w*packetsize*sizeof(int)) != 0){
				newsize++;
				test1++;
			}
		}
	}
	else {
		if (size%(k*w*sizeof(int)) != 0) {
			while (newsize%(k*w*sizeof(int)) != 0)
				newsize++;
		}
	}

	if (buffersize != 0) {
		while (newsize%buffersize != 0) {
			newsize++;
			test2++;
		}
	}
    //printf("test1 = %d\ntest2 = %d\n",test1,test2);

    return newsize;
}

int determine_readins_number(int size,int newsize,int buffersize,char *block,int *blocksize,int k){
    int readins;

    /* Allow for buffersize and determine number of read-ins */
	if (size > buffersize && buffersize != 0) {
        //判断有什么用？
		if (newsize%buffersize != 0) {
			readins = newsize/buffersize;
		}
		else {
			readins = newsize/buffersize;
		}
		block = (char *)malloc(sizeof(char)*buffersize);
		*blocksize = buffersize/k;
	}
	else {
		readins = 1;
		buffersize = size;
		block = (char *)malloc(sizeof(char)*newsize);
	}

	return readins;
}

int read_inputfile_buffersize(char *block,int size,int buffersize,FILE *fp){
    int total = 0,extra,i;

    /* Check if padding is needed, if so, add appropriate number of zeros */
    if (total < size && total+buffersize <= size) {
        total += jfread(block, sizeof(char), buffersize, fp);
    }
    else if (total < size && total+buffersize > size) {
        extra = jfread(block, sizeof(char), buffersize, fp);
        for (i = extra; i < buffersize; i++) {
            block[i] = '0';
        }
    }
    else if (total == size) {
        for (i = 0; i < buffersize; i++) {
            block[i] = '0';
        }
    }

    return total;
}

int write_data_to_files(FILE *fp,int k,char **data,int blocksize,char *fname,int n){
    int i;
    FILE *fp2;
    /* Creation of file name variables */
	char *s1, *s2;
	int md;
	char *curdir;

    for	(i = 1; i <= k; i++) {
        if (fp == NULL) {
            bzero(data[i-1], blocksize);
        } else {
            sprintf(fname, "%s/Coding/%s_k%0*d%s", curdir, s1, md, i, s2);
            if (n == 1) {
                fp2 = fopen(fname, "wb");
            }
            else {
                fp2 = fopen(fname, "ab");
            }
            fwrite(data[i-1], sizeof(char), blocksize, fp2);
            fclose(fp2);
        }

    }

    return EXIT_SUCCESS;
}


int jfread(void *ptr, int size, int nmembers, FILE *stream){
      int nd;
      int *li, i;
      if (stream != NULL) return fread(ptr, size, nmembers, stream);

      nd = size/sizeof(int);
      li = (int *) ptr;
      for (i = 0; i < nd; i++) li[i] = mrand48();
      return size;
}

/* is_prime returns 1 if number if prime, 0 if not prime */
int is_prime(int w) {
	int prime55[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,
	    73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,
		    181,191,193,197,199,211,223,227,229,233,239,241,251,257};
	int i;
	for (i = 0; i < 55; i++) {
		if (w%prime55[i] == 0) {
			if (w == prime55[i]) return 1;
			else { return 0; }
		}
	}
}

/* Handles ctrl-\ event */
void ctrl_bs_handler(int dummy,int n,int readins,enum Coding_Technique method) {
	time_t mytime;
	mytime = time(0);
	fprintf(stderr, "\n%s\n", ctime(&mytime));
	fprintf(stderr, "You just typed ctrl-\\ in encoder.c.\n");
	fprintf(stderr, "Total number of read ins = %d\n", readins);
	fprintf(stderr, "Current read in: %d\n", n);
	fprintf(stderr, "Method: %s\n\n", Methods[method]);
	signal(SIGQUIT, ctrl_bs_handler);
}
