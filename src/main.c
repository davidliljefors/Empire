#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Empire/types.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#include <glad/gl.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 2200
#define WINDOW_HEIGHT 1400

#include "entities.h"

#include <Empire/fast_obj.h>
#include <Empire/generated/assets_generated.h>
#include <Empire/stb_image.h>

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

#ifdef __EMSCRIPTEN__
#define SHADER_HEADER "#version 300 es\nprecision highp float;\n"
#else
#define SHADER_HEADER "#version 330 core\n"
#endif

static char* load_shader_from_asset(emp_buffer shader_data)
{
	if (!shader_data.data || shader_data.size == 0) {
		SDL_Log("Failed to load shader: invalid asset data");
		return NULL;
	}

	size_t header_len = strlen(SHADER_HEADER);
	char* result = (char*)SDL_malloc(header_len + shader_data.size + 1);
	memcpy(result, SHADER_HEADER, header_len);
	memcpy(result + header_len, shader_data.data, shader_data.size);
	result[header_len + shader_data.size] = '\0';

	return result;
}

typedef struct
{
	float position[3];
	float normal[3];
	float texcoord[2];
} emp_vertex_t;

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_GLContext g_gl_context = NULL;
static emp_generated_assets_o* g_assets = NULL;
static emp_asset_manager_o* g_asset_mgr = NULL;
static GLuint g_shader_program = 0;
static GLint g_mvp_loc = -1;
static GLint g_has_texture_loc = -1;
static float g_angle_x = 0.0f;
static float g_angle_y = 0.0f;
static Uint64 g_last_time = 0;
static bool g_running = true;

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	unsigned int index_count;
} emp_mesh_gpu_t;

static GLuint compile_shader(GLenum type, const char* src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		SDL_Log("Shader compile error: %s", log);
		return 0;
	}
	return shader;
}

