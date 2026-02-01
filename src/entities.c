#include "entities.h"

#include "SDL3/SDL_scancode.h"
#include "SDL3_mixer/SDL_mixer.h"

#include <Empire/assets.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/level.h>
#include <Empire/math.inl>
#include <SDL3/SDL.h>

#define ANIMATION_SPEED 0.15f

emp_G* G;
emp_enemy_conf_t* enemy_confs[16];
emp_weapon_conf_t* weapons[10];

typedef struct emp_roamer_data_t
{
	float time_until_change;
} emp_roamer_data_t;

u32 rng_state;

typedef struct emp_player_conf_t
{
	float speed;
} emp_player_conf_t;

emp_player_conf_t get_player_conf()
{
	return (emp_player_conf_t) { .speed = 620.0f };
}

void PlayOneShot(emp_asset_t* asset)
{
	MIX_PlayAudio(G->mixer, (MIX_Audio*)asset->handle);
}

emp_vec2_t render_offset()
{
	int render_w, render_h;
	SDL_Window* window = SDL_GetRenderWindow(G->renderer);
	SDL_GetWindowSize(window, &render_w, &render_h);
	return (emp_vec2_t) { .x = (float)render_w / 2.0f, .y = (float)render_h / 1.5f };
}

void draw_rect_at(emp_vec2_t pos, float size, u8 r, u8 g, u8 b, u8 a)
{
	SDL_FRect rect;
	rect.x = pos.x - (size / 2);
	rect.y = pos.y - (size / 2);
	rect.w = size;
	rect.h = size;

	rect.x -= G->player->pos.x;
	rect.y -= G->player->pos.y;

	emp_vec2_t offset = render_offset();
	rect.x += offset.x;
	rect.y += offset.y;

	SDL_SetRenderDrawColor(G->renderer, r, g, b, a);
	//SDL_RenderRect(G->renderer, &rect);
}

SDL_FRect player_rect(emp_vec2_t pos, emp_texture_t* texture)
{
	SDL_FRect rect;

	rect.x = -(texture->width / 2);
	rect.y = -(texture->height / 2);
	rect.w = (float)texture->width;
	rect.h = (float)texture->height;

	emp_vec2_t offset = render_offset();
	rect.x += offset.x;
	rect.y += offset.y;

	return rect;
}

SDL_FRect render_rect(emp_vec2_t pos, emp_texture_t* texture)
{
	SDL_FRect rect;
	rect.x = pos.x - (texture->width / 2);
	rect.y = pos.y - (texture->height / 2);
	rect.w = (float)texture->width;
	rect.h = (float)texture->height;

	rect.x -= G->player->pos.x;
	rect.y -= G->player->pos.y;

	emp_vec2_t offset = render_offset();
	rect.x += offset.x;
	rect.y += offset.y;

	return rect;
}

SDL_FRect render_rect_with_size(emp_vec2_t pos, float grid_size)
{
	SDL_FRect rect;
	rect.x = pos.x - (grid_size / 2);
	rect.y = pos.y - (grid_size / 2);
	rect.w = (float)grid_size;
	rect.h = (float)grid_size;

	rect.x -= G->player->pos.x;
	rect.y -= G->player->pos.y;

	emp_vec2_t offset = render_offset();
	rect.x += offset.x;
	rect.y += offset.y;

	return rect;
}

emp_vec2i_t get_tile(emp_vec2_t pos)
{
	int tile_x = (int)(roundf(pos.x / (EMP_TILE_SIZE * 4.0f)));
	int tile_y = (int)(roundf(pos.y / (EMP_TILE_SIZE * 4.0f)));

	return (emp_vec2i_t) { .x = tile_x, .y = tile_y };
}

bool tile_in_bounds(emp_vec2i_t tile)
{
	if (tile.x >= 0 && tile.x < (int)EMP_LEVEL_WIDTH && tile.y >= 0 && tile.y < (int)EMP_LEVEL_HEIGHT) {
		return true;
	}
	return false;
}

bool check_overlap_map(emp_vec2_t pos)
{
	emp_vec2i_t tile = get_tile(pos);

	if (tile_in_bounds(tile)) {
		u64 index = tile.y * EMP_LEVEL_WIDTH + tile.x;
		emp_tile_t* tile_data = G->level->tiles + index;
		return tile_data->state != emp_tile_state_none;
	}

	return true;
}

bool check_overlap_bullet_enemy(emp_bullet_t* bullet, emp_enemy_t* enemy)
{
	emp_texture_t* texture = enemy->texture_asset->handle;
	float size = texture->width / 2.0f;
	return emp_vec2_dist_sq(bullet->pos, enemy->pos) < size * size;
}

u64 index_from_tile(emp_vec2i_t tile)
{
	return tile.y * EMP_LEVEL_WIDTH + tile.x;
}

