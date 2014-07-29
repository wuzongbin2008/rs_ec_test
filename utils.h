#ifndef _NF_H_UTILS_H_
#define _NF_H_UTILS_H_

#include <arpa/inet.h>

/******************* strings. bit & nums ********************/
typedef struct nf_obj_key {
	size_t	len;
	char *	key;
} nf_key_t;

#define string2key(str) {sizeof((str))-1, (str)}

/* used for aligning */
#define nf_align(type, size, align) (((size)+(align)-1) & ((type)~((align)-1)))
#define nf_padlen(type, size, align) (nf_align(type, size, align) - (size))
#define nf_align32(size, align) nf_align(uint32_t, (size), (align))
#define nf_align64(size, align) nf_align(uint64_t, (size), (align))
#define nf_padlen32(size, align) nf_padlen(uint32_t, (size), (align))
#define nf_padlen64(size, align) nf_padlen(uint64_t, (size), (align))

/* unit */
#define METRIC_KILO	1024
#define METRIC_MEGA	1048576UL
#define METRIC_GIGA	1073741824UL
#define METRIC_TERA	1099511627776ULL

#define min(x, y)	((x) > (y) ? (y) : (x))
#define max(x, y)	((x) > (y) ? (x) : (y))

/* skip continuous chasracters in a string */
inline char * skip_chr(char ch, char *str);
inline void skip_leading_chr(char ch, nf_key_t *key);

/* parse metric size */
uint64_t parse_size(char *str, size_t len);
/*  */
char * size2string(size_t size);
char * size2string_r(char *buf, size_t count, size_t size);

/* convert between net and host order */
uint64_t ntoh64(uint64_t net);
uint64_t hton64(uint64_t host);

/******************** debug. profile & tuning ***************/
/* select branch. come from linux */
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

/* print brcktrace start at caller */
void print_backtrace();

/************************* time ********************/
/* like: 23/May/2011:15:49:12 +0800 */
void local_timestr(time_t ts, char *str, size_t n);
/* like: Mon, 23 May 2011 08:22:55 GMT */
void gm_timestr(time_t ts, char *str, size_t n);

/********************* file/IO **************************/
/* set flags for fd by using fcntl */
int setup_fd(int fd, long flags);

/*********** OS: version user limit ... ****************/
extern char os_name[];
/* node name */
void get_osname();

/* save current process id in path */
int save_pid(char *path);
/* get pid stored in path */
pid_t get_pid(char *path);

/* user -> uid -> process */
int set_user(char *group, char *user);

/* rlimits */
int set_rlimits(int res, int new);

/* core dumpable */
int enable_coredump();

/************************ FS ***************************/
inline int rec_lock(int fd, int type, off_t off, size_t size);
/* touch a file and lock it(advisory). */
int get_exclusive_file(char *path);

/* wrap of scandir. the last arg is used to recursivly call */
typedef void (*scandir_cb)(char *, void *, void *);
int scan_dir(char *dir, scandir_cb dir_op, void *arg);

/* use statfs to get total & free space (in size_t) */
int get_fs_size(char *mount, size_t *total, size_t *free);


void progress_init(uint64_t total);
void progress_update(int x);
void progress_finish();

ssize_t nwritev(int fd, struct iovec *iov, int iovcnt);

/****************************** NET **************************/
void set_addr(struct sockaddr_in *addr_in, const char *addr, u_short port);
#endif
