#pragma once
#include "types.h"

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
    u64 target_type;
    void* (*load)(emp_buffer buffer);
    void (*unload)(void* handle);
} emp_asset_loader_t;

typedef struct emp_asset_manager_o
{
    emp_asset_t* assets;
    u64 asset_count;
    emp_asset_loader_t* loaders;
    u64 loader_count;
} emp_asset_manager_o;


void emp_asset_manager_check_hot_reload();