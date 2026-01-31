
#include <Empire/assets.h>
#include <Empire/level.h>
#include <Empire/yyjson.h>
#include <SDL3/SDL_stdinc.h>

emp_sublevel_t* emp_level_create_sublevel(emp_level_asset_t* level, emp_values_slice_t* values, emp_tile_desc_slice_t* tiles)
{
	emp_sublevel_t* sublevel = level->sublevels.entries + level->sublevels.count;
	level->sublevels.count = level->sublevels.count + 1;
	sublevel->values = *values;
	sublevel->tiles = *tiles;
	return sublevel;
}

emp_values_slice_t emp_level_reserve_values(emp_level_asset_t* level, size_t count)
{
	u8* data = level->values.values + level->values.count;
	level->values.count = level->values.count + count;
	return (emp_values_slice_t) { .entries = data, .count = count };
}

emp_tile_desc_slice_t emp_level_reserve_tiles(emp_level_asset_t* level, size_t count)
{
	emp_tile_desc_t* data = level->tiles.entries + level->tiles.count;
	level->tiles.count = level->tiles.count + count;
	return (emp_tile_desc_slice_t) { .values = data, .count = count };
}

void emp_level_parse_int_grid_data(emp_level_asset_t* level, yyjson_val* layer, emp_values_slice_t* values)
{
	yyjson_val* int_grid = yyjson_obj_get(layer, "intGridCsv");

	size_t idx, size;
	size = yyjson_arr_size(int_grid);
	*values = emp_level_reserve_values(level, size);

	yyjson_val* value;
	yyjson_arr_foreach(int_grid, idx, size, value)
	{
		values->entries[idx] = (u8)yyjson_get_int(value);
	}
}

void emp_level_parse_tile_data(emp_level_asset_t* level, yyjson_val* layer, emp_tile_desc_slice_t* tiles)
{
	yyjson_val* layer_tiles = yyjson_obj_get(layer, "autoLayerTiles");

	size_t idx, size;
	size = yyjson_arr_size(layer_tiles);
	*tiles = emp_level_reserve_tiles(level, size);

	yyjson_val* value;
	yyjson_arr_foreach(layer_tiles, idx, size, value)
	{
		emp_tile_desc_t* tile_desc = tiles->values + idx;
		yyjson_val* px = yyjson_obj_get(value, "px");
		yyjson_val* src = yyjson_obj_get(value, "src");

		tile_desc->dst.x = (float)yyjson_get_num(yyjson_arr_get(px, 0));
		tile_desc->dst.y = (float)yyjson_get_num(yyjson_arr_get(px, 1));
		tile_desc->src.x = (float)yyjson_get_num(yyjson_arr_get(src, 0));
		tile_desc->src.y = (float)yyjson_get_num(yyjson_arr_get(src, 1));
	}
}

void emp_load_sublevel(emp_level_asset_t* level, size_t index, yyjson_val* data)
{
	yyjson_val* layerInstances = yyjson_obj_get(data, "layerInstances");

	emp_vec2_t world_offset;
	world_offset.x = (float)yyjson_get_num(yyjson_obj_get(data, "worldX"));
	world_offset.y = (float)yyjson_get_num(yyjson_obj_get(data, "worldY"));

	size_t idx, size;

	yyjson_val* layer;

	emp_tile_desc_slice_t tiles = { 0 };
	emp_values_slice_t values = { 0 };
	yyjson_arr_foreach(layerInstances, idx, size, layer)
	{
		yyjson_val* id = yyjson_obj_get(layer, "__identifier");
		const char* str = yyjson_get_str(id);

		if (SDL_strcmp(str, "world") == 0) {
			emp_level_parse_int_grid_data(level, layer, &values);
			emp_level_parse_tile_data(level, layer, &tiles);
			emp_sublevel_t* sublevel = emp_level_create_sublevel(level, &values, &tiles);
			sublevel->grid_size = (float)yyjson_get_num(yyjson_obj_get(layer, "__gridSize"));
			sublevel->grid_width = (u32)yyjson_get_uint(yyjson_obj_get(layer, "__cWid"));
			sublevel->grid_height = (u32)yyjson_get_uint(yyjson_obj_get(layer, "__cHei"));
			sublevel->offset.x = world_offset.x;
			sublevel->offset.y = world_offset.y;
		}
	}
}

void emp_load_level_asset(struct emp_asset_t* asset)
{
	emp_buffer* buffer = &asset->data;
	yyjson_doc* doc = yyjson_read((const char*)buffer->data, buffer->size, 0);

	yyjson_val* root = yyjson_doc_get_root(doc);
	yyjson_val* sublevels = yyjson_obj_get(root, "levels");

	emp_level_asset_t* level = SDL_malloc(sizeof(*level));
	memset(level, 0, sizeof(*level));

	size_t idx, size;
	yyjson_val* current;
	yyjson_arr_foreach(sublevels, idx, size, current)
	{
		emp_load_sublevel(level, idx, current);
	}

	asset->handle = level;

	yyjson_doc_free(doc);
}

void emp_unload_level_asset(struct emp_asset_t* asset)
{
	SDL_free(asset->handle);
	asset->handle = NULL;
}
