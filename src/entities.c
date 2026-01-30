#include "entities.h"

#include <SDL3/SDL.h>

emp_entities_t* G;

u32 emp_create_player()
{
	return 0;
}

emp_enemy_h emp_create_enemy()
{
	for (u64 i = 0; i < EMP_MAX_PLAYERS; ++i)
	{
		emp_enemy_t* enemy = &G->enemies[i];
		if (!enemy->alive)
		{
			enemy->generation++;
			return (emp_enemy_h){.index = i, .generation = enemy->generation};
		}

	}
}

void emp_destroy_enemy(emp_enemy_h handle)
{

}

emp_enemy_h emp_create_bullet()
{
	for (u64 i = 0; i < EMP_MAX_BULLETS; ++i)
	{
		emp_bullet_t* bullet = &G->bullets[i];
		if (!bullet->alive)
		{
			bullet->generation++;
			bullet->alive = true;
			return (emp_enemy_h){.index = i, .generation = bullet->generation};
		}

	}
}

void emp_destroy_bullet(emp_bullet_h handle)
{

}

emp_bullet_generator_h emp_create_bullet_generator()
{
	for (u64 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i)
	{
		emp_bullet_generator_t* gen = &G->generators[i];
		if (!gen->alive)
		{
			gen->generation++;
			gen->alive = true;
			return (emp_bullet_generator_h){.index = i, .generation = gen->generation};
		}

	}
}

void emp_destroy_bullet_generator(emp_bullet_generator_h handle)
{

}

void emp_player_uptdate(float dt, emp_player_t* player)
{

}

void emp_enemy_uptdate(float dt, emp_enemy_t* enemy)
{

}

void emp_bullet_uptdate(float dt, emp_bullet_t* bullet)
{

}

void emp_generator_uptdate(float dt, emp_bullet_generator_t* generator)
{

}

void emp_entities_init()
{
	G = SDL_malloc(sizeof(emp_entities_t));
	G->player = SDL_malloc(sizeof(emp_player_t) * EMP_MAX_PLAYERS);
	G->enemies = SDL_malloc(sizeof(emp_enemy_t) * EMP_MAX_ENEMIES);
	G->bullets = SDL_malloc(sizeof(emp_bullet_t) * EMP_MAX_BULLETS);
	G->generators = SDL_malloc(sizeof(emp_bullet_generator_t) * EMP_MAX_BULLET_GENERATORS);

	SDL_memset(G->player, 0, sizeof(emp_player_t) * EMP_MAX_PLAYERS);
	SDL_memset(G->enemies, 0, sizeof(emp_enemy_t) * EMP_MAX_ENEMIES);
	SDL_memset(G->bullets, 0, sizeof(emp_bullet_t) * EMP_MAX_BULLETS);
	SDL_memset(G->generators, 0, sizeof(emp_bullet_generator_t) * EMP_MAX_BULLET_GENERATORS);

}

void emp_entities_update(float dt)
{
	for (u64 i = 0; i < EMP_MAX_PLAYERS; ++i)
	{
		emp_player_t* player = &G->player[i];
		if (player->alive)
		{
			emp_player_uptdate(dt, player);
		}

	}

	for (u64 i = 0; i < EMP_MAX_ENEMIES; ++i)
	{
		emp_enemy_t* enemy = &G->enemies[i];
		if (enemy->alive)
		{
			emp_enemy_uptdate(dt, enemy); 
	  	}

	}

	for (u64 i = 0; i < EMP_MAX_BULLETS; ++i)
	{
		emp_bullet_t* bullet = &G->bullets[i];
		if (bullet->alive)
		{
			emp_bullet_uptdate(dt, bullet);
		}
	}

	for (u64 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i)
	{
		emp_bullet_generator_t* generator = &G->generators[i];
		if (generator->alive)
		{
			emp_generator_uptdate(dt, generator);
		}
	}
}