void add_enemy_to_tile(emp_enemy_t* enemy)
{
	emp_vec2i_t tile = get_tile(enemy->pos);

	if (tile_in_bounds(tile)) {
		u64 index = index_from_tile(tile);
		emp_enemy_t** first_enemy = &G->level->enemy_in_tile[index];
		enemy->next_in_tile = *first_enemy;
		*first_enemy = enemy;
		G->level->enemy_in_tile[index] = enemy;
	} else {
		enemy->next_in_tile = NULL;
	}
}

SDL_FRect source_rect(emp_asset_t* texture_asset)
{
	emp_texture_t* texture = texture_asset->handle;

	u32 total_frames = texture->columns * texture->rows;
	u32 slot = (u32)(G->args->global_time / ANIMATION_SPEED) % total_frames;

	u32 column = slot % texture->columns;
	u32 row = slot / texture->columns;

	SDL_FRect rect;

	rect.x = (float)(column * texture->source_size);
	rect.y = (float)(row * texture->source_size);
	rect.w = (float)texture->source_size;
	rect.h = (float)texture->source_size;

	return rect;
}

u32 simple_rng(u32* state)
{
	*state = *state * 1103515245u + 12345u;
	return (*state >> 16) & 0x7FFF;
}

void enemy_roamer_update(emp_enemy_t* enemy)
{
	emp_roamer_data_t* data = (emp_roamer_data_t*)&enemy->dynamic_data[0];
	float dt = G->args->dt;

	data->time_until_change -= dt;

	if (data->time_until_change <= 0.0f) {
		float angle = (float)(simple_rng(&rng_state) % 360);
		enemy->direction = emp_vec2_rotate((emp_vec2_t) { 1.0f, 0.0f }, angle);

		data->time_until_change = 0.3f + (float)(simple_rng(&rng_state) % 100) * 0.005f;
	}

	float move_speed = 120.0f;
	emp_vec2_t movement = emp_vec2_mul(enemy->direction, move_speed * dt);

	emp_vec2_t new_pos = emp_vec2_add(enemy->pos, movement);
	if (!check_overlap_map(new_pos)) {
		enemy->pos = new_pos;
	} else {
		float angle = (float)(simple_rng(&rng_state) % 360);
		enemy->direction = emp_vec2_rotate((emp_vec2_t) { 1.0f, 0.0f }, angle);
		data->time_until_change = 0.3f + (float)(simple_rng(&rng_state) % 100) * 0.005f;
	}
}

void emp_init_enemy_configs()
{
	enemy_confs[0] = SDL_malloc(sizeof(emp_enemy_conf_t));
	emp_enemy_conf_t* e0 = enemy_confs[0];
	e0->health = 10;
	e0->speed = 300.0f;
	e0->texture_asset = &G->assets->png->enemy1_32;
	e0->data_size = sizeof(emp_roamer_data_t);
	e0->update = enemy_roamer_update;

	enemy_confs[1] = SDL_malloc(sizeof(emp_enemy_conf_t));
	emp_enemy_conf_t* e1 = enemy_confs[1];
	e1->health = 10;
	e1->speed = 300.0f;
	e1->texture_asset = &G->assets->png->boss1_64;
	e1->data_size = sizeof(emp_roamer_data_t);
	e1->update = enemy_roamer_update;
}

