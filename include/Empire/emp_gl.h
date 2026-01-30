#pragma once

#include "Empire/types.h"

typedef struct emp_vertex_t
{
	float position[3];
	float normal[3];
	float texcoord[2];
} emp_vertex_t;

typedef enum emp_uniform_type {
	emp_uniform_float = 1,
	emp_uniform_matrix4x4 = 2,
	emp_uniform_int = 3
} emp_uniform_type;

typedef struct emp_uniform_t
{
	emp_uniform_type type;
	int location;
	void* stable_data;
} emp_uniform_t;

typedef struct emp_ubo_t
{
	emp_uniform_t uniforms[16];
	size_t count;
} emp_ubo_t;

typedef struct emp_material_t
{
	unsigned int shader_program;
	emp_ubo_t ubo;
} emp_material_t;

typedef struct emp_mesh_gpu_t
{
	unsigned int vao;
	unsigned int vbo;
	unsigned int ebo;
	unsigned int index_count;
} emp_mesh_gpu_t;

typedef struct emp_gpu_instance_t
{
	emp_mesh_gpu_t* mesh;
} emp_gpu_instance_t;

typedef struct emp_gpu_instance_list_t
{
	emp_gpu_instance_t entries[16];
	size_t count;
} emp_gpu_instance_list_t;

typedef struct emp_gpu_state_t
{
	emp_material_t material;
	emp_gpu_instance_list_t instances;
} emp_gpu_state_t;

struct SDL_Window;
struct emp_asset_t;

int emp_gl(struct SDL_Window* window);

void emp_gl_clear(float r, float g, float b, float a);
void emp_gl_depth_test_enable(void);
void emp_gl_depth_test_disable(void);

void emp_ubo_push(emp_material_t* ubo, emp_uniform_type type, int location, void* stable_data);
void emp_ubo_push_from_name(emp_material_t* material, emp_uniform_type type, const char* name, void* stable_data);
void emp_ubo_clear(emp_material_t* ubo);

void emp_material_bind(emp_material_t const* material);
void emp_mesh_draw(emp_mesh_gpu_t const* mesh);

void emp_gpu_instance_list_add(emp_gpu_instance_list_t* list, emp_gpu_instance_t const* value);
void emp_gpu_instance_list_render(emp_material_t const* material, emp_gpu_instance_list_t const* instances);

void emp_mesh_load_func(struct emp_asset_t* asset);
void emp_mesh_unload_func(struct emp_asset_t* asset);
unsigned int create_shader_program_from_assets(emp_buffer vert_data, emp_buffer frag_data);

emp_material_t emp_material_create(emp_buffer vert_data, emp_buffer frag_data);
void emp_material_destroy(emp_material_t* material);

