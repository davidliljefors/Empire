#include <Empire/util.h>
#include <SDL3/SDL.h>
#include <Empire/lz4.h>

const char* emp_concat(const char* root, const char* path)
{
	char buffer[260];

	if (root != NULL && strcmp(root, "") == 0) {
		return SDL_strdup(path);
	}

	if (SDL_snprintf(buffer, sizeof(buffer), "%s/%s", root, path) != 0) {
		return SDL_strdup(buffer);
	}
	return SDL_strdup(path);
}

emp_buffer emp_read_entire_file(const char* path)
{
	emp_buffer buffer = { 0 };
	size_t size = 0;
	void* data = SDL_LoadFile(path, &size);
	if (!data) {
		SDL_Log("Failed to read file: %s - %s", path, SDL_GetError());
		return buffer;
	}
	buffer.size = size;
	buffer.data = (u8*)data;
	return buffer;
}

emp_buffer emp_allocate_buffer(u64 size)
{
	emp_buffer buffer;
	buffer.data = SDL_malloc(size);
	buffer.size = size;
	SDL_memset(buffer.data, 0, size);
	return buffer;
}

void emp_free_buffer(emp_buffer* buffer)
{
	if (buffer->data)
	{
		SDL_free(buffer->data);
		buffer->size = 0;
		buffer->data = 0;
	}
}

emp_compressed_buffer emp_compress_buffer(emp_buffer buffer)
{
	emp_compressed_buffer result = {0};

    int max_compressed_size = LZ4_compressBound((int)buffer.size);
    
    result.data = (u8*)SDL_malloc(max_compressed_size);

    int actual_compressed_size = LZ4_compress_default(
        (const char*)buffer.data, 
        (char*)result.data, 
        (int)buffer.size, 
        max_compressed_size
    );

	result.compressed_size = (u64)actual_compressed_size;
	result.original_size = buffer.size;
	result.data = (u8*)SDL_realloc(result.data, result.compressed_size);

    return result;
}

emp_buffer emp_decompress_buffer(emp_compressed_buffer buffer)
{
	emp_buffer result = {0};

    result.data = (u8*)SDL_malloc(buffer.original_size);

    int decompression_result = LZ4_decompress_safe(
        (const char*)buffer.data, 
        (char*)result.data, 
        (int)buffer.compressed_size, 
        (int)buffer.original_size
    );

    result.size = (u64)decompression_result;

    return result;
}