#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <string.h>
#include <fcntl.h>

#include "unit_test.h"
#include "queue.h"
#include "buffer.h"

void linker_table(){
    nf_buff_t *buff1=NULL;
    nf_bp_t	*buffer_pool = NULL;
    struct block_frame *bf, *tvar;
    int ret,i=0,n;

    buffer_pool = (nf_bp_t *)calloc(1, sizeof(nf_bp_t));
    printf("sizeof(nf_bp_t) = %d\n",sizeof(nf_bp_t));

    //first buff
    buff1 = (nf_buff_t *)malloc(sizeof(nf_buff_t));
    printf("sizeof(nf_buff_t) = %d\n\n",sizeof(nf_buff_t));
    buff1->len = 1;
	buff1->off = 1;
	buff1->last = "first buff";
	buff1->pool = buffer_pool;
	STAILQ_INIT(&buff1->head);

	for(n=0;n<3;n++){
        ret = try_expand_buffer(buff1, NF_PAGE_SIZE);
	}

	for ( (bf) = STAILQ_FIRST((&buff1->head)); (bf) && ( (tvar) = STAILQ_NEXT((bf), field), 1 ); (bf) = (tvar) ){
        printf("buff %d\n",++i);
	}

	printf("ret = %d\n",ret);
}

void get_param(int argc,char *argv[]){
        int result;
    char *conf_file = "./t.conf";

    opterr = 0;  //使getopt不行stderr输出错误信息

    while( (result = getopt(argc, argv, "hf::c")) != -1 )
     {
            switch(result)
            {
               case 'h':
                    printf("option=h, optopt=%c, optarg=%s\n", optopt, optarg);
                    break;
               case 'f':
                    printf("option=f, optopt=%c, optarg=%s\n", optopt, optarg);
                    break;
               case 'c':
                    printf("option=c, optopt=%c, optarg=%s\n", optopt, optarg);
                    break;
               case '?':
                     printf("result=?, optopt=%c, optarg=%s\n", optopt, optarg);
                     break;
               default:
                    printf("default, result=%c\n",result);
                    break;
            }
            printf("argv[%d]=%s\n", optind, argv[optind]);
     }

     printf("result=-1, optind=%d\n", optind);   //看看最后optind的位置

     for(result = optind; result < argc; result++)
          printf("-----argv[%d]=%s\n", result, argv[result]);

    //看看最后的命令行参数，看顺序是否改变了哈。
     for(result = 1; result < argc; result++)
           printf("\nat the end-----argv[%d]=%s\n", result, argv[result]);
}

void print_file_Statfs(){
    struct statfs diskInfo;

    statfs("/home/", &diskInfo);
    unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;   //总的字节数，f_blocks为block的数目
    printf("Total_size = %llu B = %llu KB = %llu MB = %llu GB\n",
        totalsize, totalsize>>10, totalsize>>20, totalsize>>30);

    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小
    unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //可用空间大小
    printf("Disk_free = %llu MB = %llu GB\nDisk_available = %llu MB = %llu GB\n",
        freeDisk>>20, freeDisk>>30, availableDisk>>20, availableDisk>>30);
}

off_t get_file_offset(){
    int fd;
    off_t off;
    char *file = "./t.txt";
    fd = open(file,O_RDONLY);
    off = lseek(fd,0,SEEK_END);

    return off;
}

int notfs_hash(){
    int PIC_STO_HASH_SHIFT = 24;
    int PIC_KEY_PREFIX = 8,i;
    char *str = "88bbb9eb5cb5a98ac9b5cd535c42ee24000455ac.bmi";
    static char map_table[128];
    int hv = 0;
    char c = 0;

	for (i='0'; i<='9'; i++) {
		map_table[i] = c++;
	}
	for (i='a'; i<='z'; i++) {
		map_table[i] = c++;
	}

    for (i=0; i<PIC_KEY_PREFIX; i++) {
		c = str[i];
		hv |= map_table[c];
		hv = hv << 4;
	}
	hv = hv >> 4;

	i = (1 << PIC_STO_HASH_SHIFT) - 1;

	return hv & i;
}

