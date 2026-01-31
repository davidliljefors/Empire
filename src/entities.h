#pragma once

#include <Empire/types.h>

typedef struct emp_asset_t emp_asset_t;

typedef struct emp_bullet_conf_t
{
	float speed;
	float start_angle;
	float lifetime;
	float damage;
	emp_asset_t* texture_asset;
} emp_bullet_conf_t;

typedef struct emp_weapon_conf_t
{
	float delay_between_shots;
	u32 num_shots;
	emp_bullet_conf_t shots[8];
} emp_weapon_conf_t;

#define EMP_MAX_PLAYERS 1
typedef struct emp_player_t
{
	u32 generation;
	bool alive;
	emp_vec2_t pos;
	emp_asset_t* texture_asset;
	float shot_delay;
} emp_player_t;

#define EMP_MAX_ENEMIES 256
typedef struct emp_enemy_t
{
	bool alive;
	u32 generation;
} emp_enemy_t;

typedef struct emp_enemy_h
{
	u32 index;
	u32 generation;
} emp_enemy_h;

#define EMP_MAX_BULLETS 65535
typedef struct emp_bullet_t
{
	bool alive;
	u32 generation;
	emp_vec2_t pos;
	emp_vec2_t vel;
	float life_left;
	float damage;
	emp_asset_t* texture_asset;
} emp_bullet_t;

typedef struct emp_bullet_h
{
	u32 index;
	u32 generation;
} emp_bullet_h;

#define EMP_MAX_BULLET_GENERATORS 1024
typedef struct emp_bullet_generator_t
{
	bool alive;
	u32 generation;
} emp_bullet_generator_t;

typedef struct emp_bullet_generator_h
{
	u32 index;
	u32 generation;
} emp_bullet_generator_h;

#define EMP_LEVEL_WIDTH 1024;
#define EMP_LEVEL_HEIGHT 1024;

typedef struct emp_tile_t
{

} emp_tile_t;

typedef struct emp_level_t
{

	emp_tile_t* tiles;
} emp_level_t;

typedef struct emp_entities_t
{
	emp_update_args_t* args;
	emp_generated_assets_o* assets;
	emp_player_t* player;
	emp_enemy_t* enemies;
	emp_bullet_t* bullets;
	emp_bullet_generator_t* generators;
} emp_G;

extern emp_G* G;

void emp_init_weapon_configs();
u32 emp_create_player();

emp_enemy_h emp_create_enemy();
void emp_destroy_enemy(emp_enemy_h handle);

emp_bullet_h emp_create_bullet();
void emp_destroy_bullet(emp_bullet_h handle);

emp_bullet_generator_h emp_create_bullet_generator();
void emp_destroy_bullet_generator(emp_bullet_generator_h handle);

void emp_entities_init();
void emp_entities_update(emp_update_args_t* args);