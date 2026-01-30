#include "Empire/emp_gl.h"

#include "Empire/types.h"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#include <glad/gl.h>
#endif

#include "Empire/assets.h"
#include "Empire/fast_obj.h"

#include <SDL3/SDL.h>

void emp_ubo_push(emp_material_t* material, emp_uniform_type type, int location, void* stable_data)
{
	emp_ubo_t* ubo = &material->ubo;
	emp_uniform_t* uniform = ubo->uniforms + ubo->count;
	uniform->type = type;
	uniform->location = location;
	uniform->stable_data = stable_data;
	ubo->count = ubo->count + 1;
}

void emp_ubo_push_from_name(emp_material_t* material, emp_uniform_type type, const char* name, void* stable_data)
{
	emp_ubo_t* ubo = &material->ubo;
	emp_uniform_t* uniform = ubo->uniforms + ubo->count;
	uniform->type = type;
	uniform->location = glGetUniformLocation(material->shader_program, name);
	uniform->stable_data = stable_data;
	ubo->count = ubo->count + 1;
}

void emp_ubo_clear(emp_material_t* material)
{
	emp_ubo_t* ubo = &material->ubo;
	ubo->count = 0;
}

void emp_gpu_instance_list_add(emp_gpu_instance_list_t* list, emp_gpu_instance_t const* value)
{
	assert(list->count < SDL_arraysize(list->entries));

	if (list->count < SDL_arraysize(list->entries)) {
		list->entries[list->count] = *value;
		list->count = list->count + 1;
	}
}

void emp_mesh_draw(emp_mesh_gpu_t const* mesh)
{
	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void*)0);
}

void emp_material_bind(emp_material_t const* material)
{
	glUseProgram(material->shader_program);

	for (size_t index = 0; index < material->ubo.count; index++) {
		emp_uniform_t const* uniform = material->ubo.uniforms + index;
		switch (uniform->type) {
		case emp_uniform_float:
			glUniform1f(uniform->location, *(GLfloat*)uniform->stable_data);
			break;
		case emp_uniform_matrix4x4:
			glUniformMatrix4fv(uniform->location, 1, GL_FALSE, (const float*)uniform->stable_data);
			break;
		case emp_uniform_int:
			glUniform1i(uniform->location, *(GLint*)uniform->stable_data);
			break;
		}
	}
}

void emp_gpu_instance_list_render(emp_material_t const* material, emp_gpu_instance_list_t const* instances)
{
	emp_material_bind(material);
	for (size_t index = 0; index < instances->count; index++) {
		emp_gpu_instance_t const* instance = instances->entries + index;
		emp_mesh_draw(instance->mesh);
	}
}

void emp_gl_clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void emp_gl_depth_test_enable(void)
{
	glEnable(GL_DEPTH_TEST);
}
void emp_gl_depth_test_disable(void)
{
	glDisable(GL_DEPTH_TEST);
}

GLuint compile_shader(GLenum type, const char* src)
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

#ifdef __EMSCRIPTEN__
#define SHADER_HEADER "#version 300 es\nprecision highp float;\n"
#else
#define SHADER_HEADER "#version 330 core\n"
#endif

int emp_gl(SDL_Window* window)
{
	static SDL_GLContext gl_context;

	if (window == NULL) {
		SDL_GL_DestroyContext(gl_context);
		return 0;
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

	gl_context = SDL_GL_CreateContext(window);

	if (!gl_context) {
		SDL_Log("Failed to create GL context: %s", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

#ifndef __EMSCRIPTEN__
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		SDL_Log("Failed to load OpenGL functions");
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
#endif
	return 0;
}

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

unsigned int create_shader_program_from_assets(emp_buffer vert_data, emp_buffer frag_data)
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

	glGenBuffers(1, &gpu_mesh->vbo);
	glGenBuffers(1, &gpu_mesh->ebo);

	glBindBuffer(GL_ARRAY_BUFFER, gpu_mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, unique_vertex_count * sizeof(emp_vertex_t), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu_mesh->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &gpu_mesh->vao);
	glBindVertexArray(gpu_mesh->vao);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, position));
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, normal));
	glEnableVertexAttribArray(1);

	// Texture coordinate attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, texcoord));
	glEnableVertexAttribArray(2);

	SDL_Log("Loaded mesh: %u unique vertices, %u indices (%.1f%% vertex reuse)",
		unique_vertex_count, index_count,
		(1.0f - (float)unique_vertex_count / index_count) * 100.0f);

	SDL_free(vertices);
	SDL_free(indices);
	fast_obj_destroy(obj);

	asset->handle = gpu_mesh;
}

void emp_mesh_unload_func(emp_asset_t* asset)
{
	if (!asset->handle)
		return;

	emp_mesh_gpu_t* gpu_mesh = (emp_mesh_gpu_t*)asset->handle;

	glDeleteBuffers(1, &gpu_mesh->vbo);
	glDeleteBuffers(1, &gpu_mesh->ebo);
	glDeleteVertexArrays(1, &gpu_mesh->vao);

	SDL_free(gpu_mesh);
	asset->handle = NULL;
}

emp_material_t emp_material_create(emp_buffer vert_data, emp_buffer frag_data)
{
	unsigned int program = create_shader_program_from_assets(vert_data, frag_data);
	return (emp_material_t) { .shader_program = program, .ubo = { 0 } };
}

void emp_material_destroy(emp_material_t* material)
{
	glDeleteProgram(material->shader_program);
}
