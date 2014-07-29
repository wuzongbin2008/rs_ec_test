#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include<sys/file.h>
#include <errno.h>

#define STR_PAD_LEFT			0
#define STR_PAD_RIGHT			1
#define STR_PAD_BOTH			2

static void customlog(char *filename,const char *msg);
char * str_pad ( char *input , int pad_length,char *pad_string,int pad_type );

  static const char *hello_str = "Hello World!\n";
  static const char *hello_path = "/hello";
  char *filename = "/data0/log.txt";

  /**
	 * @name customLog
	 * @desc 记录自定义日志,请注意日志文件大小问题
	 * @param string filename 记录日志的文件名
	 * @param string msg    错误信息
	 * @param int priority  接受的类型
	 */
static void customlog(char *filename,const char *msg){
        int pad_length = 70;
        char pad_string[4] = "*";
        char *br="\r\n",*log_content;
        struct tm *newtime;
        char tmpbuf[50];
        time_t lt1;
        FILE *fp=NULL;//需要注意

        time(&lt1);
        newtime=localtime(&lt1);
        strftime(tmpbuf,128,"%Y-%m-%d %H:%M:%S",newtime);
        //printf("%s\n",tmpbuf);

        log_content = str_pad(tmpbuf,pad_length,pad_string,STR_PAD_BOTH);
        log_content = str_pad(log_content,strlen(log_content)+strlen(br),br,STR_PAD_RIGHT);
        log_content = str_pad(log_content,strlen(log_content)+strlen(msg),msg,STR_PAD_RIGHT);
        log_content = str_pad(log_content,strlen(log_content)+strlen(br),br,STR_PAD_RIGHT);
        printf(log_content);

        fp = fopen(filename, "a+");
        int ret = chmod(filename,S_IRWXG|S_IRWXU|S_IRWXO);
        //printf("chmod  = %d\n",ret);
        //printf("errno.%02d is: %s/n",errno, strerror(errno));
        flock(fp, LOCK_EX);
        fwrite(log_content, sizeof(char),strlen(log_content),fp);
        flock(fp, LOCK_UN);
        fclose(fp);
        free(log_content);
}

/**
	 * @name str_pad
	 * @desc 使用另一个字符串填充字符串为指定长度
	 * @param char * input 输入字符串
	 * @param int pad_length    如果 pad_length 的值是负数，小于或者等于输入字符串的长度，不会发生任何填充。
	 * @param char *pad_str_val  如果填充字符的长度不能被 pad_str_val 整除，那么 pad_str_val 可能会被缩短。
	 * @param int pad_type_val 可选的 pad_type 参数的可能值为 STR_PAD_RIGHT，STR_PAD_LEFT 或 STR_PAD_BOTH
	 */