void emp_init_weapon_configs()
{
	weapons[0] = SDL_malloc(sizeof(emp_weapon_conf_t));
	weapons[0]->delay_between_shots = 0.2f;
	weapons[0]->num_shots = 1;
	weapons[0]->shots[0] = (emp_bullet_conf_t) {
		.speed = 900.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.texture_asset = &G->assets->png->bullet_8,
		.damage = 1.0,
	};
	weapons[0]->asset = &G->assets->wav->shot1;

	weapons[1] = SDL_malloc(sizeof(emp_weapon_conf_t));
	weapons[1]->delay_between_shots = 0.4f;
	weapons[1]->num_shots = 1;
	weapons[1]->shots[0] = (emp_bullet_conf_t) {
		.speed = 300.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8,
	};
	weapons[1]->asset = &G->assets->wav->shot1;

	weapons[2] = SDL_malloc(sizeof(emp_weapon_conf_t)); // 3 shot
	weapons[2]->delay_between_shots = 0.3f;
	weapons[2]->num_shots = 3;
	weapons[2]->shots[0] = (emp_bullet_conf_t) {
		.speed = 450.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[2]->shots[1] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -15.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[2]->shots[2] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 15.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[2]->asset = &G->assets->wav->shot2;

	weapons[3] = SDL_malloc(sizeof(emp_weapon_conf_t)); // 5 shot
	weapons[3]->delay_between_shots = 0.3f;
	weapons[3]->num_shots = 5;
	weapons[3]->shots[0] = (emp_bullet_conf_t) {
		.speed = 450.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[3]->shots[1] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -15.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet2_8
	};
	weapons[3]->shots[2] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 15.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet2_8
	};
	weapons[3]->shots[3] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -30.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[3]->shots[4] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 30.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[3]->asset = &G->assets->wav->shot3;

	weapons[4] = SDL_malloc(sizeof(emp_weapon_conf_t)); // full circle
	weapons[4]->delay_between_shots = 0.3f;
	weapons[4]->num_shots = 12;
	weapons[4]->asset = &G->assets->wav->shot1;
	weapons[4]->shots[0] = (emp_bullet_conf_t) {
		.speed = 450.0f,
		.start_angle = 0.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[1] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -30.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[2] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 30.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[3] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -60.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[4] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 60.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[5] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -90.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[6] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 90.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[7] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -120.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[8] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 120.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[9] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -150.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[10] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 150.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[11] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = 180.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};
	weapons[4]->shots[12] = (emp_bullet_conf_t) {
		.speed = 400.0f,
		.start_angle = -180.0f,
		.lifetime = 3.0f,
		.damage = 1.0,
		.texture_asset = &G->assets->png->bullet_8
	};

	int total_bullets = 24;
	weapons[5] = SDL_malloc(sizeof(emp_weapon_conf_t)); // simple double circle
	weapons[5]->delay_between_shots = 0.3f;
	weapons[5]->num_shots = total_bullets;
	weapons[5]->asset = &G->assets->wav->shot2;
	for (int i = 0; i < total_bullets; i++) {
		float angle = i * 15.0f;
		float current_speed = (i % 2) ? 300.0f : 400.0f;

		weapons[5]->shots[i] = (emp_bullet_conf_t) {
			.speed = current_speed,
			.start_angle = angle,
			.lifetime = 3.0f,
			.damage = 1.0,
			.texture_asset = &G->assets->png->bullet_8
		};
	}

	weapons[6] = SDL_malloc(sizeof(emp_weapon_conf_t)); // pretty double circle
	weapons[6]->delay_between_shots = 0.5f;
	weapons[6]->num_shots = total_bullets;
	weapons[6]->asset = &G->assets->wav->shot3;
	for (int i = 0; i < total_bullets; i++) {
		float angle = i * 15.0f;
		float current_speed = (i % 2) ? 300.0f : 400.0f;

		weapons[6]->shots[i] = (emp_bullet_conf_t) {
			.speed = current_speed,
			.start_angle = angle,
			.lifetime = 3.0f,
			.damage = 1.0,
			.texture_asset = &G->assets->png->bullet2_8
		};
	}

	weapons[7] = SDL_malloc(sizeof(emp_weapon_conf_t)); // pretty half circle
	weapons[7]->delay_between_shots = 0.5f;
	weapons[7]->num_shots = total_bullets / 2 + 1;

	for (int i = 0; i < total_bullets; i++) {
		float angle = 90 - i * 15.0f;
		float current_speed = (i % 2) ? 300.0f : 400.0f;

		weapons[7]->shots[i] = (emp_bullet_conf_t) {
			.speed = current_speed,
			.start_angle = angle,
			.lifetime = 3.0f,
			.damage = 1.0,
			.texture_asset = &G->assets->png->bullet3_8
		};
	}

	weapons[7] = SDL_malloc(sizeof(emp_weapon_conf_t)); // pretty half circle
	weapons[7]->delay_between_shots = 0.5f;
	weapons[7]->num_shots = total_bullets / 2 + 1;
	weapons[7]->asset = &G->assets->wav->shot3;
	for (int i = 0; i < total_bullets; i++) {
		float angle = 90 - i * 15.0f;
		float current_speed = (i % 2) ? 300.0f : 400.0f;

		weapons[7]->shots[i] = (emp_bullet_conf_t) {
			.speed = current_speed,
			.start_angle = angle,
			.lifetime = 3.0f,
			.damage = 1.0,
			.texture_asset = &G->assets->png->bullet2_8
		};
	}
}

emp_bullet_conf_t get_bullet1()
{
	emp_bullet_conf_t bullet = (emp_bullet_conf_t) { .speed = 1000.0f, .texture_asset = &G->assets->png->bullet_8 };
	return bullet;
}

void spawn_bullets(emp_vec2_t pos, emp_vec2_t direction, bullet_mask mask, emp_weapon_conf_t* conf)
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
		bullet->mask = mask;
	}
	PlayOneShot(conf->asset);
}

u32 emp_create_player()
{
	G->player[0].alive = true;
	G->player[0].generation = 1;
	G->player[0].weapon_index = 0;
	G->player[0].health = 10;
	G->player[0].max_health = 30;
	return 0;
}

