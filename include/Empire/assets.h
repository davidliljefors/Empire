#pragma once
#include "types.h"

struct emp_generated_assets_o;

typedef struct emp_asset_t
{
    const char* path;
    emp_buffer data;
    u64 size;
    u64 hash;
    void* handle;
} emp_asset_t;

typedef struct emp_asset_loader_t
{
    void (*load)(emp_asset_t* asset);
    void (*unload)(emp_asset_t* asset);
} emp_asset_loader_t;

typedef struct emp_asset_type_t
{ 
    u64 count;
    emp_asset_t* assets;
    emp_asset_loader_t loader;
} emp_asset_type_t;

typedef struct emp_asset_kvp
{
    u64 key; // type
    emp_asset_type_t value;
} emp_asset_kvp;

typedef struct emp_asset_manager_o
{
    emp_asset_kvp* assets_by_ext;
} emp_asset_manager_o;

emp_asset_manager_o* emp_asset_manager_create(struct emp_generated_assets_o* assets);

void emp_asset_manager_add_loader(emp_asset_manager_o* mgr, emp_asset_loader_t loader, u64 type);

void emp_asset_manager_check_hot_reload(emp_asset_manager_o* mgr);