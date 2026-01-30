#include "entities.h"

emp_entities_t* G;

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

void emp_entities_update(float dt)
{
	for (u64 i = 0; i < EMP_MAX_PLAYERS; ++i)
	{
		emp_player_t* player = &G->player[i];
		emp_player_uptdate(dt, player);
	}

	for (u64 i = 0; i < EMP_MAX_ENEMIES; ++i)
	{
		emp_player_t* enemy = &G->enemies[i];
		emp_enemy_uptdate(dt, enemy);
	}

	for (u64 i = 0; i < EMP_MAX_BULLETS; ++i)
	{
		emp_bullet_t* bullet = &G->bullets[i];
		emp_bullet_uptdate(dt, bullet);
	}

	for (u64 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i)
	{
		emp_bullet_generator_t* generator = &G->generators[i];
		emp_generator_uptdate(dt, generator);

	}
}