emp_enemy_h emp_create_enemy(emp_vec2_t pos, u32 enemy_conf_index, u32 weapon_index, emp_spawner_h spawned_by)
{
	for (u32 i = 0; i < EMP_MAX_ENEMIES; ++i) {
		emp_enemy_t* enemy = &G->enemies[i];
		if (!enemy->alive) {
			emp_enemy_conf_t* conf = enemy_confs[enemy_conf_index];
			SDL_memset(&enemy->dynamic_data, 0, 64);
			enemy->pos = pos;

			emp_vec2_t player_pos = G->player->pos;
			emp_vec2_t dir = emp_vec2_normalize(emp_vec2_sub(enemy->pos, player_pos));

			enemy->direction = dir;
			enemy->generation++;
			enemy->health = conf->health;
			enemy->update = conf->update;
			enemy->speed = conf->speed;
			enemy->texture_asset = conf->texture_asset;
			enemy->weapon = weapons[weapon_index];
			enemy->alive = true;
			enemy->spawned_by = spawned_by;
			enemy->enemy_shot_delay = 3.0;
			return (emp_enemy_h) { .index = i, .generation = enemy->generation };
		}
	}

	assert(false && "out of enemies");

	return (emp_enemy_h) { 0 };
}

void emp_create_spawner(emp_vec2_t pos, float health, u32 enemy_conf_index, u32 weapon_index, float frequency, u32 limit)
{
	for (u32 i = 0; i < EMP_MAX_SPAWNERS; ++i) {
		emp_spawner_t* spawner = &G->spawners[i];
		if (!spawner->alive) {
			SDL_memset(spawner, 0, sizeof(*spawner));
			spawner->enemy_conf_index = enemy_conf_index;
			spawner->alive = true;
			spawner->frequency = frequency;
			spawner->accumulator = 0;
			spawner->health = health;
			spawner->weapon_index = weapon_index;
			spawner->x = pos.x;
			spawner->y = pos.y;
			spawner->limit = limit;
			return;
		}
	}
	assert(false && "out of enemies");
}

