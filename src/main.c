#include "SDL3_mixer/SDL_mixer.h"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <Empire/types.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#include "entities.h"

#include <Empire/fast_obj.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/level.h>
#include <Empire/stb_image.h>
#include <Empire/text.h>

typedef struct
{
	float m[16];
} emp_mat4_t;

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static emp_generated_assets_o* g_assets = NULL;
static emp_asset_manager_o* g_asset_mgr = NULL;
static Uint64 g_last_time = 0;
static bool g_running = true;

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
	double delta_time = (current_time - g_last_time) / 1000.0;
	delta_time = SDL_min(delta_time, 0.5f);
	g_last_time = current_time;
	G->args->dt = (float)delta_time;
	G->args->global_time += delta_time;

	SDL_SetRenderDrawColor(g_renderer, 17, 25, 45, 1);
	SDL_RenderClear(g_renderer);

	emp_entities_update();

	char buffer2[64];
	SDL_snprintf(buffer2, sizeof(buffer2), "Under the C");
	emp_draw_text(WINDOW_WIDTH / 2 - 200, 100, buffer2, &g_assets->ttf->asepritefont);

	char buffer[64];
	SDL_snprintf(buffer, sizeof(buffer), "Health: %.0f/%.0f", G->player->health, G->player->max_health);
	emp_draw_text(50, WINDOW_HEIGHT - 50, buffer, &g_assets->ttf->asepritefont);

	SDL_RenderPresent(g_renderer);

	SDL_GL_SwapWindow(g_window);
}

int parse_atlas_width_from_path_name(const char* path)
{
	const char* dot = SDL_strrchr(path, '.');
	const char* underscore = SDL_strrchr(path, '_');
	if (underscore && (!dot || underscore < dot)) {
		return SDL_atoi(underscore + 1);
	}
	return 0;
}

void emp_png_load_func(emp_asset_t* asset)
{
	int width, height, channels;
	unsigned char* data = stbi_load(asset->path, &width, &height, &channels, 4);

	SDL_Surface* surface = SDL_CreateSurfaceFrom(
		width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);

	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	SDL_DestroySurface(surface);
	stbi_image_free(data);
	emp_texture_t* emp_tex = SDL_malloc(sizeof(emp_texture_t));

	emp_tex->texture = texture;

	int atlas_size = parse_atlas_width_from_path_name(asset->path);
	if (atlas_size) {
		emp_tex->source_size = (u32)(atlas_size);
		emp_tex->columns = width / atlas_size;
		emp_tex->rows = height / atlas_size;
		emp_tex->width = atlas_size * SPRITE_MAGNIFICATION;
		emp_tex->height = atlas_size * SPRITE_MAGNIFICATION;
	} else {
		emp_tex->source_size = width;
		emp_tex->rows = 1;
		emp_tex->columns = 1;
		emp_tex->width = width * SPRITE_MAGNIFICATION;
		emp_tex->height = height * SPRITE_MAGNIFICATION;
	}

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

void emp_load_wav_asset(struct emp_asset_t* asset)
{
	asset->handle = MIX_LoadAudio(G->mixer, asset->path, 0);
}
void emp_unload_wav_asset(struct emp_asset_t* asset)
{
	MIX_DestroyAudio(asset->handle);
}

int main(int argc, char* argv[])
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_CreateWindowAndRenderer("Empire", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &g_window, &g_renderer);

	SDL_SetRenderVSync(g_renderer, 1);
	// SDL_SetRenderScale(g_renderer, 2, 2);

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

	emp_asset_loader_t ldtk_loader = {
		.load = &emp_load_level_asset,
		.unload = &emp_unload_level_asset,
	};

	emp_asset_loader_t wav_loader = {
		.load = &emp_load_wav_asset,
		.unload = &emp_unload_wav_asset,
	};

	G = SDL_malloc(sizeof(emp_G));
	// SDL_AudioSpec spec;
	MIX_Init();
	G->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
	if (!G->mixer) {
		SDL_Log("Couldn't create mixer on default device: %s", SDL_GetError());
	}

	emp_load_font(g_renderer, &g_assets->ttf->bauhs93, 84.0f);
	emp_load_font(g_renderer, &g_assets->ttf->asepritefont, 84.0f);
	emp_asset_manager_add_loader(g_asset_mgr, png_loader, EMP_ASSET_TYPE_PNG);
	emp_asset_manager_add_loader(g_asset_mgr, ldtk_loader, EMP_ASSET_TYPE_LDTK);
	emp_asset_manager_add_loader(g_asset_mgr, wav_loader, EMP_ASSET_TYPE_WAV);

	emp_asset_manager_check_hot_reload(g_asset_mgr, 10.0f);

	G->assets = g_assets;
	G->args = SDL_malloc(sizeof(emp_update_args_t));
	G->renderer = g_renderer;

	emp_entities_init();
	emp_init_enemy_configs();
	emp_init_weapon_configs();

	emp_create_level(&G->assets->ldtk->world, 0);

	emp_music_player_init();

	SDL_zerop(G->args);
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

		//i32 preferred_track = 0;
		//for(i32 index = 0; index < SDL_arraysize(track_steps); index++) {
		//	if(G->player->pos.x < track_steps[index] * 4) 
		//	{
		//		preferred_track = index;
		//		break;
		//	}
		//}
		//preferred_track = SDL_min(preferred_track, SDL_arraysize(tracks));
		//if (preferred_track != current_track) {
		//	MIX_StopTrack(tracks[current_track], MIX_TrackMSToFrames(tracks[current_track], 1000));
		//	current_track = preferred_track;
		//	MIX_PlayTrack(tracks[current_track], options);
		//}

		//Sint64 remaining = MIX_GetTrackRemaining(tracks[current_track]);
		//Sint64 ms = MIX_TrackFramesToMS(tracks[current_track], remaining);
		//if (ms == 0) {
		//	MIX_PlayTrack(tracks[current_track], 0);
		//}

		main_loop();
		emp_asset_manager_check_hot_reload(g_asset_mgr, G->args->dt);
	}
#endif
	SDL_DestroyWindow(g_window);
	SDL_Quit();

	return 0;
}
