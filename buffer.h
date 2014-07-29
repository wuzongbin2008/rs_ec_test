#ifndef _H_NF_BUFFER_H
#define _H_NF_BUFFER_H

#define NF_PAGE_SIZE	4096


#include <sys/uio.h>
#include "utils.h"
#include "queue.h"

#ifdef BUFSIZ
#define BUFFSIZE BUFSIZ
#else
#define BUFFSIZE 8192
#endif

struct block_frame {
	STAILQ_ENTRY(block_frame)	field;
	void						*data;
}; /* no ref count */

typedef STAILQ_HEAD(block_frame_head, block_frame) bf_head;

typedef struct buffer_pool {
	int							used;
	int							total;
	bf_head						free_buff;
	bf_head						free_frame;

	int							bf_len;
	int							bf_off;
	void 						**bf_page;
} nf_bp_t;

/*********************** Chained buffer ************************/
typedef struct buffer {
	int							len;
	int							off;
	char						*last;
	nf_bp_t						*pool;
	STAILQ_HEAD(, block_frame)	head;
} nf_buff_t;

int			init_buffer_pool(nf_bp_t *pool, int n);
void		shink_buffer_pool(nf_bp_t *pool);
void		destroy_pool(nf_bp_t *pool);

inline void	buffer_init(nf_buff_t *buff, nf_bp_t *pool);

/**********************************************************
 * Expand nf_buff_t to ensure that buffer has enouth space
 *	to hold min(n, PAGESIZE) content.
 */
int			expand_buffer(nf_buff_t *buff, nf_bp_t *pool, size_t n);

/* copy mem from string to a nf_buff_t */
ssize_t		buffer_copy(nf_buff_t *buff, char *src, size_t count);
/**********************************************************
 * Read file indicated by fd in a nf_buff_t 
 *		On success the number of bytes read is returned.
 *			zero indicates end of file or connection closed.
 *		On error -1 is returned, and errno is set by read syscall.
 *			but, EFAULT may be sat if expand buffer failed.
 ******/
ssize_t		buffer_read(int fd, nf_buff_t *buff, size_t count);

/*********************************************************
 * Make io vector which is used by writev from nf_buff_t.
 *		On success, iovcnt is returned.
 *		On error, 0 is returned.
 ************/
int			buffer2iovec(nf_buff_t *buf, struct iovec **iov);

int			buffer_printf(nf_buff_t *buff, size_t size, char *fmt, ...);

void		free_buffer(nf_buff_t *buff, nf_bp_t *pool);

typedef struct simple_buff {
	int							len;
	int							off;
	char						*ptr;
	struct block_frame			*bf;
} nf_sbuff_t;

int				init_simple_buff(nf_sbuff_t *sbuf, nf_bp_t *pool);
void			free_simple_buff(nf_sbuff_t *sbuf, nf_bp_t *pool);
ssize_t			sbuffer_read(int fd, nf_sbuff_t *buf, size_t count);
//#define			sbuffer_push(buf, count) (buf)->off += (count)
inline void		sbuffer_push(nf_sbuff_t *buf, size_t count);
inline void		sbuffer_pop(nf_sbuff_t *buf, size_t count);
/********************************************************
 * Like snprintf
 **/
inline size_t	sbuffer_printf(nf_sbuff_t *buff, size_t size, char *fmt, ...);


/************************** simple dictionary ***************/
#define NF_KVT_HASH_LEN 32
typedef struct kv_pair {
	nf_key_t		key;
	nf_key_t		val;
} nf_kvp_t;

typedef struct kv_table {
	int				n;
	nf_kvp_t		*hash_table[NF_KVT_HASH_LEN];
} nf_kvtb_t;

void		init_kv_table(nf_kvtb_t *table);
int			tb_insert(nf_kvtb_t *table, nf_kvp_t *elm);
void		tb_delete(nf_kvtb_t *table, nf_key_t *key);
nf_kvp_t *	tb_find(nf_kvtb_t *table, nf_key_t *key);

typedef struct kv_pair2 {
	nf_key_t		key;
	nf_key_t		val;
} nf_kvp2_t;

typedef struct kv_table2 {
	int				n;
	nf_kvp2_t		hash_table[NF_KVT_HASH_LEN];
} nf_kvtb2_t;

void		init_kv_table2(nf_kvtb2_t *table);
int			tb_insert2(nf_kvtb2_t *table, nf_key_t *key, nf_key_t *val);
void		tb_delete2(nf_kvtb_t *table, nf_kvp_t *elm);
nf_key_t *	tb_find2(nf_kvtb2_t *table, nf_key_t *key);

typedef struct block_pool {
	int				slot;
	int				bucket;
	void			*ptr;
	void			**list;
	size_t			off;
	int				objs;
} nf_pool_t;

#define NF_POOL_INITIALIZER {		\
	.slot		= 0,				\
	.bucket		= 0,				\
	.ptr		= NULL,				\
	.list		= NULL,				\
	.off		= 0,				\
	.objs		= 0,				\
}

int init_block_pool(nf_pool_t *pool);
void destroy_block_pool(nf_pool_t *pool);
void * block_pool_alloc(nf_pool_t *pool, size_t count);

#define block_pool_size(pool) ((pool)->slot * NF_PAGE_SIZE)
#define block_pool_count(pool)	((pool)->objs)

#endif
