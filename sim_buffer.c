#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "buffer.h"

int init_buffer_pool(nf_bp_t *pool, int n)
{
	return 0;
}

void shink_buffer_pool(nf_bp_t *pool)
{
	return;
}

void destroy_pool(nf_bp_t *pool)
{
	return;
}

struct block_frame * get_free_block(nf_bp_t *pool)
{
	struct block_frame *bf;
	//print_backtrace();

	bf = calloc(1, sizeof(struct block_frame));
	if (bf == NULL) return NULL;

	bf->data = malloc(NF_PAGE_SIZE);
	if (bf->data) return bf;

	free(bf);
	return NULL;
}

inline void free_block(struct block_frame *bf, nf_bp_t *pool)
{
	if (bf == NULL) return;
	if (bf->data) free(bf->data);
	free(bf);
	return;
}

inline void buffer_init(nf_buff_t *buff, nf_bp_t *pool)
{
	buff->len = 0;
	buff->off = 0;
	buff->last = NULL;
	buff->pool = pool;
	STAILQ_INIT(&buff->head);
	return;
}

/* NOTE: buff must draind before expand
 *		 always alloc ONE page a time regardness of n
 ***/
int expand_buffer(nf_buff_t *buff, nf_bp_t *pool, size_t n)
{
	int count;
	struct block_frame *bf;
	int len,off;

    len = buff->len;
    off = buff->off;
	count = 0;// buff->len - buff->off;
	if (count >= n) return 0;
	bf = get_free_block(pool);
	if (unlikely(bf == NULL)) return -1;

	STAILQ_INSERT_TAIL(&buff->head, bf, field);

	buff->len += NF_PAGE_SIZE;
	buff->last = bf->data;

	return 0;
}

inline int try_expand_buffer(nf_buff_t *buff, size_t count)
{
	int n,len,off;

    len = buff->len;
    off = buff->off;
	n = 0;// buff->len - buff->off;
	if (n > 0) return 0;

	if (unlikely(count <= 0)) {
		return -1;
	}

	return expand_buffer(buff, buff->pool, NF_PAGE_SIZE);
}

ssize_t buffer_read(int fd, nf_buff_t *buff, size_t count)
{
	size_t n;
	ssize_t ret;

	ret = try_expand_buffer(buff, NF_PAGE_SIZE);
	if (ret != 0) {
		errno = EFAULT;		// buff is out of address space
		return -1;
	}

	n = buff->len - buff->off;
	ret = read(fd, buff->last, min(n, count));
	if (ret > 0) {
		buff->last += ret;
		buff->off += ret;
	}

	return ret;
}

ssize_t buffer_copy(nf_buff_t *buff, char *src, size_t count)
{
	size_t n;
	ssize_t ret;

	ret = try_expand_buffer(buff, NF_PAGE_SIZE);
	if (ret != 0) {
		errno = EFAULT;		// buff is out of address space
		return -1;
	}

	n = buff->len - buff->off;
	ret = min(n, count);

	memcpy(buff->last, src, ret);
	buff->last += ret;
	buff->off += ret;
	return ret;
}

int buffer2iovec(nf_buff_t *buff, struct iovec **iov)
{
	int i = 0, count;
	struct iovec * vec;
	struct block_frame *bf;

	count = buff->off / NF_PAGE_SIZE + 1;
	vec = calloc(count, sizeof(struct iovec));
	if (unlikely(vec == NULL)) {
		return 0;
	}

	STAILQ_FOREACH(bf, &buff->head, field) {
		vec[i].iov_base = bf->data;
		vec[i].iov_len = NF_PAGE_SIZE;
		i++;
	}
	vec[count-1].iov_len = buff->off % NF_PAGE_SIZE;

	*iov = vec;
	return i;
}

int buffer_printf(nf_buff_t *buff, size_t size, char *fmt, ...)
{
	int ret;
	va_list ap;
	char line[1024];

	if (size > 1024) return -1;

	va_start(ap, fmt);
	ret = vsnprintf(line, size, fmt, ap);
	va_end(ap);
	if (ret <= 0) return ret;

	if (try_expand_buffer(buff, ret) != 0) return -1;

	return buffer_copy(buff, line, ret);
}

