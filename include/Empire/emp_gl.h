#pragma once

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
	emp_mesh_gpu_t mesh;
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