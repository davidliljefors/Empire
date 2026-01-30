#include "Empire/emp_gl.h"
#include "Empire/types.h"
#include "glad/gl.h"
#include <SDL3/SDL.h>

void emp_ubo_set_push(emp_ubo_t* ubo, emp_uniform_type type, int location, void* stable_data)
{
	emp_uniform_t* uniform = ubo->uniforms + ubo->count;
	uniform->type = type;
	uniform->location = location;
	uniform->stable_data = stable_data;
	ubo->count = ubo->count + 1;
}

void emp_ubo_set_clear(emp_ubo_t* ubo)
{
	ubo->count = 0;
}

void emp_gpu_instance_list_add(emp_gpu_instance_list_t* list, emp_gpu_instance_t const* value)
{
	SDL_assert(list->count < SDL_arraysize(list->entries));

	list->entries[list->count] = *value;
	list->count = list->count + 1;
}

static void emp_mesh_draw(emp_mesh_gpu_t const* mesh)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
}

static void emp_material_bind(emp_material_t const* material)
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

static void render_test(emp_material_t const* material, emp_gpu_instance_list_t const* instances)
{
	emp_material_bind(material);
	for (size_t index = 0; index < instances->count; index++) {
		emp_gpu_instance_t const* instance = instances->entries + index;
		emp_mesh_draw(&instance->mesh);
	}
}
