#include <Empire/util.h>
#include <SDL3/SDL.h>

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