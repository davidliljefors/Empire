
#include <Empire/assets.h>
#include <Empire/yyjson.h>
#include <SDL3/SDL_stdinc.h>

#define emp_sublevel_capacity 16
#define emp_value_buffer_capacity 512 * 512

typedef struct emp_value_buffer_t
{
	u8 values[emp_value_buffer_capacity];
	size_t count;
} emp_value_buffer_t;

typedef struct emp_values_slice_t
{
	u8* values;
	size_t count;
} emp_values_slice_t;

typedef struct emp_tile_desc_t {
	emp_vec2_t src;
	emp_vec2_t dst;
}emp_tile_desc_t;

typedef struct emp_sublevel_t
{
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
}emp_tile_desc_list_t;

typedef struct emp_level_t
{
	emp_value_buffer_t values;
	emp_sublevel_list_t sublevels;
} emp_level_t;

emp_sublevel_t* emp_level_create_sublevel(emp_level_t* level, emp_values_slice_t* values)
{
	emp_sublevel_t* sublevel = level->sublevels.entries + level->sublevels.count;
	level->sublevels.count = level->sublevels.count + 1;
	sublevel->values = *values;
	return sublevel;
}

emp_values_slice_t emp_level_reserve_values(emp_level_t* level, size_t count)
{
	u8* data = level->values.values + level->values.count;
	level->values.count = level->values.count + count;
	return (emp_values_slice_t) { .values = data, .count = count };
}

void emp_level_parse_int_grid_data(emp_level_t* level, yyjson_val* layer, emp_values_slice_t* values)
{
	yyjson_val* int_grid = yyjson_obj_get(layer, "intGridCsv");

	size_t idx, size;
	size = yyjson_arr_size(int_grid);
	*values = emp_level_reserve_values(level, size);

	yyjson_val* value;
	yyjson_arr_foreach(int_grid, idx, size, value)
	{
		values->values[idx] = (u8)yyjson_get_int(value);
	}
}

void emp_load_sublevel(emp_level_t* level, size_t index, yyjson_val* data)
{
	yyjson_val* layerInstances = yyjson_obj_get(data, "layerInstances");

	//emp_sublevel_list sublevels = { 0 };

	size_t idx, size;

	yyjson_val* layer;

	emp_values_slice_t values = {0};
	yyjson_arr_foreach(layerInstances, idx, size, layer)
	{
		yyjson_val* id = yyjson_obj_get(layer, "__identifier");
		printf("%s\n", yyjson_get_str(id));
		const char* str = yyjson_get_str(id);

		if (SDL_strcmp(str, "world") == 0) {
			emp_level_parse_int_grid_data(level, layer, &values);
		}
	}
}

void emp_load_level_asset(struct emp_asset_t* asset)
{
	emp_buffer* buffer = &asset->data;
	yyjson_doc* doc = yyjson_read((const char*)buffer->data, buffer->size, 0);

	yyjson_val* root = yyjson_doc_get_root(doc);
	yyjson_val* sublevels = yyjson_obj_get(root, "levels");

	emp_level_t* level = SDL_malloc(sizeof(*level));
	memset(level, 0, sizeof(*level));

	size_t idx, size;
	yyjson_val* current;
	yyjson_arr_foreach(sublevels, idx, size, current)
	{
		emp_load_sublevel(level, idx, current);
	}
}

void emp_unload_level_asset(struct emp_asset_t* asset)
{
}