emp_bullet_h emp_create_bullet()
{
	for (u32 i = 0; i < EMP_MAX_BULLETS; ++i) {
		emp_bullet_t* bullet = &G->bullets[i];
		if (!bullet->alive) {
			bullet->generation++;
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

void emp_player_update(emp_player_t* player)
{
	const bool* state = SDL_GetKeyboardState(NULL);
	emp_player_conf_t conf = get_player_conf();

	emp_vec2_t movement = { 0 };

	if (state[SDL_SCANCODE_W]) {
		movement.y = -conf.speed;
	}

	if (state[SDL_SCANCODE_A]) {
		movement.x = -conf.speed;
		player->flip = true;
	}

	if (state[SDL_SCANCODE_S]) {
		movement.y = conf.speed;
	}

	if (state[SDL_SCANCODE_D]) {
		movement.x = conf.speed;
		player->flip = false;
	}

	movement = emp_vec2_normalize(movement);
	movement = emp_vec2_mul(movement, G->args->dt * conf.speed);

	SDL_FRect src = source_rect(player->texture_asset);
	emp_texture_t* tex = player->texture_asset->handle;
	SDL_FRect dst = player_rect(player->pos, tex);
	dst.x = player->flip ? dst.x + dst.w : dst.x;
	dst.w = player->flip ? -dst.w : dst.w;
	SDL_RenderTexture(G->renderer, tex->texture, &src, &dst);

	emp_vec2_t mouse_pos;
	SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);

	emp_vec2_t pos_dx = emp_vec2_addx(player->pos, movement);
	if (check_overlap_map(pos_dx)) {
		movement.x = 0;
	}

	emp_vec2_t pos_dy = emp_vec2_addy(player->pos, movement);
	if (check_overlap_map(pos_dy)) {
		movement.y = 0;
	}

	player->pos = emp_vec2_add(player->pos, movement);

	if (state[SDL_SCANCODE_1]) {
		player->weapon_index = 1;
	} else if (state[SDL_SCANCODE_2]) {
		player->weapon_index = 2;
	} else if (state[SDL_SCANCODE_3]) {
		player->weapon_index = 3;
	} else if (state[SDL_SCANCODE_4]) {
		player->weapon_index = 4;
	} else if (state[SDL_SCANCODE_5]) {
		player->weapon_index = 5;
	} else if (state[SDL_SCANCODE_6]) {
		player->weapon_index = 6;
	} else if (state[SDL_SCANCODE_7]) {
		player->weapon_index = 7;
	} else if (state[SDL_SCANCODE_8]) {
		player->weapon_index = 8;
	} else if (state[SDL_SCANCODE_9]) {
		player->weapon_index = 9;
	} else if (state[SDL_SCANCODE_K]) {
		emp_create_level(&G->assets->ldtk->world, 1);
	}

	if (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT) || state[SDL_SCANCODE_SPACE]) {
		if (player->last_shot + weapons[player->weapon_index]->delay_between_shots < G->args->global_time) {
			emp_vec2_t player_screen_pos = (emp_vec2_t) { .x = dst.x + dst.w / 2, .y = dst.y + dst.h / 2 };
			emp_vec2_t delta = emp_vec2_sub(mouse_pos, player_screen_pos);
			spawn_bullets(player->pos, delta, emp_enemy_bullet_mask | emp_heavy_bullet_mask, weapons[player->weapon_index]);
			player->last_shot = G->args->global_time;
		}
	}
}

void emp_enemy_update(emp_enemy_t* enemy)
{
	enemy->update(enemy);

	if (enemy->health <= 0) {
		enemy->alive = false;
		if (enemy->spawned_by.index < EMP_MAX_SPAWNERS) {
			emp_spawner_t* spawner = G->spawners + enemy->spawned_by.index;
			SDL_assert(spawner->count != 0);
			spawner->count = spawner->count - 1;
		}
	}

	add_enemy_to_tile(enemy);

	emp_vec2_t player_pos = G->player->pos;
	emp_vec2_t dir = emp_vec2_normalize(emp_vec2_sub(player_pos, enemy->pos));

	if (enemy->last_shot + enemy->weapon->delay_between_shots * enemy->enemy_shot_delay <= G->args->global_time) {
		spawn_bullets(enemy->pos, dir, emp_player_bullet_mask, enemy->weapon);
		enemy->last_shot = G->args->global_time;
	}

	emp_texture_t* texture = enemy->texture_asset->handle;

	SDL_FRect src = source_rect(enemy->texture_asset);
	SDL_FRect dst = render_rect(enemy->pos, texture);

	double t = 0.3;
	double has_taken_damage = enemy->last_damage_time + t - G->args->global_time;
	if (has_taken_damage > 0.0) {
		u8 mod_value = 255 - (u8)(600.0 * has_taken_damage);
		SDL_SetTextureColorMod(texture->texture, 255, mod_value, mod_value);
		SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
		SDL_SetTextureColorMod(texture->texture, 255, 255, 255);
	} else {
		SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
	}

	//draw_rect_at(enemy->pos, 64, 255, 0, 0, 255);
}

void emp_bullet_update(emp_bullet_t* bullet)
{
	bullet->life_left -= G->args->dt;

	bullet->pos.x += bullet->vel.x * G->args->dt;
	bullet->pos.y += bullet->vel.y * G->args->dt;

	if (bullet->life_left <= 0.0f) {
		bullet->alive = false;
	}

	emp_vec2i_t tile = get_tile(bullet->pos);

	if (tile.x >= 0 && tile.x < (int)EMP_LEVEL_WIDTH && tile.y >= 0 && tile.y < (int)EMP_LEVEL_HEIGHT) {
		u32 index = ((u32)tile.y * EMP_LEVEL_WIDTH) + (u32)tile.x;
		emp_tile_t* tile_data = &G->level->tiles[index];

		if (tile_data->state != emp_tile_state_none) {
			bullet->alive = false;
			if (tile_data->state == emp_tile_state_breakable) {
				if (bullet->mask & emp_heavy_bullet_mask && G->level->health[index].value > 0) {
					G->level->health[index].value--;
				}
			}
		}
	}

	emp_texture_t* tex = bullet->texture_asset->handle;
	SDL_FRect dstRect = render_rect(bullet->pos, bullet->texture_asset->handle);
	SDL_RenderTexture(G->renderer, tex->texture, NULL, &dstRect);
	draw_rect_at(bullet->pos, 32, 255, 0, 0, 255);

	if (bullet->mask & emp_enemy_bullet_mask) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				emp_vec2i_t bullet_tile = get_tile(bullet->pos);
				bullet_tile.x += x;
				bullet_tile.y += y;
				emp_enemy_t* enemy_in_tile = G->level->enemy_in_tile[index_from_tile(bullet_tile)];
				while (enemy_in_tile != NULL) {
					if (check_overlap_bullet_enemy(bullet, enemy_in_tile)) {
						bullet->alive = false;
						enemy_in_tile->health -= bullet->damage;
						enemy_in_tile->last_damage_time = G->args->global_time;
						goto collision_done;
					}
					enemy_in_tile = enemy_in_tile->next_in_tile;
				}
			}
		}
	}
collision_done:;
}

static emp_texture_t* emp_texture_find(const char* path)
{
	if (SDL_strstr(G->assets->png->tilemap.path, path)) {
		return (emp_texture_t*)G->assets->png->tilemap.handle;
	}
	return NULL;
}

