#include "entities.h"

#include <SDL3/SDL.h>

emp_entities_t* G;

typedef struct emp_player_conf_t
{
	float speed;
} emp_player_conf_t;

emp_player_conf_t get_player_conf()
{
	return (emp_player_conf_t) { .speed = 5.0f };
}

u32 emp_create_player()
{
	G->player[0].alive = true;
	G->player[0].generation = 1;
	return 0;
}

emp_enemy_h emp_create_enemy()
{
	for (u32 i = 0; i < EMP_MAX_PLAYERS; ++i) {
		emp_enemy_t* enemy = &G->enemies[i];
		if (!enemy->alive) {
			enemy->generation++;
			return (emp_enemy_h) { .index = i, .generation = enemy->generation };
		}
	}

	assert(false && "out of enemies");

	return (emp_enemy_h) { 0 };
}

void emp_destroy_enemy(emp_enemy_h handle)
{
}

emp_enemy_h emp_create_bullet()
{
	for (u32 i = 0; i < EMP_MAX_BULLETS; ++i) {
		emp_bullet_t* bullet = &G->bullets[i];
		if (!bullet->alive) {
			bullet->generation++;
			bullet->alive = true;
			return (emp_enemy_h) { .index = i, .generation = bullet->generation };
		}
	}

	assert(false && "out of bullets");

	return (emp_enemy_h) { 0 };
}

void emp_destroy_bullet(emp_bullet_h handle)
{
}

emp_bullet_generator_h emp_create_bullet_generator()
{
	for (u32 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i) {
		emp_bullet_generator_t* gen = &G->generators[i];
		if (!gen->alive) {
			gen->generation++;
			gen->alive = true;
			return (emp_bullet_generator_h) { .index = i, .generation = gen->generation };
		}
	}

	assert(false && "out of generators");

	return (emp_bullet_generator_h) { 0 };
}

void emp_destroy_bullet_generator(emp_bullet_generator_h handle)
{
}

void emp_player_uptdate(emp_update_args_t* args, emp_player_t* player)
{
	const bool* state = SDL_GetKeyboardState(NULL);
	emp_player_conf_t conf = get_player_conf();

	// SDL_Log("%s, x:%f y:%f", "update enemy", player->x, player->y);

	if (state[SDL_SCANCODE_W]) {
		player->y -= conf.speed;
	}

	if (state[SDL_SCANCODE_A]) {
		player->x -= conf.speed;
	}

	if (state[SDL_SCANCODE_S]) {
		player->y += conf.speed;
	}

	if (state[SDL_SCANCODE_D]) {
		player->x += conf.speed;
	}

	emp_texture_t* tex = player->texture;

	SDL_FRect dstRect = (SDL_FRect) { player->x, player->y, (float)tex->width, (float)tex->height };

	SDL_RenderTexture(args->r, tex->texture, NULL, &dstRect);
}

void emp_enemy_uptdate(emp_update_args_t* args, emp_enemy_t* enemy)
{
}

void emp_bullet_uptdate(emp_update_args_t* args, emp_bullet_t* bullet)
{
}

void emp_generator_uptdate(emp_update_args_t* args, emp_bullet_generator_t* generator)
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

void emp_entities_update(emp_update_args_t* args)
{
	for (u64 i = 0; i < EMP_MAX_PLAYERS; ++i) {
		emp_player_t* player = &G->player[i];
		if (player->alive) {
			emp_player_uptdate(args, player);
		}
	}

	for (u64 i = 0; i < EMP_MAX_ENEMIES; ++i) {
		emp_enemy_t* enemy = &G->enemies[i];
		if (enemy->alive) {
			emp_enemy_uptdate(args, enemy);
		}
	}

	for (u64 i = 0; i < EMP_MAX_BULLETS; ++i) {
		emp_bullet_t* bullet = &G->bullets[i];
		if (bullet->alive) {
			emp_bullet_uptdate(args, bullet);
		}
	}

	for (u64 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i) {
		emp_bullet_generator_t* generator = &G->generators[i];
		if (generator->alive) {
			emp_generator_uptdate(args, generator);
		}
	}
}