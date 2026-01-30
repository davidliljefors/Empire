#pragma once

#include "types.h"

#define EMP_ASPECT(...)

typedef struct emp_health_aspect
{
	i32 (*get_health)(void*);
	void (*set_health)(void*, i32);
} emp_health_aspect;

typedef struct emp_aspect_def
{
	const char* aspect_name;
	void* impl;
} emp_aspect_def;

typedef struct emp_player
{
	int health;
} emp_player;

i32 emp_player_get_health(emp_player* player)
{
	return player->health;
}

void emp_player_set_health(emp_player* player, i32 health)
{
	player->health = health;
}

void emp_register_aspect(const char* aspect_name, u32 size);

void emp_register_prototype(const char* prototype_name, u32 size);

void emp_add_aspect_to_prototype(const char* prototype_name, const char* aspect_name, void* impl);

void test()
{
	emp_register_aspect("emp_health_aspect", sizeof(emp_health_aspect));

	emp_register_prototype("emp_player", sizeof(emp_player));

	emp_health_aspect player_health_aspect;
	player_health_aspect.get_health = (typeof(player_health_aspect.get_health))emp_player_get_health;
	player_health_aspect.set_health = (typeof(player_health_aspect.set_health))emp_player_set_health;

	emp_add_aspect_to_prototype("emp_player", "emp_health_aspect", &player_health_aspect);
}
