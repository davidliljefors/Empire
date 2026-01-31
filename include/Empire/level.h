#pragma once

struct emp_asset_t;

#define emp_sublevel_capacity 16
#define emp_value_buffer_capacity 512 * 512

typedef struct emp_value_buffer_t
{
	u8 values[emp_value_buffer_capacity];
	size_t count;
} emp_value_buffer_t;

typedef struct emp_tile_desc_t
{
	emp_vec2_t src;
	emp_vec2_t dst;
} emp_tile_desc_t;

typedef struct emp_values_slice_t
{
	u8* values;
	size_t count;
} emp_values_slice_t;

typedef struct emp_tile_desc_slice_t
{
	emp_tile_desc_t* values;
	size_t count;
} emp_tile_desc_slice_t;

typedef struct emp_sublevel_t
{
	float grid_size;

	emp_tile_desc_slice_t tiles;
	emp_values_slice_t values;
} emp_sublevel_t;

typedef struct emp_sublevel_list_t
{
	emp_sublevel_t entries[emp_sublevel_capacity];
	size_t count;
} emp_sublevel_list_t;

typedef struct emp_tile_desc_list_t
{
	emp_tile_desc_t entries[emp_value_buffer_capacity];
	size_t count;
} emp_tile_desc_list_t;

typedef struct emp_level_asset_t
{
	emp_tile_desc_list_t tiles;
	emp_value_buffer_t values;
	emp_sublevel_list_t sublevels;
} emp_level_asset_t;


void emp_load_level_asset(struct emp_asset_t* asset);
void emp_unload_level_asset(struct emp_asset_t* asset);