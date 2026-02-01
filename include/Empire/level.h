#pragma once
#include "Empire/types.h"
struct emp_asset_t;

#define emp_sublevel_capacity 16
#define emp_value_buffer_capacity 512 * 512
#define emp_entities_capacity 2048
#define emp_teleporter_capacity 32

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

typedef struct emp_value_grid_t
{
	float grid_size;
	u32 grid_width;
	u32 grid_height;
	u8* entries;
	size_t count;
} emp_value_grid_t;

typedef struct emp_tiles_data_t
{
	char* tilemap;
	emp_tile_desc_t* values;
	size_t count;
} emp_tiles_data_t;

typedef struct emp_sublevel_t
{
	emp_vec2_t offset;

	emp_tiles_data_t tiles;
	emp_value_grid_t values;
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

typedef enum emp_entity_type {
	emp_entity_type_player,
	emp_entity_type_spawner,
	emp_entity_type_boss,
} emp_entity_type;

typedef enum emp_behaviour_type {
	emp_behaviour_type_none,
	emp_behaviour_type_roamer,
	emp_behaviour_type_chaser,
} emp_behaviour_type;

typedef struct emp_level_entity_t
{
	emp_entity_type type;
	emp_behaviour_type behaviour;
	uint32_t weapon_index;
	float x;
	float y;
	float w;
	float h;
} emp_level_entity_t;

typedef struct emp_level_entities_list_t
{
	float grid_size;

	emp_level_entity_t entries[emp_entities_capacity];
	u32 count;
} emp_level_entities_list_t;

typedef struct emp_level_teleporter_t
{
	u64 id;

	float x;
	float y;
	float w;
	float h;

	u64 other;
} emp_level_teleporter_t;

typedef struct emp_level_teleporter_list_t
{
	emp_level_teleporter_t entries[emp_teleporter_capacity];
	u32 count;
}emp_level_teleporter_list_t;

typedef struct emp_level_asset_t
{
	emp_tile_desc_list_t tiles;
	emp_value_buffer_t values;
	emp_sublevel_list_t sublevels;
	emp_level_entities_list_t entities;
	emp_level_teleporter_list_t teleporters;
} emp_level_asset_t;

u32 emp_level_query(emp_level_asset_t* level, emp_entity_type type, u32 offset);
emp_level_entity_t* emp_level_get(emp_level_asset_t* level, size_t at);

u32 emp_level_teleporter_list_find(emp_level_teleporter_list_t* set, u64 id);

void emp_load_level_asset(struct emp_asset_t* asset);
void emp_unload_level_asset(struct emp_asset_t* asset);
