#include "entities.h"

#include <Empire/assets.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/math.inl>
#include <SDL3/SDL.h>

emp_G* G;

typedef struct emp_player_conf_t
{
	float speed;
} emp_player_conf_t;

emp_player_conf_t get_player_conf()
{
	return (emp_player_conf_t) { .speed = 120.0f };
}

emp_weapon_conf_t* wep1;
emp_weapon_conf_t* wep2;
emp_weapon_conf_t* wep3;
emp_weapon_conf_t* wep4;

void emp_init_weapon_configs()
{
	wep1 = SDL_malloc(sizeof(emp_weapon_conf_t));
	wep1->delay_between_shots = 0.5f;
	wep1->num_shots = 1;
	wep1->shots[0] = (emp_bullet_conf_t) {
		.speed = 500.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet,
	};

	wep2 = SDL_malloc(sizeof(emp_weapon_conf_t));
	wep2->delay_between_shots = 0.5f;
	wep2->num_shots = 5;
	wep2->shots[0] = (emp_bullet_conf_t) {
		.speed = 500.0f,
		.start_angle = 15.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet
	};

	wep2->shots[1] = (emp_bullet_conf_t) {
		.speed = 500.0f,
		.start_angle = -15.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet
	};

	wep2->shots[2] = (emp_bullet_conf_t) {
		.speed = 600.0f,
		.start_angle = 25.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet
	};

	wep2->shots[3] = (emp_bullet_conf_t) {
		.speed = 600.0f,
		.start_angle = -25.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet
	};

	wep2->shots[4] = (emp_bullet_conf_t) {
		.speed = 800.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet
	};

	wep3 = SDL_malloc(sizeof(emp_weapon_conf_t));
	wep4 = SDL_malloc(sizeof(emp_weapon_conf_t));
}

emp_bullet_conf_t get_bullet1()
{
	emp_bullet_conf_t bullet = (emp_bullet_conf_t) { .speed = 1000.0f, .texture_asset = &G->args->assets->png->bullet };
	return bullet;
}

void spawn_bullets(emp_vec2_t pos, emp_vec2_t direction, emp_weapon_conf_t* conf)
{
	for (u32 i = 0; i < conf->num_shots; ++i) {
		emp_bullet_conf_t bullet_conf = conf->shots[i];
		emp_bullet_h bullet_handle = emp_create_bullet();
		emp_bullet_t* bullet = &G->bullets[bullet_handle.index];

		bullet->vel = emp_vec2_mul(emp_vec2_normalize(direction), bullet_conf.speed);
		if (bullet_conf.start_angle != 0.0f) {
			bullet->vel = emp_vec2_rotate(bullet->vel, bullet_conf.start_angle);
		}
		bullet->life_left = bullet_conf.lifetime;
		bullet->damage = bullet_conf.damage;
		bullet->pos = pos;
		bullet->texture_asset = bullet_conf.texture_asset;
	}
}

u32 emp_create_player()
{
	G->player[0].alive = true;
	G->player[0].generation = 1;
	return 0;
}

emp_enemy_h emp_create_enemy()
{
	for (u32 i = 0; i < EMP_MAX_ENEMIES; ++i) {
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

emp_bullet_h emp_create_bullet()
{
	for (u32 i = 0; i < EMP_MAX_BULLETS; ++i) {
		emp_bullet_t* bullet = &G->bullets[i];
		if (!bullet->alive) {
			bullet->generation++;
			// Initialize fields BEFORE setting alive to prevent update with stale data
			bullet->pos = (emp_vec2_t) { 0 };
			bullet->vel = (emp_vec2_t) { 0 };
			bullet->life_left = 0.0f;
			bullet->damage = 0.0f;
			bullet->texture_asset = NULL;
			bullet->alive = true;
			return (emp_bullet_h) { .index = i, .generation = bullet->generation };
		}
	}

	assert(false && "out of bullets");

	return (emp_bullet_h) { 0 };
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


	emp_vec2_t movement = {0};

	if (state[SDL_SCANCODE_W]) {
		movement.y = -conf.speed;
	}

	if (state[SDL_SCANCODE_A]) {
		movement.x = -conf.speed;
	}

	if (state[SDL_SCANCODE_S]) {
		movement.y = conf.speed;
	}

	if (state[SDL_SCANCODE_D]) {
		movement.x = conf.speed;
	}
	movement = emp_vec2_normalize(movement);
	movement = emp_vec2_mul(movement, args->dt * conf.speed);

	player->pos = emp_vec2_add(player->pos, movement);

	emp_texture_t* tex = player->texture_asset->handle;
	SDL_FRect dstRect = (SDL_FRect) { player->pos.x, player->pos.y, (float)tex->width, (float)tex->height };
	SDL_RenderTexture(args->r, tex->texture, NULL, &dstRect);

	emp_vec2_t mouse_pos;
	SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);

	if (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
		if (player->shot_delay <= 0.0f) {
			emp_vec2_t delta = emp_vec2_sub(mouse_pos, player->pos);
			spawn_bullets(player->pos, delta, wep2);
			player->shot_delay = wep2->delay_between_shots;
		}
	}

	player->shot_delay -= args->dt;
}

void emp_enemy_uptdate(emp_update_args_t* args, emp_enemy_t* enemy)
{
}

void emp_bullet_uptdate(emp_update_args_t* args, emp_bullet_t* bullet)
{
	bullet->life_left -= args->dt;

	bullet->pos.x += bullet->vel.x * args->dt;
	bullet->pos.y += bullet->vel.y * args->dt;

	if (bullet->life_left <= 0.0f) {
		bullet->alive = false;
	}

	emp_texture_t* tex = bullet->texture_asset->handle;
	SDL_FRect dstRect = (SDL_FRect) { bullet->pos.x, bullet->pos.y, (float)tex->width, (float)tex->height };
	SDL_RenderTexture(args->r, tex->texture, NULL, &dstRect);
}

void emp_generator_uptdate(emp_update_args_t* args, emp_bullet_generator_t* generator)
{
}

void emp_entities_init()
{
	G = SDL_malloc(sizeof(emp_G));
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
			emp_generator_uptdate (args, generator);
		}
	}
}