static GLuint create_shader_program_from_assets(emp_buffer vert_data, emp_buffer frag_data)
{
	char* vert_src = load_shader_from_asset(vert_data);
	char* frag_src = load_shader_from_asset(frag_data);
	if (!vert_src || !frag_src) {
		SDL_free(vert_src);
		SDL_free(frag_src);
		return 0;
	}

	GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
	GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
	SDL_free(vert_src);
	SDL_free(frag_src);

	if (!vs || !fs)
		return 0;

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char log[512];
		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		SDL_Log("Shader link error: %s", log);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
	return program;
}

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

	emp_update_args_t update_args;

	Uint64 current_time = SDL_GetTicks();
	float delta_time = (current_time - g_last_time) / 1000.0f;
	g_last_time = current_time;

	g_angle_x += 0.5f * delta_time;
	g_angle_y += 0.8f * delta_time;

	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);

	// glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_program);

	emp_mat4_t model = mat4_mul(mat4_make_rotation_x(g_angle_x), mat4_make_rotation_y(g_angle_y));
	emp_mat4_t view = mat4_make_translation(0.0f, 0.0f, -3.0f);
	emp_mat4_t proj = mat4_make_perspective((float)(M_PI / 4.0), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
	emp_mat4_t mvp = mat4_mul(proj, mat4_mul(view, model));

	glUniformMatrix4fv(g_mvp_loc, 1, GL_FALSE, mvp.m);

	// Set texture uniform (0 = no texture, for now)
	glUniform1i(g_has_texture_loc, 0);

	emp_mesh_gpu_t* cube_mesh = (emp_mesh_gpu_t*)g_assets->obj->cube.handle;
	if (cube_mesh) {
		glBindVertexArray(cube_mesh->vao);
		glDrawElements(GL_TRIANGLES, cube_mesh->index_count, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	glDisable(GL_DEPTH_TEST);

	update_args.dt = delta_time;
	update_args.assets = g_assets;
	update_args.r = g_renderer;
	emp_entities_update(&update_args);

	SDL_RenderPresent(g_renderer);

	SDL_GL_SwapWindow(g_window);
}

// Hash function for vertex deduplication (FNV-1a)
static unsigned int hash_vertex(emp_vertex_t* v)
{
	unsigned int hash = 2166136261u;
	unsigned char* bytes = (unsigned char*)v;
	for (size_t i = 0; i < sizeof(emp_vertex_t); i++) {
		hash ^= bytes[i];
		hash *= 16777619u;
	}
	return hash;
}

static int vertices_equal(emp_vertex_t* a, emp_vertex_t* b)
{
	return memcmp(a, b, sizeof(emp_vertex_t)) == 0;
}

void emp_mesh_load_func(emp_asset_t* asset)
{
	fastObjMesh* obj = fast_obj_read(asset->path);
	if (!obj) {
		SDL_Log("Failed to load mesh from %s", asset->path);
		return;
	}

	// First pass: count how many triangles we'll have after triangulation
	unsigned int triangle_count = 0;
	for (unsigned int i = 0; i < obj->face_count; i++) {
		unsigned int verts_in_face = obj->face_vertices[i];
		if (verts_in_face >= 3) {
			triangle_count += verts_in_face - 2;
		}
	}

	unsigned int max_vertices = triangle_count * 3;
	emp_vertex_t* vertices = (emp_vertex_t*)SDL_malloc(max_vertices * sizeof(emp_vertex_t));
	unsigned int* indices = (unsigned int*)SDL_malloc(max_vertices * sizeof(unsigned int));

	// Simple hash table for vertex deduplication
	unsigned int hash_size = max_vertices * 2;
	int* hash_table = (int*)SDL_malloc(hash_size * sizeof(int));
	memset(hash_table, -1, hash_size * sizeof(int));
	int* hash_next = (int*)SDL_malloc(max_vertices * sizeof(int));

	unsigned int unique_vertex_count = 0;
	unsigned int index_count = 0;

	// Second pass: triangulate faces and build vertex/index buffers with deduplication
	unsigned int idx_offset = 0;

	for (unsigned int face = 0; face < obj->face_count; face++) {
		unsigned int verts_in_face = obj->face_vertices[face];

		// Fan triangulation
		for (unsigned int v = 2; v < verts_in_face; v++) {
			unsigned int face_indices[3] = { 0, v - 1, v };

			for (int t = 0; t < 3; t++) {
				fastObjIndex idx = obj->indices[idx_offset + face_indices[t]];

				// Build vertex
				emp_vertex_t vert = { 0 };
				if (idx.p) {
					vert.position[0] = obj->positions[idx.p * 3 + 0];
					vert.position[1] = obj->positions[idx.p * 3 + 1];
					vert.position[2] = obj->positions[idx.p * 3 + 2];
				}
				if (idx.n) {
					vert.normal[0] = obj->normals[idx.n * 3 + 0];
					vert.normal[1] = obj->normals[idx.n * 3 + 1];
					vert.normal[2] = obj->normals[idx.n * 3 + 2];
				} else {
					vert.normal[1] = 1.0f;
				}
				if (idx.t) {
					vert.texcoord[0] = obj->texcoords[idx.t * 2 + 0];
					vert.texcoord[1] = obj->texcoords[idx.t * 2 + 1];
				}

				// Look up in hash table
				unsigned int hash = hash_vertex(&vert) % hash_size;
				int found_idx = -1;
				for (int hi = hash_table[hash]; hi != -1; hi = hash_next[hi]) {
					if (vertices_equal(&vertices[hi], &vert)) {
						found_idx = hi;
						break;
					}
				}

				if (found_idx == -1) {
					// New unique vertex
					found_idx = unique_vertex_count;
					vertices[unique_vertex_count] = vert;
					hash_next[unique_vertex_count] = hash_table[hash];
					hash_table[hash] = unique_vertex_count;
					unique_vertex_count++;
				}

				indices[index_count++] = found_idx;
			}
		}

		idx_offset += verts_in_face;
	}

	SDL_free(hash_table);
	SDL_free(hash_next);

	emp_mesh_gpu_t* gpu_mesh = (emp_mesh_gpu_t*)SDL_malloc(sizeof(emp_mesh_gpu_t));
	gpu_mesh->index_count = index_count;

	glGenVertexArrays(1, &gpu_mesh->vao);
	glGenBuffers(1, &gpu_mesh->vbo);
	glGenBuffers(1, &gpu_mesh->ebo);

	glBindVertexArray(gpu_mesh->vao);

	glBindBuffer(GL_ARRAY_BUFFER, gpu_mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, unique_vertex_count * sizeof(emp_vertex_t), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu_mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	SDL_Log("Loaded mesh: %u unique vertices, %u indices (%.1f%% vertex reuse)",
		unique_vertex_count, index_count,
		(1.0f - (float)unique_vertex_count / index_count) * 100.0f);

	SDL_free(vertices);
	SDL_free(indices);
	fast_obj_destroy(obj);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, position));
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, normal));
	glEnableVertexAttribArray(1);

	// Texture coordinate attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, texcoord));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	asset->handle = gpu_mesh;
}

