
#include <Empire/assets.h>
#include <Empire/level.h>
#include <Empire/yyjson.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

emp_sublevel_t* emp_level_create_sublevel(emp_level_asset_t* level, emp_value_grid_t* values, emp_tiles_data_t* tiles)
{
	emp_sublevel_t* sublevel = level->sublevels.entries + level->sublevels.count;
	level->sublevels.count = level->sublevels.count + 1;
	sublevel->values = *values;
	sublevel->tiles = *tiles;
	return sublevel;
}

emp_value_grid_t emp_level_reserve_values(emp_level_asset_t* level, u32 count)
{
	u8* data = level->values.values + level->values.count;
	level->values.count = level->values.count + count;
	return (emp_value_grid_t) { .entries = data, .count = count };
}

emp_tiles_data_t emp_level_reserve_tiles(emp_level_asset_t* level, u32 count)
{
	emp_tile_desc_t* data = level->tiles.entries + level->tiles.count;
	level->tiles.count = level->tiles.count + count;
	return (emp_tiles_data_t) { .values = data, .count = count };
}

void emp_level_parse_int_grid_data(emp_level_asset_t* level, yyjson_val* layer, emp_value_grid_t* values)
{
	yyjson_val* int_grid = yyjson_obj_get(layer, "intGridCsv");

	u64 idx;
	u64 size = yyjson_arr_size(int_grid);
	*values = emp_level_reserve_values(level, (u32)size);

	yyjson_val* value;
	yyjson_arr_foreach(int_grid, idx, size, value)
	{
		values->entries[idx] = (u8)yyjson_get_int(value);
	}
}