void emp_level_update(void)
{
	emp_level_asset_t* level_asset = (emp_level_asset_t*)G->assets->ldtk->world.handle;

	memset(G->level->tiles, 0, sizeof(*G->level->tiles) * EMP_LEVEL_TILES);

	for (u64 i = 0; i < EMP_LEVEL_TILES; ++i) {
		if (G->level->enemy_in_tile[i] != NULL) {
			emp_vec2_t pos;
			pos.x = 64.0f * (i % EMP_LEVEL_WIDTH);
			pos.y = 64.0f * (i / EMP_LEVEL_WIDTH);

			draw_rect_at(pos, 64, 0, 255, 0, 255);
		}
	}

	for (u64 li = 0; li < level_asset->sublevels.count; li++) {
		emp_sublevel_t* sublevel = level_asset->sublevels.entries + li;

		emp_texture_t* texture = emp_texture_find(sublevel->tiles.tilemap);
		if (texture == NULL) {
			continue;
		}
		for (u64 ti = 0; ti < sublevel->tiles.count; ti++) {
			float grid_size = sublevel->values.grid_size;
			emp_tile_desc_t* desc = sublevel->tiles.values + ti;
			u64 lx = (u64)(desc->dst.x / grid_size);
			u64 ly = (u64)(desc->dst.y / grid_size);

			size_t index = (size_t)(ly * sublevel->values.grid_width) + (size_t)lx;
			u8 value = sublevel->values.entries[index];

			SDL_FRect src = { desc->src.x, desc->src.y, grid_size, grid_size };
			emp_vec2_t pos = emp_vec2_add(desc->dst, sublevel->offset);
			u64 wx = (u64)(pos.x / grid_size);
			u64 wy = (u64)(pos.y / grid_size);

			u64 di = (wy * EMP_LEVEL_WIDTH) + wx;
			emp_tile_t* tile = &G->level->tiles[di];
			emp_tile_health_t* health = &G->level->health[di];

			if (value == 2) {
				if (health->value == 0) {
					continue;
				}
			}

			pos = emp_vec2_mul(pos, SPRITE_MAGNIFICATION);
			SDL_FRect dst = render_rect_with_size(pos, grid_size * SPRITE_MAGNIFICATION);
			SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);

			if (value == 1) {
				tile->state = emp_tile_state_occupied;
				draw_rect_at(pos, grid_size * SPRITE_MAGNIFICATION, 255, 0, 0, 255);
			}
			if (value == 2) {
				tile->state = emp_tile_state_breakable;
				draw_rect_at(pos, grid_size * SPRITE_MAGNIFICATION, 255, 255, 0, 255);
			}
		}
	}
	SDL_memset(G->level->enemy_in_tile, 0, sizeof(emp_enemy_t*) * EMP_LEVEL_TILES);
}

void emp_generator_uptdate(emp_bullet_generator_t* generator)
{
}

void emp_entities_init()
{
	G->player = SDL_malloc(sizeof(emp_player_t) * EMP_MAX_PLAYERS);
	G->enemies = SDL_malloc(sizeof(emp_enemy_t) * EMP_MAX_ENEMIES);
	G->bullets = SDL_malloc(sizeof(emp_bullet_t) * EMP_MAX_BULLETS);
	G->generators = SDL_malloc(sizeof(emp_bullet_generator_t) * EMP_MAX_BULLET_GENERATORS);
	G->spawners = SDL_malloc(sizeof(emp_spawner_t) * EMP_MAX_SPAWNERS);

	SDL_memset(G->player, 0, sizeof(emp_player_t) * EMP_MAX_PLAYERS);
	SDL_memset(G->enemies, 0, sizeof(emp_enemy_t) * EMP_MAX_ENEMIES);
	SDL_memset(G->bullets, 0, sizeof(emp_bullet_t) * EMP_MAX_BULLETS);
	SDL_memset(G->generators, 0, sizeof(emp_bullet_generator_t) * EMP_MAX_BULLET_GENERATORS);
	SDL_memset(G->spawners, 0, sizeof(emp_spawner_t) * EMP_MAX_SPAWNERS);
}

