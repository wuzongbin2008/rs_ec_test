#ifndef EC_STORAGE_H_INCLUDED
#define EC_STORAGE_H_INCLUDED

#include <stdint.h>

#define DATA_CHUNK_K
#define CODING_CHUNK_M

/* data structure for generating Lookup Table for the code*/
typedef struct {
	int chunkIndex;
	long start;
	long Nbytes;
	char host_ip[CHUNK_HOST_IP_LEN];
} chunk_Info_t;

typedef struct nf_obj_pos {
	uint32_t	volid;
	uint32_t	size;
	uint64_t	offset;
	chunk_info_t chunk_info[EC_CHUNK_N];
} nf_pos_t;

struct hash_entry {
	char						key[PIC_KEY_SIZE];
	uint32_t					flags;
	nf_pos_t					pos[5];
	SLIST_ENTRY(hash_entry)		field;
};

#endif // EC_STORAGE_H_INCLUDED
