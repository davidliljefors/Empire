#pragma once
#include "types.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline u64 hash_murmur3(const u8* data, u64 len)
{
	u64 h = 0;
	const u64 c1 = 0x87c37b91114253d5ULL;
	const u64 c2 = 0x4cf5ad432745937fULL;

	const u64 nblocks = len / 8;
	const u64* blocks = (const u64*)data;

	for (u64 i = 0; i < nblocks; i++) {
		u64 k = blocks[i];
		k *= c1;
		k = (k << 31) | (k >> 33);
		k *= c2;

		h ^= k;
		h = (h << 27) | (h >> 37);
		h = h * 5 + 0x52dce729;
	}

	const u8* tail = data + nblocks * 8;
	u64 k = 0;

	switch (len & 7) {
	case 7: k ^= (u64)tail[6] << 48; /* fallthrough */
	case 6: k ^= (u64)tail[5] << 40; /* fallthrough */
	case 5: k ^= (u64)tail[4] << 32; /* fallthrough */
	case 4: k ^= (u64)tail[3] << 24; /* fallthrough */
	case 3: k ^= (u64)tail[2] << 16; /* fallthrough */
	case 2: k ^= (u64)tail[1] << 8; /* fallthrough */
	case 1:
		k ^= (u64)tail[0];
		k *= c1;
		k = (k << 31) | (k >> 33);
		k *= c2;
		h ^= k;
	}

	h ^= (u64)len;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ULL;
	h ^= h >> 33;

	return h;
}

static inline u64 emp_hash_data(const emp_buffer buffer)
{
	return hash_murmur3(buffer.data, (u64)buffer.size);
}

static inline u64 hash_str(const char* str)
{
	return hash_murmur3((const u8*)str, strlen(str));
}
