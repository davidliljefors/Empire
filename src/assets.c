#include <Empire/assets.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/hash.inl>
#include <Empire/stb_ds.h>
#include <Empire/util.h>
#include <SDL3/SDL.h>
#include <stdio.h>

typedef struct emp_generated_generic_t
{
	int count;
	u64 type_hash;
	emp_asset_t asset[1];
} emp_generated_generic_t;

emp_asset_manager_o* emp_asset_manager_create(emp_generated_assets_o* assets)
{
	emp_asset_manager_o* mgr = SDL_malloc(sizeof(emp_asset_manager_o));
	SDL_zerop(mgr);

	emp_generated_generic_t** generic_assets = (emp_generated_generic_t**)assets;

	for (u64 i = 0; i < sizeof(emp_generated_assets_o) / sizeof(void*); ++i) {
		emp_generated_generic_t* generic = generic_assets[i];
		emp_asset_type_t asset_type = { .count = generic->count, .assets = generic->asset, .loader = { 0 } };
		hmput(mgr->assets_by_ext, generic->type_hash, asset_type);
	}

	return mgr;
}

void emp_asset_manager_add_loader(emp_asset_manager_o* mgr, emp_asset_loader_t loader, u64 type)
{
	emp_asset_kvp* asset_type = stbds_hmgetp(mgr->assets_by_ext, type);
	if (asset_type) {
		asset_type->value.loader = loader;
	}
}

void emp_asset_manager_check_hot_reload(emp_asset_manager_o* mgr, float dt)
{
	mgr->accumulator = mgr->accumulator + dt;
	if (mgr->accumulator < 1.0f) {
		return;
	}
	mgr->accumulator = 0;

	u64 len = hmlen(mgr->assets_by_ext);
	for (u64 i = 0; i < len; ++i) {
		emp_asset_kvp* pair = &mgr->assets_by_ext[i];
		emp_asset_loader_t loader = pair->value.loader;
		if (loader.load && loader.unload) {
			for (u64 j = 0; j < pair->value.count; ++j) {
				emp_asset_t* asset = &pair->value.assets[j];
				SDL_PathInfo info;
				SDL_GetPathInfo(asset->path, &info);

				if (asset->handle == NULL) {
					loader.load(asset);
					asset->last_modified = info.modify_time;
				}
#ifndef __EMSCRIPTEN__
				if (asset->last_modified != info.modify_time) {
					emp_buffer new_data = emp_read_entire_file(asset->path);
					u64 new_hash = emp_hash_data(new_data);
					if (new_hash != 0 && new_hash != asset->hash) {
						loader.unload(asset);
						SDL_free(asset->data.data);
						asset->data = new_data;
						asset->hash = new_hash;
						loader.load(asset);
					} else {
						SDL_free(new_data.data);
					}
				}
#endif
			}
		}
	}
}