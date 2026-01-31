#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

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
	float dt;
	double global_time;
} emp_update_args_t;

typedef struct
{
	float width;
	float height;
	u32 atlas_size;
	u32 rows;
	u32 columns;
	SDL_Texture* texture;
} emp_texture_t;


typedef struct emp_vec2_t
{
	float x;
	float y;
} emp_vec2_t;
