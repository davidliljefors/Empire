#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Empire/types.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 2200
#define WINDOW_HEIGHT 1200

#include "entities.h"

#include <Empire/fast_obj.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/stb_image.h>
#include <Empire/text.h>

typedef struct
{
	float m[16];
} emp_mat4_t;

static emp_mat4_t mat4_identity(void)
{
	emp_mat4_t r = { 0 };
	r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
	return r;
}

static emp_mat4_t mat4_mul(emp_mat4_t a, emp_mat4_t b)
{
	emp_mat4_t r = { 0 };
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 4; row++) {
			for (int k = 0; k < 4; k++) {
				r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
			}
		}
	}
	return r;
}

static emp_mat4_t mat4_make_rotation_x(float angle)
{
	emp_mat4_t r = mat4_identity();
	float c = cosf(angle), s = sinf(angle);
	r.m[5] = c;
	r.m[9] = -s;
	r.m[6] = s;
	r.m[10] = c;
	return r;
}

static emp_mat4_t mat4_make_rotation_y(float angle)
{
	emp_mat4_t r = mat4_identity();
	float c = cosf(angle), s = sinf(angle);
	r.m[0] = c;
	r.m[8] = s;
	r.m[2] = -s;
	r.m[10] = c;
	return r;
}

static emp_mat4_t mat4_make_perspective(float fov, float aspect, float nearZ, float farZ)
{
	emp_mat4_t r = { 0 };
	float f = 1.0f / tanf(fov / 2.0f);
	r.m[0] = f / aspect;
	r.m[5] = f;
	r.m[10] = (farZ + nearZ) / (nearZ - farZ);
	r.m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
	r.m[11] = -1.0f;
	return r;
}

static emp_mat4_t mat4_make_translation(float x, float y, float z)
{
	emp_mat4_t r = mat4_identity();
	r.m[12] = x;
	r.m[13] = y;
	r.m[14] = z;
	return r;
}

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static emp_generated_assets_o* g_assets = NULL;
static emp_asset_manager_o* g_asset_mgr = NULL;
static float g_angle_x = 0.0f;
static float g_angle_y = 0.0f;
static Uint64 g_last_time = 0;
static bool g_running = true;

//typedef struct sprite_t {
//	
//}sprite_t;

static void main_loop(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			g_running = false;
		}
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
			g_running = false;
		}
	}


	Uint64 current_time = SDL_GetTicks();
	float delta_time = (current_time - g_last_time) / 1000.0f;
	g_last_time = current_time;

	emp_update_args_t update_args;
	update_args.dt = delta_time;
	update_args.assets = g_assets;
	update_args.r = g_renderer;
	G->args = &update_args;

	g_angle_x += 0.5f * delta_time;
	g_angle_y += 0.8f * delta_time;

	SDL_SetRenderDrawColor(g_renderer, 108, 129, 161, 1);
	SDL_RenderClear(g_renderer);

	// glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	//emp_gl_clear(0.1f, 0.1f, 0.15f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//emp_ubo_clear(&g_material);

	emp_mat4_t model = mat4_mul(mat4_make_rotation_x(g_angle_x), mat4_make_rotation_y(g_angle_y));
	emp_mat4_t view = mat4_make_translation(0.0f, 0.0f, -3.0f);
	emp_mat4_t proj = mat4_make_perspective((float)(M_PI / 4.0), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
	emp_mat4_t mvp = mat4_mul(proj, mat4_mul(view, model));
	(void)mvp;
	//emp_ubo_push_from_name(&g_material, emp_uniform_matrix4x4, "u_mvp", mvp.m);

	//int zero = 0;
	//emp_ubo_push_from_name(&g_material, emp_uniform_int, "u_has_texture", &zero);

	//emp_mesh_gpu_t* cube_mesh = (emp_mesh_gpu_t*)g_assets->obj->cylinder.handle;
	//emp_gpu_instance_list_t render_list = { 0 };
	//emp_gpu_instance_list_add(&render_list, &(emp_gpu_instance_t) { .mesh = cube_mesh });
	//emp_gpu_instance_list_render(&g_material, &render_list);

	emp_draw_text(100, 100, "hello world", &g_assets->ttf->bauhs93);

	//emp_gl_depth_test_disable();

	emp_entities_update(&update_args);

	SDL_RenderPresent(g_renderer);

	//emp_gl_depth_test_enable();

	SDL_GL_SwapWindow(g_window);
}

void emp_png_load_func(emp_asset_t* asset)
{
	int width, height, channels;
	unsigned char* data = stbi_load(asset->path, &width, &height, &channels, 4);

	SDL_Surface* surface = SDL_CreateSurfaceFrom(
		width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);

	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	SDL_DestroySurface(surface);
	stbi_image_free(data);
	emp_texture_t* emp_tex = SDL_malloc(sizeof(emp_texture_t));

	emp_tex->texture = texture;
	emp_tex->width = width;
	emp_tex->height = height;

	asset->handle = emp_tex;
}

void emp_png_unload_func(emp_asset_t* asset)
{
	emp_texture_t* emp_tex = asset->handle;
	SDL_DestroyTexture(emp_tex->texture);
	SDL_free(emp_tex);
}

const char* get_asset_argument(int argc, char* arguments[])
{
	for (int index = 0; index < argc; index++) {
		char* arg = arguments[index];
		const char token[] = "cwd=";
		char* path = SDL_strstr(arg, token);

		if (path != NULL) {
			return path + sizeof(token) - 1; // Null terminator
		}
	}
	return "";
}

int main(int argc, char* argv[])
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_CreateWindowAndRenderer("Empire", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL, &g_window, &g_renderer);

	SDL_SetRenderVSync(g_renderer, 0);
	SDL_SetRenderScale(g_renderer, 2, 2);
	

	if (!g_window) {
		SDL_Log("Failed to create window: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	const char* root = get_asset_argument(argc, argv);

	g_assets = emp_generated_assets_create(root);
	g_asset_mgr = emp_asset_manager_create(g_assets);

	emp_asset_loader_t png_loader = {
		.load = &emp_png_load_func,
		.unload = &emp_png_unload_func,
	};

	emp_load_font(g_renderer, &g_assets->ttf->bauhs93, 84.0f);
	emp_asset_manager_add_loader(g_asset_mgr, mesh_loader, EMP_ASSET_TYPE_OBJ);
	emp_asset_manager_add_loader(g_asset_mgr, png_loader, EMP_ASSET_TYPE_PNG);
	emp_asset_manager_check_hot_reload(g_asset_mgr);

	g_material = emp_material_create(g_assets->vert->cube.data, g_assets->frag->cube.data);

	emp_entities_init();

	u32 player = emp_create_player();
	G->player[player].texture_asset = &g_assets->png->base;
	G->assets = g_assets;

	emp_init_weapon_configs();

	g_last_time = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, 0, 1);
#else
	u64 last_time = SDL_GetTicks() - 900;
	u64 frame_count = 0;
	while (g_running) {
		frame_count++;
		u64 currentTime = SDL_GetTicks();
		if (currentTime - last_time >= 1000) {
			char title[64];
			SDL_snprintf(title, sizeof(title), "My App - FPS: %llu", frame_count);
			SDL_SetWindowTitle(g_window, title);
			frame_count = 0;
			last_time = currentTime;
		}

		main_loop();
		emp_asset_manager_check_hot_reload(g_asset_mgr);
	}
#endif
	SDL_DestroyWindow(g_window);
	SDL_Quit();

	return 0;
}