void free_buffer(nf_buff_t *buff, nf_bp_t *pool)
{
	struct block_frame *bf, *tvar;

	STAILQ_FOREACH_SAFE(bf, &buff->head, field, tvar) {
		free_block(bf, pool);
	}

	buffer_init(buff, pool);
	return;
}

int init_simple_buff(nf_sbuff_t *sbuf, nf_bp_t *pool)
{
	struct block_frame *bf;

	bf = get_free_block(pool);
	if (unlikely(bf == NULL)) return -1;

	sbuf->bf = bf;
	sbuf->len = NF_PAGE_SIZE;
	sbuf->off = 0;
	sbuf->ptr = bf->data;

	return 0;
}

void free_simple_buff(nf_sbuff_t *sbuf, nf_bp_t *pool)
{
	if (sbuf->bf) {
		free_block(sbuf->bf, pool);
	}
	sbuf->bf = NULL;
	sbuf->len = 0;
	sbuf->off = 0;
	sbuf->ptr = NULL;
	return;
}

inline void sbuffer_pop(nf_sbuff_t *buf, size_t count)
{
	buf->len -= count;
	buf->off -= count;
	buf->ptr += count;
}

/* use inline func instead of define to gen profile info */
inline void sbuffer_push(nf_sbuff_t *buf, size_t count)
{
	//print_backtrace();
	buf->off += count;
}
//#define sbuffer_push(buf, count) (buf)->off += (count)

#define _____XXXXX
#ifndef _____XXXXX
ssize_t sbuffer_read(int fd, nf_sbuff_t *buf, size_t count)
{
	size_t n;
	ssize_t ret;

	n = buf->len - buf->off;
	if (n == 0) {
		if (buf->off > 0) return buf->off;
		return 0;
	}

	ret = read(fd, buf->ptr + buf->off, min(n, count));
	if (ret > 0) {
		sbuffer_push(buf, ret);
	}
	return ret;
}

#else

/* reduce the read syscall */
inline ssize_t sbuffer_read(int fd, nf_sbuff_t *buf, size_t count)
{
	size_t n;
	ssize_t ret;

	if (buf->off > 0) return buf->off;

	n = buf->len - buf->off;
	if (n == 0) {
		errno = EFAULT;
		return -1;
	}

	ret = read(fd, buf->ptr + buf->off, min(n, count));
	if (ret > 0) {
		sbuffer_push(buf, ret);
	}
	return ret;
}
#endif

size_t sbuffer_printf(nf_sbuff_t *buff, size_t size, char *fmt, ...)
{
	va_list ap;
	size_t left, ret;

	left = buff->len - buff->off;
	if (left == 0) return 0;
	left = min(left, size);

	va_start(ap, fmt);
	ret = vsnprintf(buff->ptr + buff->off, left, fmt, ap);
	va_end(ap);

	sbuffer_push(buff, ret);
	return ret;
}

/* from linu dcache.c */
inline static int hash(nf_key_t *key)
{
	int i;
	char c;
	int hv = 0;

	for (i=0; i<key->len; i++) {
		c = key->key[i];
		hv = (hv + (c << 4) + (c >> 4)) * 11;
	}

	return hv & (NF_KVT_HASH_LEN -1);
}

nf_kvp_t * NF_KVP_NULL = (nf_kvp_t *)0;
nf_kvp_t * NF_KVP_DELETED = (nf_kvp_t *)(-1);

