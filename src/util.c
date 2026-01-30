#include <Empire/util.h>
#include <SDL3/SDL.h>

const char* emp_concat(const char* root, const char* path) {
    char buffer[260];

    if(root != NULL && strcmp(root, "") == 0)
    {
        return path;
    }

    if(SDL_snprintf(buffer, sizeof(buffer), "%s\\%s", root, path) != NULL) 
	{
        return SDL_strdup(buffer);
    }
    return path;
}

emp_buffer emp_read_entire_file(const char* path) {
    emp_buffer buffer = {0};
    size_t size = 0;
    void* data = SDL_LoadFile(path, &size);
    if (!data) {
        return buffer;
    }
    buffer.size = size;
    buffer.data = (u8*)data;
    return buffer;
}