void emp_level_parse_tile_data(emp_level_asset_t* level, yyjson_val* layer, emp_tiles_data_t* tiles)
{
	yyjson_val* layer_tiles = yyjson_obj_get(layer, "autoLayerTiles");

	u64 idx, size;
	size = yyjson_arr_size(layer_tiles);
	*tiles = emp_level_reserve_tiles(level, (u32)size);

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

int emp_level_add_entities_from_fields(emp_level_asset_t* level, yyjson_val* fields, emp_level_entity_t* out_entity)
{
	u64 idx, size;
	yyjson_val* field;
	int is_reasonably_constructed = 0;
	yyjson_arr_foreach(fields, idx, size, field)
	{
		yyjson_val* identifier = yyjson_obj_get(field, "__identifier");
		if (identifier) {
			const char* str = yyjson_get_str(identifier);
			if (SDL_strcmp(str, "weapon") == 0) {
				yyjson_val* value = yyjson_obj_get(field, "__value");
				if (value) {
					out_entity->weapon_index = (uint32_t)yyjson_get_uint(value);
				}
			}
		}

		yyjson_val* type = yyjson_obj_get(field, "__type");
		if (type) {
			const char* str = yyjson_get_str(type);
			if (SDL_strcmp(str, "LocalEnum.behaviour") == 0) {
				out_entity->type = emp_entity_type_spawner;
				yyjson_val* value = yyjson_obj_get(field, "__value");
				if (value) {
					if (SDL_strcmp(yyjson_get_str(value), "roamer") == 0) {
						out_entity->behaviour = emp_behaviour_type_roamer;
						is_reasonably_constructed = 1;
					}
					if (SDL_strcmp(yyjson_get_str(value), "chaser") == 0) {
						out_entity->behaviour = emp_behaviour_type_chaser;
						is_reasonably_constructed = 1;
					}
				}
			}

			if (SDL_strcmp(str, "LocalEnum.boss") == 0) {
				out_entity->type = emp_entity_type_boss;
				yyjson_val* value = yyjson_obj_get(field, "__value");
				if (value) {
					if (SDL_strcmp(yyjson_get_str(value), "octopus") == 0) {
						out_entity->behaviour = emp_behaviour_type_roamer;
						is_reasonably_constructed = 1;
					}
				}
			}
		}
	}
	SDL_Log("No Entity mapping found");
	return is_reasonably_constructed;
}

static void emp_level_entities_list_add(emp_level_entities_list_t* list, emp_level_entity_t* value)
{
	if (list->count < SDL_arraysize(list->entries)) {
		list->entries[list->count] = *value;
		list->count = list->count + 1;
	} else {
		SDL_Log("No space in entity list anymore. Bump the capacity");
	}
}

static void emp_level_add_entities(emp_level_asset_t* level, yyjson_val* instances)
{
	u64 idx, size;
	yyjson_val* instance;
	yyjson_arr_foreach(instances, idx, size, instance)
	{
		emp_level_entity_t entity = { 0 };

		entity.x = (float)yyjson_get_num(yyjson_obj_get(instance, "__worldX"));
		entity.y = (float)yyjson_get_num(yyjson_obj_get(instance, "__worldY"));
		entity.w = (float)yyjson_get_num(yyjson_obj_get(instance, "width"));
		entity.h = (float)yyjson_get_num(yyjson_obj_get(instance, "height"));

		yyjson_val* identifier = yyjson_obj_get(instance, "__identifier");

		if (identifier) {
			const char* str = yyjson_get_str(identifier);
			if (SDL_strcmp(str, "player") == 0) {
				entity.type = emp_entity_type_player;
				entity.behaviour = emp_behaviour_type_none;
				emp_level_entities_list_add(&level->entities, &entity);
				continue;
			}
		}

		yyjson_val* fields = yyjson_obj_get(instance, "fieldInstances");
		if (emp_level_add_entities_from_fields(level, fields, &entity)) {
			emp_level_entities_list_add(&level->entities, &entity);
		}
	}
}

void emp_load_sublevel(emp_level_asset_t* level, u32 index, yyjson_val* data)
{
	yyjson_val* layerInstances = yyjson_obj_get(data, "layerInstances");

	u64 idx, size;

	yyjson_val* layer;

	emp_tiles_data_t tiles = { 0 };
	emp_value_grid_t values = { 0 };

	yyjson_arr_foreach(layerInstances, idx, size, layer)
	{
		yyjson_val* id = yyjson_obj_get(layer, "__identifier");
		const char* str = yyjson_get_str(id);

		if (SDL_strcmp(str, "world") == 0) {
			emp_level_parse_int_grid_data(level, layer, &values);
			emp_level_parse_tile_data(level, layer, &tiles);
			tiles.tilemap = SDL_strdup(yyjson_get_str(yyjson_obj_get(layer, "__tilesetRelPath")));
			values.grid_size = (float)yyjson_get_num(yyjson_obj_get(layer, "__gridSize"));
			values.grid_width = (u32)yyjson_get_uint(yyjson_obj_get(layer, "__cWid"));
			values.grid_height = (u32)yyjson_get_uint(yyjson_obj_get(layer, "__cHei"));
		}

		if (SDL_strcmp(str, "entities") == 0) {
			level->entities.grid_size = (float)yyjson_get_num(yyjson_obj_get(layer, "__gridSize"));
			yyjson_val* instances = yyjson_obj_get(layer, "entityInstances");
			emp_level_add_entities(level, instances);
		}
	}

	emp_sublevel_t* sublevel = emp_level_create_sublevel(level, &values, &tiles);
	sublevel->offset.x = (float)yyjson_get_num(yyjson_obj_get(data, "worldX"));
	sublevel->offset.y = (float)yyjson_get_num(yyjson_obj_get(data, "worldY"));
}

u32 emp_level_query(emp_level_asset_t* level, emp_entity_type type, u32 offset)
{
	for (u32 index = offset; index < level->entities.count; index++) {
		emp_level_entity_t* entity = level->entities.entries + index;
		if (entity->type == type) {
			return index + 1;
		}
	}
	return 0;
}

emp_level_entity_t* emp_level_get(emp_level_asset_t* level, u32 at)
{
	return level->entities.entries + at;
}

void emp_load_level_asset(struct emp_asset_t* asset)
{
	emp_buffer* buffer = &asset->data;
	yyjson_doc* doc = yyjson_read((const char*)buffer->data, (size_t)buffer->size, 0);

	yyjson_val* root = yyjson_doc_get_root(doc);
	yyjson_val* sublevels = yyjson_obj_get(root, "levels");

	emp_level_asset_t* level = SDL_malloc(sizeof(*level));
	memset(level, 0, sizeof(*level));

	u64 idx, size;
	yyjson_val* current;
	yyjson_arr_foreach(sublevels, idx, size, current)
	{
		emp_load_sublevel(level, (u32)idx, current);
	}

	asset->handle = (emp_level_asset_t*)level;

	yyjson_doc_free(doc);
}

void emp_unload_level_asset(struct emp_asset_t* asset)
{
	emp_level_asset_t* level_asset = (emp_level_asset_t*)asset->handle;
	for (u32 li = 0; li < level_asset->sublevels.count; li++) {
		emp_sublevel_t* sublevel = level_asset->sublevels.entries + li;
		SDL_free(sublevel->tiles.tilemap);
	}

	SDL_free(asset->handle);
	asset->handle = NULL;
}