/* linear probe */
/* BUG: deleted element may can not be reused */
int _hash_lookup(nf_kvtb_t *table, nf_key_t *key, nf_kvp_t ***obj)
{
	int hv;
	nf_kvp_t **ptr;
	nf_kvp_t **dummy, **bound;

	hv = hash(key);
	dummy = ptr = table->hash_table + hv;
	bound = table->hash_table + NF_KVT_HASH_LEN;

	if (unlikely(obj == NULL)) {
		do {
			if (unlikely(*ptr == NF_KVP_NULL)) {
				return 0;
			}
			if (unlikely(*ptr == NF_KVP_DELETED)) continue;
			if ((*ptr)->key.len == key->len) {
				if (memcmp((*ptr)->key.key, key->key, key->len) == 0) {
					return 1;
				}
			}
			if (unlikely(++ptr == bound)) {
				ptr = table->hash_table;
			}
		} while (ptr != dummy);
	} else {
		do {
			if (unlikely(*ptr == NF_KVP_NULL)) {
				*obj = ptr;
				return 0;
			}
			if ((*ptr)->key.len == key->len) {
				if (memcmp((*ptr)->key.key, key->key, key->len) == 0) {
					*obj = ptr;
					return 1;
				}
			}
			if (unlikely(*ptr == NF_KVP_DELETED)) continue;
			if (unlikely(++ptr == bound)) {
				ptr = table->hash_table;
			}
		} while (ptr != dummy);
	}

	/* table is full */
	return -1;
}

#define NF_E_HT_FULL		-301
#define NF_E_DUP_KEY		-302

int tb_insert(nf_kvtb_t *table, nf_kvp_t *elm)
{
	int ret;
	nf_kvp_t **obj = NULL;

	ret = _hash_lookup(table, &elm->key, &obj);
	if (ret == -1) return NF_E_HT_FULL;
	if (ret == 1) return NF_E_DUP_KEY;
	*obj = elm;
	table->n++;

	return 0;
}

nf_kvp_t * tb_find(nf_kvtb_t *table, nf_key_t *key)
{
	int ret;
	nf_kvp_t **obj = NULL;

	ret = _hash_lookup(table, key, &obj);
	if (ret == 1) return *obj;
	return NULL;
}

void tb_delete(nf_kvtb_t *table, nf_key_t *key)
{
	int ret;
	nf_kvp_t **obj = NULL;
	ret = _hash_lookup(table, key, &obj);

	if (ret == 1) *obj = (nf_kvp_t *)(-1);
}

void tb_delete2(nf_kvtb_t *table, nf_kvp_t *elm)
{
	int hv;
	nf_kvp_t **ptr;
	nf_kvp_t **dummy;//, **bound;

	hv = hash(&elm->key);
	dummy = ptr = table->hash_table + hv;
	//bound = table->hash_table + NF_KVT_HASH_LEN;

	do {
		if (unlikely(*ptr == NULL)) {
			return;
		}
		if (*ptr == elm) {
			*ptr = NF_KVP_DELETED;
		}
	} while (ptr != dummy);
	return;
}

void init_kv_table(nf_kvtb_t *table)
{
	table->n = 0;
	memset(table->hash_table, 0, NF_KVT_HASH_LEN * sizeof(nf_key_t *));
	return;
}

void init_kv_table2(nf_kvtb2_t *table)
{
	table->n = 0;
	memset(table->hash_table, 0, NF_KVT_HASH_LEN * sizeof(nf_key_t));
}

int _hash_lookup2(nf_kvtb2_t *table, nf_key_t *key, nf_kvp2_t **obj)
{
	int hv;
	nf_kvp2_t *ptr;
	nf_kvp2_t *dummy, *bound;

	hv = hash(key);
	dummy = ptr = table->hash_table + hv;
	bound = table->hash_table + NF_KVT_HASH_LEN;

	do {
		if (ptr->key.len == key->len) {
			if (memcmp(ptr->key.key, key->key, key->len) == 0) {
				*obj = ptr;
				return 1;
			}
		}
		if (ptr->key.len == 0) {
			*obj = ptr;
			return 0;
		}
		if (unlikely(++ptr == bound)) {
			ptr = table->hash_table;
		}
	} while (ptr != dummy);

	/* table is full */
	return -1;
}

int tb_insert2(nf_kvtb2_t *table, nf_key_t *key, nf_key_t *val)
{
	int ret;
	nf_kvp2_t *obj;

	ret = _hash_lookup2(table, key, &obj);
	if (ret < 0) return -1;

	obj->key.key = key->key;
	obj->key.len = key->len;
	obj->val.key = val->key;
	obj->val.len = val->len;
	table->n ++;

	return 0;
}

