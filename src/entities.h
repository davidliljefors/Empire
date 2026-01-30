#pragma once

#include <Empire/types.h>

typedef struct emp_asset_t emp_asset_t;

#define EMP_MAX_PLAYERS 1
typedef struct emp_player_t
{
	u32 generation;
	bool alive;
	emp_vec2_t pos;
	emp_asset_t* texture_asset;
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

typedef struct emp_entities_t
{
	emp_player_t* player;
	emp_enemy_t* enemies;
	emp_bullet_t* bullets;
	emp_bullet_generator_t* generators;
} emp_entities_t;

extern emp_entities_t* G;

u32 emp_create_player();

emp_enemy_h emp_create_enemy();
void emp_destroy_enemy(emp_enemy_h handle);

emp_bullet_h emp_create_bullet();
void emp_destroy_bullet(emp_bullet_h handle);

emp_bullet_generator_h emp_create_bullet_generator();
void emp_destroy_bullet_generator(emp_bullet_generator_h handle);

void emp_entities_init();
void emp_entities_update(emp_update_args_t* args);