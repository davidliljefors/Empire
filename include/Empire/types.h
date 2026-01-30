#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;


typedef struct emp_buffer
{
    u64 size;
    u8* data;
} emp_buffer;

typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct emp_generated_assets_o emp_generated_assets_o;


typedef struct emp_update_args_t
{
	emp_generated_assets_o* assets;
	SDL_Renderer* r;
	float dt;
} emp_update_args_t;

typedef struct {
	u32 width;
	u32 height;
	SDL_Texture* texture;
} emp_texture_t;