nf_key_t * tb_find2(nf_kvtb2_t *table, nf_key_t *key)
{
	int ret;
	nf_kvp2_t *obj;

	ret = _hash_lookup2(table, key, &obj);
	if (ret != 1) return NULL;

	return &obj->val;
}

static int expand_block_pool(nf_pool_t *pool)
{
	void	*ptr;
	void	**list;
	int		bucket;

	bucket = pool->bucket;
	if (bucket == 0) bucket = 512;
	if (pool->slot == pool->bucket) bucket += 512;

	list = realloc(pool->list, bucket * sizeof(void *));
	if (list == NULL) return -1;
	pool->list = list;
	pool->bucket = bucket;

	ptr = malloc(NF_PAGE_SIZE);
	if (ptr == NULL) return -1;

	list[pool->slot] = ptr;
	pool->ptr = ptr;
	pool->slot++;
	pool->off = 0;

	return 0;
}

int init_block_pool(nf_pool_t *pool)
{
	pool->slot = 0;
	pool->off = NF_PAGE_SIZE;

	return expand_block_pool(pool);
}

void destroy_block_pool(nf_pool_t *pool)
{
	int i;

	if (pool->slot == 0) return;

	for (i=0; i<pool->slot; i++) {
		if (pool->list[i]) free(pool->list[i]);
	}
	if (pool->list) free(pool->list);

	pool->slot = 0;
	pool->bucket = 0;
	pool->off = NF_PAGE_SIZE;
	pool->ptr = NULL;
	pool->list = NULL;
	return;
}

void * block_pool_alloc(nf_pool_t *pool, size_t count)
{
	int ret;
	void *ptr;
	size_t len;

	len = nf_align(size_t, count, 8);
	if (len > NF_PAGE_SIZE) return NULL;

	if (len > (NF_PAGE_SIZE - pool->off)) {
		ret = expand_block_pool(pool);
		if (ret == -1) return NULL;
	}

	ptr = pool->ptr + pool->off;
	pool->off += len;
	pool->objs ++;

	return ptr;
}


#ifdef __UNITEST_/*{{{*/
int main(int argc, char *argv[])
{
	int ret;
	nf_kvp_t kvp1 = {
		string2key("key1"),
		string2key("val1"),
	};
	nf_kvp_t kvp2 = {
		string2key("ke2"),
		string2key("val2"),
	};
	nf_kvp_t kvp3 = {
		string2key("key3"),
		string2key("val3"),
	};
	nf_kvp_t *kvp;

	nf_kvtb_t table;
	init_kv_table(&table);
	ret = tb_insert(&table, &kvp1);
	printf("ret: %d\n", ret);
	ret = tb_insert(&table, &kvp2);
	printf("ret: %d\n", ret);
	ret = tb_insert(&table, &kvp3);
	printf("ret: %d\n", ret);

	kvp = tb_find(&table, &kvp3.key);
	printf("%s\n", kvp->val.key);
	kvp = tb_find(&table, &kvp2.key);
	printf("%s\n", kvp->val.key);
	kvp = tb_find(&table, &kvp1.key);
	printf("%s\n", kvp->val.key);

	nf_kvtb2_t table2;
	nf_key_t * val;
	init_kv_table2(&table2);
	ret = tb_insert2(&table2, &kvp1.key, &kvp1.val);
	printf("ret: %d\n", ret);
	ret = tb_insert2(&table2, &kvp2.key, &kvp2.val);
	printf("ret: %d\n", ret);
	ret = tb_insert2(&table2, &kvp3.key, &kvp3.val);
	printf("ret: %d\n", ret);

	val = tb_find2(&table2, &kvp3.key);
	printf("%s\n", val->key);
	val = tb_find2(&table2, &kvp2.key);
	printf("%s\n", val->key);
	val = tb_find2(&table2, &kvp1.key);
	printf("%s\n", val->key);

	return 0;
}

#endif/*}}}*/