void emp_mesh_unload_func(emp_asset_t* asset)
{
	if (!asset->handle)
		return;

	emp_mesh_gpu_t* gpu_mesh = (emp_mesh_gpu_t*)asset->handle;

	glDeleteVertexArrays(1, &gpu_mesh->vao);
	glDeleteBuffers(1, &gpu_mesh->vbo);
	glDeleteBuffers(1, &gpu_mesh->ebo);

	SDL_free(gpu_mesh);
	asset->handle = NULL;
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

#ifdef __EMSCRIPTEN__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_CreateWindowAndRenderer("Empire", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL, &g_window, &g_renderer);

	SDL_SetRenderVSync(g_renderer, 1);

	if (!g_window) {
		SDL_Log("Failed to create window: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	g_gl_context = SDL_GL_CreateContext(g_window);
	if (!g_gl_context) {
		SDL_Log("Failed to create GL context: %s", SDL_GetError());
		SDL_DestroyWindow(g_window);
		SDL_Quit();
		return 1;
	}

#ifndef __EMSCRIPTEN__
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		SDL_Log("Failed to load OpenGL functions");
		SDL_GL_DestroyContext(g_gl_context);
		SDL_DestroyWindow(g_window);
		SDL_Quit();
		return 1;
	}
#endif

	glEnable(GL_DEPTH_TEST);

	const char* root = get_asset_argument(argc, argv);

	g_assets = emp_generated_assets_create(root);
	g_asset_mgr = emp_asset_manager_create(g_assets);
	emp_asset_loader_t mesh_loader = {
		.load = &emp_mesh_load_func,
		.unload = &emp_mesh_unload_func,
	};

	emp_asset_loader_t png_loader = {
		.load = &emp_png_load_func,
		.unload = &emp_png_unload_func,
	};

	emp_asset_manager_add_loader(g_asset_mgr, mesh_loader, EMP_ASSET_TYPE_OBJ);
	emp_asset_manager_add_loader(g_asset_mgr, png_loader, EMP_ASSET_TYPE_PNG);
	emp_asset_manager_check_hot_reload(g_asset_mgr);

	g_shader_program = create_shader_program_from_assets(g_assets->vert->cube.data, g_assets->frag->cube.data);
	emp_entities_init();
	u32 player = emp_create_player();
	G->player[player].texture_asset = &g_assets->png->base;

	if (!g_shader_program) {
		SDL_Log("Failed to create shader program");
		return 1;
	}
	g_mvp_loc = glGetUniformLocation(g_shader_program, "u_mvp");
	g_has_texture_loc = glGetUniformLocation(g_shader_program, "u_has_texture");

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

	glDeleteProgram(g_shader_program);

	SDL_GL_DestroyContext(g_gl_context);
	SDL_DestroyWindow(g_window);
	SDL_Quit();

	return 0;
}