int emp_teleporter_uptdate(emp_level_teleporter_t const* teleporter)
{
	emp_asset_t* texture_asset = &G->assets->png->cave_32;
	emp_texture_t* texture = texture_asset->handle;
	SDL_FRect src = source_rect(texture_asset);
	emp_vec2_t pos = (emp_vec2_t) {
		.x = (teleporter->x - (float)EMP_TILE_SIZE / 2) * SPRITE_MAGNIFICATION,
		.y = (teleporter->y - (float)EMP_TILE_SIZE / 2) * SPRITE_MAGNIFICATION,
	};
	SDL_FRect dst = render_rect(pos, texture);

	emp_vec2_t centre = (emp_vec2_t){.x = dst.x + (dst.w / 2), .y = dst.y + (dst.h / 2) };

	int on_teleporter = 0;
	float distance = emp_vec2_dist(G->player->pos, pos);

	emp_vec2_t mouse_pos;
	SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
	// buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);
	//(void)buttons;
	int is_hovering = emp_vec2_dist(mouse_pos, centre) < EMP_TILE_SIZE* SPRITE_MAGNIFICATION;
	float required = EMP_TILE_SIZE * SPRITE_MAGNIFICATION * (G->player->is_teleporting ? 2.0f : 1.0f);
	if (distance < required) {
		if (buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT) && is_hovering) {
			if (G->player->is_teleporting == 0 /*&& G->player->was_teleporting == 0*/) {
				emp_level_asset_t* level = (emp_level_asset_t*)G->assets->ldtk->world.handle;
				u32 found = emp_level_teleporter_list_find(&level->teleporters, teleporter->other);
				if (found) {
					u32 at = found - 1;
					emp_level_teleporter_t const* tp = level->teleporters.entries + at;

					emp_vec2_t new_pos = (emp_vec2_t) { .x = tp->x, .y = tp->y };
					new_pos = emp_vec2_mul(new_pos, SPRITE_MAGNIFICATION);
					G->player->pos.x = new_pos.x - (EMP_TILE_SIZE / 2.0f);
					G->player->pos.y = new_pos.y - (EMP_TILE_SIZE / 2.0f);
					G->player->is_teleporting = 1;
				}
			}
			G->player->is_teleporting = 1;
			on_teleporter = G->player->is_teleporting;
		}
	}

	if (G->player->is_teleporting) {

		SDL_SetTextureAlphaMod(texture->texture, 64);
		SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
		SDL_SetTextureAlphaMod(texture->texture, 255);
	} else {
		SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
	}

	// SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
	draw_rect_at(pos, teleporter->w * SPRITE_MAGNIFICATION, 255, 0, 0, 255);
	return on_teleporter;
}

void emp_spawner_update(u32 index, emp_spawner_t* spawner)
{
	emp_vec2_t pos = (emp_vec2_t) { .x = spawner->x, .y = spawner->y };
	if (spawner->count < spawner->limit) {
		spawner->accumulator = spawner->accumulator + G->args->dt;
		if (spawner->accumulator > spawner->frequency) {
			spawner->accumulator = spawner->accumulator - spawner->frequency;
			spawner->count = spawner->count + 1;
			emp_create_enemy(pos, spawner->enemy_conf_index, spawner->weapon_index, (emp_spawner_h) { .index = index });
		}
	}

	emp_asset_t* texture_asset = &G->assets->png->cave2_32;
	emp_texture_t* texture = texture_asset->handle;
	SDL_FRect src = source_rect(texture_asset);
	SDL_FRect dst = render_rect(pos, texture);
	SDL_RenderTexture(G->renderer, texture->texture, &src, &dst);
}

void emp_entities_update()
{
	emp_level_update();

	int is_teleporting = 0;
	emp_level_asset_t* level = (emp_level_asset_t*)G->assets->ldtk->world.handle;
	for (u64 i = 0; i < SDL_arraysize(level->teleporters.entries); i++) {
		emp_level_teleporter_t* tp = level->teleporters.entries + i;
		if (tp->id != 0) {
			is_teleporting = emp_teleporter_uptdate(tp) || is_teleporting;
		}
	}
	G->player->is_teleporting = is_teleporting;

	for (u64 i = 0; i < EMP_MAX_PLAYERS; ++i) {
		emp_player_t* player = &G->player[i];
		if (player->alive) {
			emp_player_update(player);
		}
	}

	for (u64 i = 0; i < EMP_MAX_ENEMIES; ++i) {
		emp_enemy_t* enemy = &G->enemies[i];
		if (enemy->alive) {
			emp_enemy_update(enemy);
		}
	}

	for (u64 i = 0; i < EMP_MAX_BULLETS; ++i) {
		emp_bullet_t* bullet = &G->bullets[i];
		if (bullet->alive) {
			emp_bullet_update(bullet);
		}
	}

	for (u32 i = 0; i < EMP_MAX_SPAWNERS; ++i) {
		emp_spawner_t* spawner = &G->spawners[i];
		if (spawner->alive) {
			emp_spawner_update(i, spawner);
		}
	}

	for (u64 i = 0; i < EMP_MAX_BULLET_GENERATORS; ++i) {
		emp_bullet_generator_t* generator = &G->generators[i];
		if (generator->alive) {
			emp_generator_uptdate(generator);
		}
	}
}

