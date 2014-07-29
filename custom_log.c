#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include<sys/file.h>
#include <errno.h>
#include <fcntl.h>

#include "custom_log.h"

#define STR_PAD_LEFT			0
#define STR_PAD_RIGHT			1
#define STR_PAD_BOTH			2

/**
	 * @name customLog
	 * @desc 记录自定义日志,请注意日志文件大小问题
	 * @param string filename 记录日志的文件名
	 * @param string msg    错误信息
	 * @param int priority  接受的类型
	 */
void customlog(char *filename,char *msg,char *priority){
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