char * str_pad ( char *input , int pad_length,char *pad_str_val,int pad_type_val ){
        /* Helper variables */
        int  input_len;
        size_t	   num_pad_chars;		/* Number of padding characters (total - input size) */
        char  *result = NULL;		/* Resulting string */
        int	   result_len = 0;		/* Length of the resulting string */
       // char  *pad_str_val [100];	/* Pointer to padding string */
        int    pad_str_len = 1;		/* Length of the padding string */
        //long   pad_type_val = STR_PAD_RIGHT; /* The padding type value */
        int	   i, left_pad=0, right_pad=0;

        input_len = strlen(input);
        pad_str_len = strlen(pad_str_val);

        /* If resulting string turns out to be shorter than input string,
           we simply copy the input and return. */
        if (pad_length <= 0 || (pad_length - input_len) <= 0) {
                result = input;
                //trcpy(result,input);
        }

        if (pad_str_len == 0) {
                printf("Padding string cannot be empty");
                EXIT_FAILURE;
        }

        if (pad_type_val < STR_PAD_LEFT || pad_type_val > STR_PAD_BOTH) {
                printf("Padding type has to be STR_PAD_LEFT, STR_PAD_RIGHT, or STR_PAD_BOTH");
                EXIT_FAILURE;
        }

        num_pad_chars = pad_length - input_len;
        //printf("num_pad_chars = %d\n",num_pad_chars);
        if (num_pad_chars >= INT_MAX) {
                printf("Padding length is too long\n");
                EXIT_FAILURE;
        }

        result = (char *)malloc(input_len + num_pad_chars + 1);

        /* We need to figure out the left/right padding lengths. */
        switch (pad_type_val) {
            case STR_PAD_RIGHT:
                left_pad = 0;
                right_pad = num_pad_chars;
                break;

            case STR_PAD_LEFT:
                left_pad = num_pad_chars;
                right_pad = 0;
                break;

            case STR_PAD_BOTH:
                left_pad = num_pad_chars / 2;
                right_pad = num_pad_chars - left_pad;
                break;
        }

        //strcpy(pad_str_val,pad_string);
        pad_str_len = strlen(pad_str_val);
        //printf("pad_str_len = %d\n",pad_str_len);

        /* First we pad on the left. */
        for (i = 0; i < left_pad; i++)
            result[result_len++] = pad_str_val[i % pad_str_len];

        /* Then we copy the input string. */
        memcpy(result + result_len, input, input_len);
        result_len += input_len;

        /* Finally, we pad on the right. */
        for (i = 0; i < right_pad; i++){
                //printf("i % pad_str_len = %d\n",i % pad_str_len);
                //printf("%d= %c\n",i,pad_str_val[i % pad_str_len]);
                result[result_len++] = pad_str_val[i % pad_str_len];
        }

        result[result_len] = '\0';
       //printf("result = %s\n",result);

        return result;
}

  /*该函数与stat()类似，用于得到文件的属性，将其存入到结构体struct stat中*/
  static int hello_getattr(const char *path, struct stat *stbuf)
  {
            int res = 0;

            memset(stbuf, 0, sizeof(struct stat));   //用于初始化结构体stat

            if (strcmp(path, "/") == 0)
            {
                stbuf->st_mode = S_IFDIR | 0755;     //S_IFDIR 用于说明/为目录，详见
                stbuf->st_nlink = 2;                 //文件链接数
            }
            else if (strcmp(path, hello_path) == 0)
            {
                stbuf->st_mode = S_IFREG | 0444;     //S_IFREG用于说明/hello为常规文件
                stbuf->st_nlink = 1;
                stbuf->st_size = strlen(hello_str);  //设置文件长度为hello_str的长度
            }
            else
                 res = -ENOENT;                      //返回错误信息，没有该文件或目录

            return res;                              //执行成功返回 0
  }

  /*该函数用于读取/目录中的内容，并在/目录下增加了 . .. hello三个目录项*/
 static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
 {
        (void) offset;
        (void) fi;

        if(strcmp(path, "/") != 0)
         return -ENOENT;

        customlog(filename,path);

        filler(buf, ".", NULL, 0);               // 在/目录下增加. 这个目录项
        filler(buf, "..", NULL, 0);              // 增加.. 目录项
        filler(buf, hello_path + 1, NULL, 0);    // 增加hello目录项

        return 0;
 }

 /* fill的定义：
      typedef int (*fuse_fill_dir_t) (void *buf, const char *name, const struct stat *stbuf, off_t off);
      其作用是在readdir函数中增加一个目录项
  */

  /*用于打开hello文件*/
 static int hello_open(const char *path, struct fuse_file_info *fi)
 {
         customlog(filename,"hello_open start");
         if(strcmp(path, hello_path) != 0)
             return -ENOENT;

        if((fi->flags & 3) != O_RDONLY)
             return -EACCES;
        customlog(filename,"hello_open end");
        return 0;
 }

  /*读取hello文件时的操作，它实际上读取的是字符串hello_str的内容*/
 static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
 {
         size_t len;
         (void) fi;
         if(strcmp(path, hello_path) != 0)
             return -ENOENT;

        len = strlen(hello_str);
         if (offset < len) {
             if (offset + size > len)
                 size = len - offset;
             memcpy(buf, hello_str + offset, size);
         } else
             size = 0;

        return size;
 }

  /*注册上面定义的函数*/
  static struct fuse_operations hello_oper =
  {
            .getattr = hello_getattr,
            .readdir = hello_readdir,
            .open    = hello_open,
            .read    = hello_read,
  };

  /*用户只需要调用fuse_main()，剩下的事就交给FUSE了*/
  int main(int argc, char *argv[])
  {
            return fuse_main(argc, argv, &hello_oper, NULL);
  }
