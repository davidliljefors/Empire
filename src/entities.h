#pragma once

#include <Empire/types.h>

#define EMP_MAX_PLAYERS 1
typedef struct emp_player_t
{
	float dummy;
} emp_player_t;

#define EMP_MAX_ENEMIES 256
typedef struct emp_enemy_t
{
	float dummy;
} emp_enemy_t;

#define EMP_MAX_BULLETS 65535
typedef struct emp_bullet_t
{
	float dummy;
} emp_bullet_t;

#define EMP_MAX_BULLET_GENERATORS 1024
typedef struct emp_bullet_generator_t
{
	float dummy;
} emp_bullet_generator_t;

typedef struct emp_entities_t
{
	emp_player_t* player;
	emp_enemy_t* enemies;
	emp_bullet_t* bullets;
	emp_bullet_generator_t* generators;
} emp_entities_t;

extern emp_entities_t* G;

void emp_entities_update(float dt);