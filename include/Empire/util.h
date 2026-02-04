#include "types.h"

const char* emp_concat(const char* root, const char* path);
emp_buffer emp_read_entire_file(const char* path);

emp_buffer emp_allocate_buffer(u64 size);
void emp_free_buffer(emp_buffer* buffer);

emp_compressed_buffer emp_compress_buffer(emp_buffer buffer);
emp_buffer emp_decompress_buffer(emp_compressed_buffer buffer);