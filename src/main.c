#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct { float m[16]; } emp_mat4_t;

static emp_mat4_t mat4_identity(void) {
    emp_mat4_t r = {0};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static emp_mat4_t mat4_mul(emp_mat4_t a, emp_mat4_t b) {
    emp_mat4_t r = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            for (int k = 0; k < 4; k++) {
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
        }
    }
    return r;
}

static emp_mat4_t mat4_make_rotation_x(float angle) {
    emp_mat4_t r = mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    r.m[5] = c;   r.m[9] = -s;
    r.m[6] = s;   r.m[10] = c;
    return r;
}

static emp_mat4_t mat4_make_rotation_y(float angle) {
    emp_mat4_t r = mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    r.m[0] = c;   r.m[8] = s;
    r.m[2] = -s;  r.m[10] = c;
    return r;
}

static emp_mat4_t mat4_make_perspective(float fov, float aspect, float nearZ, float farZ) {
    emp_mat4_t r = {0};
    float f = 1.0f / tanf(fov / 2.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (farZ + nearZ) / (nearZ - farZ);
    r.m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    r.m[11] = -1.0f;
    return r;
}

static emp_mat4_t mat4_make_translation(float x, float y, float z) {
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

static char* load_shader_file(const char* path) {
    size_t size = 0;
    void* data = SDL_LoadFile(path, &size);
    if (!data) {
        SDL_Log("Failed to load shader: %s", path);
        return NULL;
    }
    
    size_t header_len = strlen(SHADER_HEADER);
    char* result = (char*)SDL_malloc(header_len + size + 1);
    memcpy(result, SHADER_HEADER, header_len);
    memcpy(result + header_len, data, size);
    result[header_len + size] = '\0';
    
    SDL_free(data);
    return result;
}

typedef struct {
    float position[3];
    float color[3];
} emp_vertex_t;

typedef struct {
    emp_vertex_t* vertices;
    unsigned short* indices;
    int vertex_count;
    int index_count;
} emp_mesh_t;

static emp_mesh_t load_obj(const char* path) {
    emp_mesh_t mesh = {0};
    
    size_t size = 0;
    char* data = (char*)SDL_LoadFile(path, &size);
    if (!data) {
        SDL_Log("Failed to load OBJ: %s", path);
        return mesh;
    }
    
    int pos_count = 0, color_count = 0, face_count = 0;
    char* line = data;
    while (*line) {
        if (line[0] == 'v' && line[1] == ' ') pos_count++;
        else if (line[0] == 'v' && line[1] == 'c') color_count++;
        else if (line[0] == 'f' && line[1] == ' ') face_count++;
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    
    float* positions = (float*)SDL_malloc(pos_count * 3 * sizeof(float));
    float* colors = (float*)SDL_malloc(color_count * 3 * sizeof(float));
    
    mesh.vertices = (emp_vertex_t*)SDL_malloc(face_count * 3 * sizeof(emp_vertex_t));
    mesh.indices = (unsigned short*)SDL_malloc(face_count * 3 * sizeof(unsigned short));
    
    int pi = 0, ci = 0;
    line = data;
    while (*line) {
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf(line, "v %f %f %f", &positions[pi*3], &positions[pi*3+1], &positions[pi*3+2]);
            pi++;
        } else if (line[0] == 'v' && line[1] == 'c') {
            sscanf(line, "vc %f %f %f", &colors[ci*3], &colors[ci*3+1], &colors[ci*3+2]);
            ci++;
        }
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    
    int vi = 0;
    line = data;
    while (*line) {
        if (line[0] == 'f' && line[1] == ' ') {
            int p1, c1, p2, c2, p3, c3;
            sscanf(line, "f %d/%d %d/%d %d/%d", &p1, &c1, &p2, &c2, &p3, &c3);
            p1--; c1--; p2--; c2--; p3--; c3--;
            
            mesh.vertices[vi].position[0] = positions[p1*3+0];
            mesh.vertices[vi].position[1] = positions[p1*3+1];
            mesh.vertices[vi].position[2] = positions[p1*3+2];
            mesh.vertices[vi].color[0] = colors[c1*3+0];
            mesh.vertices[vi].color[1] = colors[c1*3+1];
            mesh.vertices[vi].color[2] = colors[c1*3+2];
            mesh.indices[vi] = (unsigned short)vi;
            vi++;
            
            mesh.vertices[vi].position[0] = positions[p2*3+0];
            mesh.vertices[vi].position[1] = positions[p2*3+1];
            mesh.vertices[vi].position[2] = positions[p2*3+2];
            mesh.vertices[vi].color[0] = colors[c2*3+0];
            mesh.vertices[vi].color[1] = colors[c2*3+1];
            mesh.vertices[vi].color[2] = colors[c2*3+2];
            mesh.indices[vi] = (unsigned short)vi;
            vi++;
            
            mesh.vertices[vi].position[0] = positions[p3*3+0];
            mesh.vertices[vi].position[1] = positions[p3*3+1];
            mesh.vertices[vi].position[2] = positions[p3*3+2];
            mesh.vertices[vi].color[0] = colors[c3*3+0];
            mesh.vertices[vi].color[1] = colors[c3*3+1];
            mesh.vertices[vi].color[2] = colors[c3*3+2];
            mesh.indices[vi] = (unsigned short)vi;
            vi++;
        }
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }
    
    mesh.vertex_count = vi;
    mesh.index_count = vi;
    
    SDL_free(positions);
    SDL_free(colors);
    SDL_free(data);
    return mesh;
}

static void free_mesh(emp_mesh_t* mesh) {
    SDL_free(mesh->vertices);
    SDL_free(mesh->indices);
    mesh->vertices = NULL;
    mesh->indices = NULL;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
}

static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static GLuint g_shader_program = 0;
static GLuint g_vao = 0;
static GLuint g_vbo = 0;
static GLuint g_ebo = 0;
static GLint g_mvp_loc = -1;
static float g_angle_x = 0.0f;
static float g_angle_y = 0.0f;
static Uint64 g_last_time = 0;
static bool g_running = true;
static int g_index_count = 0;

static GLuint compile_shader(GLenum type, const char* src) {
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

static GLuint create_shader_program(const char* vert_path, const char* frag_path) {
    char* vert_src = load_shader_file(vert_path);
    char* frag_src = load_shader_file(frag_path);
    if (!vert_src || !frag_src) {
        SDL_free(vert_src);
        SDL_free(frag_src);
        return 0;
    }

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    SDL_free(vert_src);
    SDL_free(frag_src);
    
    if (!vs || !fs) return 0;

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

static void main_loop(void) {
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

    g_angle_x += 0.5f * delta_time;
    g_angle_y += 0.8f * delta_time;

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_shader_program);

    emp_mat4_t model = mat4_mul(mat4_make_rotation_x(g_angle_x), mat4_make_rotation_y(g_angle_y));
    emp_mat4_t view = mat4_make_translation(0.0f, 0.0f, -3.0f);
    emp_mat4_t proj = mat4_make_perspective((float)(M_PI / 4.0), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
    emp_mat4_t mvp = mat4_mul(proj, mat4_mul(view, model));

    glUniformMatrix4fv(g_mvp_loc, 1, GL_FALSE, mvp.m);

    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, g_index_count, GL_UNSIGNED_SHORT, 0);

    SDL_GL_SwapWindow(g_window);
}

int main(int argc, char* argv[]) {
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

    g_window = SDL_CreateWindow("Empire - OpenGL Cube", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
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

    g_shader_program = create_shader_program("src/shaders/cube.vert", "src/shaders/cube.frag");
    if (!g_shader_program) {
        SDL_Log("Failed to create shader program");
        return 1;
    }
    g_mvp_loc = glGetUniformLocation(g_shader_program, "u_mvp");

    emp_mesh_t cube = load_obj("src/models/cube.obj");
    if (!cube.vertices) {
        SDL_Log("Failed to load cube mesh");
        return 1;
    }
    g_index_count = cube.index_count;

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glGenBuffers(1, &g_ebo);

    glBindVertexArray(g_vao);

    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, cube.vertex_count * sizeof(emp_vertex_t), cube.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube.index_count * sizeof(unsigned short), cube.indices, GL_STATIC_DRAW);

    free_mesh(&cube);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(emp_vertex_t), (void*)offsetof(emp_vertex_t, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    g_last_time = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (g_running) {
        main_loop();
        SDL_Delay(16);
    }
#endif

    glDeleteVertexArrays(1, &g_vao);
    glDeleteBuffers(1, &g_vbo);
    glDeleteBuffers(1, &g_ebo);
    glDeleteProgram(g_shader_program);

    SDL_GL_DestroyContext(g_gl_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return 0;
}