void setup_level(emp_asset_t* level_asset)
{
	SDL_memset(G->enemies, 0, sizeof(emp_enemy_t) * EMP_MAX_ENEMIES);
	SDL_memset(G->bullets, 0, sizeof(emp_bullet_t) * EMP_MAX_BULLETS);
	SDL_memset(G->generators, 0, sizeof(emp_bullet_generator_t) * EMP_MAX_BULLET_GENERATORS);
	SDL_memset(G->spawners, 0, sizeof(emp_spawner_t) * EMP_MAX_SPAWNERS);

	u32 player = emp_create_player();
	G->player[player].texture_asset = &G->assets->png->player_32;

	emp_level_asset_t* level = (emp_level_asset_t*)level_asset->handle;
	u32 found = emp_level_query(level, emp_entity_type_player, 0);
	if (found) {
		emp_level_entity_t* player_entity = emp_level_get(level, found - 1);
		float half = level->entities.grid_size * 0.5f;
		float x = player_entity->x - half;
		float y = player_entity->y - half;
		G->player[player].pos.x = x * SPRITE_MAGNIFICATION;
		G->player[player].pos.y = y * SPRITE_MAGNIFICATION;
	} else {
		SDL_Log("No Player config broke!");
	}

	found = 0;
	for (;;) {
		found = emp_level_query(level, emp_entity_type_spawner, found);
		if (!found) {
			break;
		}

		emp_level_entity_t* spawner = emp_level_get(level, found - 1);
		emp_vec2_t pos = (emp_vec2_t) {
			.x = (spawner->x - (float)EMP_TILE_SIZE / 2) * SPRITE_MAGNIFICATION,
			.y = (spawner->y - (float)EMP_TILE_SIZE / 2) * SPRITE_MAGNIFICATION,
		};
		// Maybe just pass down the fucking asset lol
		emp_create_spawner(pos, 10, 0, spawner->weapon_index, spawner->frequency, spawner->limit);
	}

	found = 0;
	for (;;) {
		found = emp_level_query(level, emp_entity_type_boss, found);
		if (!found) {
			break;
		}

		float half = level->entities.grid_size * 0.5f;
		emp_level_entity_t* boss = emp_level_get(level, found - 1);
		float x = boss->x - half;
		float y = boss->y - half;

		emp_create_enemy((emp_vec2_t) { x * SPRITE_MAGNIFICATION, y * SPRITE_MAGNIFICATION }, 1, boss->weapon_index, (emp_spawner_h) { .index = EMP_MAX_SPAWNERS });
	}

	for (u64 li = 0; li < level->sublevels.count; li++) {
		emp_sublevel_t* sublevel = level->sublevels.entries + li;
		for (u64 ti = 0; ti < sublevel->tiles.count; ti++) {
			float grid_size = sublevel->values.grid_size;
			emp_tile_desc_t* desc = sublevel->tiles.values + ti;
			u64 lx = (u64)(desc->dst.x / grid_size);
			u64 ly = (u64)(desc->dst.y / grid_size);

			size_t index = (size_t)(ly * sublevel->values.grid_width) + (size_t)lx;
			u8 value = sublevel->values.entries[index];

			emp_vec2_t pos = emp_vec2_add(desc->dst, sublevel->offset);
			u64 wx = (u64)(pos.x / grid_size);
			u64 wy = (u64)(pos.y / grid_size);

			u64 di = (wy * EMP_LEVEL_WIDTH) + wx;
			emp_tile_health_t* health = &G->level->health[di];
			if (value == 2) {
				health->value = 10;
			}
		}
	}
}

void emp_create_level(emp_asset_t* level_asset, int is_reload)
{
	if (!is_reload) {
		G->level = SDL_malloc(sizeof(emp_level_t));
		G->level->tiles = SDL_malloc(sizeof(*G->level->tiles) * EMP_LEVEL_TILES);
		G->level->health = SDL_malloc(sizeof(*G->level->health) * EMP_LEVEL_TILES);
		G->level->enemy_in_tile = SDL_malloc(sizeof(emp_enemy_t*) * EMP_LEVEL_TILES);
	}
	emp_tile_t* tiles = G->level->tiles;
	emp_tile_health_t* health = G->level->health;
	emp_enemy_t** enemy_in_tile = G->level->enemy_in_tile;
	SDL_memset(G->level->tiles, 0, sizeof(*G->level->tiles) * EMP_LEVEL_TILES);
	SDL_memset(G->level->health, 0, sizeof(*G->level->health) * EMP_LEVEL_TILES);
	SDL_memset(G->level->enemy_in_tile, 0, sizeof(emp_enemy_t*) * EMP_LEVEL_TILES);
	SDL_zerop(G->level);
	G->level->tiles = tiles;
	G->level->health = health;
	G->level->enemy_in_tile = enemy_in_tile;
	SDL_Log("%s", level_asset->path);
	setup_level(&G->assets->ldtk->world);
}

void emp_destroy_level(void)
{
	SDL_free(G->level->tiles);
	SDL_free(G->level);
}