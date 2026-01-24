/**
 * Glad - OpenGL ES 3.0 Loader
 * Generated for WebGL 2.0 / OpenGL ES 3.0 compatibility
 */
#ifndef GLAD_GL_H_
#define GLAD_GL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GLAD_API_CALL extern

#include <stddef.h>

typedef void (*GLADloadfunc)(const char *name);
typedef void* (*GLADloadproc)(const char *name);

int gladLoadGL(GLADloadfunc load);

/* OpenGL ES 3.0 types */
typedef void GLvoid;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef char GLchar;
typedef short GLshort;
typedef signed char GLbyte;
typedef unsigned short GLushort;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

/* OpenGL ES 3.0 constants */
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NONE 0
#define GL_NO_ERROR 0

#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000

#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006

#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44

#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0

#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_NEAREST 0x2600
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901

#define GL_RGBA 0x1908
#define GL_RGB 0x1907

/* OpenGL ES 3.0 function pointers */
typedef void (*PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGLCLEARPROC)(GLbitfield mask);
typedef void (*PFNGLENABLEPROC)(GLenum cap);
typedef void (*PFNGLDISABLEPROC)(GLenum cap);
typedef void (*PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef const GLubyte* (*PFNGLGETSTRINGPROC)(GLenum name);
typedef void (*PFNGLBLENDFUNCPROC)(GLenum sfactor, GLenum dfactor);

typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const* string, const GLint* length);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (*PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (*PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (*PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);

typedef void (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);

typedef void (*PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (*PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const void* indices);

typedef void (*PFNGLGENTEXTURESPROC)(GLsizei n, GLuint* textures);
typedef void (*PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint* textures);
typedef void (*PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
typedef void (*PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);

typedef void (*PFNGLGETINTEGERVPROC)(GLenum pname, GLint* data);
typedef GLenum (*PFNGLGETERRORPROC)(void);

/* Global function pointers */
GLAD_API_CALL PFNGLCLEARCOLORPROC glad_glClearColor;
GLAD_API_CALL PFNGLCLEARPROC glad_glClear;
GLAD_API_CALL PFNGLENABLEPROC glad_glEnable;
GLAD_API_CALL PFNGLDISABLEPROC glad_glDisable;
GLAD_API_CALL PFNGLVIEWPORTPROC glad_glViewport;
GLAD_API_CALL PFNGLGETSTRINGPROC glad_glGetString;
GLAD_API_CALL PFNGLBLENDFUNCPROC glad_glBlendFunc;
GLAD_API_CALL PFNGLGETERRORPROC glad_glGetError;

GLAD_API_CALL PFNGLCREATESHADERPROC glad_glCreateShader;
GLAD_API_CALL PFNGLDELETESHADERPROC glad_glDeleteShader;
GLAD_API_CALL PFNGLSHADERSOURCEPROC glad_glShaderSource;
GLAD_API_CALL PFNGLCOMPILESHADERPROC glad_glCompileShader;
GLAD_API_CALL PFNGLGETSHADERIVPROC glad_glGetShaderiv;
GLAD_API_CALL PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog;

GLAD_API_CALL PFNGLCREATEPROGRAMPROC glad_glCreateProgram;
GLAD_API_CALL PFNGLDELETEPROGRAMPROC glad_glDeleteProgram;
GLAD_API_CALL PFNGLATTACHSHADERPROC glad_glAttachShader;
GLAD_API_CALL PFNGLLINKPROGRAMPROC glad_glLinkProgram;
GLAD_API_CALL PFNGLUSEPROGRAMPROC glad_glUseProgram;
GLAD_API_CALL PFNGLGETPROGRAMIVPROC glad_glGetProgramiv;
GLAD_API_CALL PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog;

GLAD_API_CALL PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation;
GLAD_API_CALL PFNGLUNIFORM1IPROC glad_glUniform1i;
GLAD_API_CALL PFNGLUNIFORM1FPROC glad_glUniform1f;
GLAD_API_CALL PFNGLUNIFORM2FPROC glad_glUniform2f;
GLAD_API_CALL PFNGLUNIFORM3FPROC glad_glUniform3f;
GLAD_API_CALL PFNGLUNIFORM4FPROC glad_glUniform4f;
GLAD_API_CALL PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv;

GLAD_API_CALL PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays;
GLAD_API_CALL PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays;
GLAD_API_CALL PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray;

GLAD_API_CALL PFNGLGENBUFFERSPROC glad_glGenBuffers;
GLAD_API_CALL PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers;
GLAD_API_CALL PFNGLBINDBUFFERPROC glad_glBindBuffer;
GLAD_API_CALL PFNGLBUFFERDATAPROC glad_glBufferData;

GLAD_API_CALL PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer;
GLAD_API_CALL PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
GLAD_API_CALL PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray;

GLAD_API_CALL PFNGLDRAWARRAYSPROC glad_glDrawArrays;
GLAD_API_CALL PFNGLDRAWELEMENTSPROC glad_glDrawElements;

GLAD_API_CALL PFNGLGENTEXTURESPROC glad_glGenTextures;
GLAD_API_CALL PFNGLDELETETEXTURESPROC glad_glDeleteTextures;
GLAD_API_CALL PFNGLBINDTEXTUREPROC glad_glBindTexture;
GLAD_API_CALL PFNGLTEXIMAGE2DPROC glad_glTexImage2D;
GLAD_API_CALL PFNGLTEXPARAMETERIPROC glad_glTexParameteri;
GLAD_API_CALL PFNGLACTIVETEXTUREPROC glad_glActiveTexture;

GLAD_API_CALL PFNGLGETINTEGERVPROC glad_glGetIntegerv;

/* Define function macros */
#define glClearColor glad_glClearColor
#define glClear glad_glClear
#define glEnable glad_glEnable
#define glDisable glad_glDisable
#define glViewport glad_glViewport
#define glGetString glad_glGetString
#define glBlendFunc glad_glBlendFunc

#define glCreateShader glad_glCreateShader
#define glDeleteShader glad_glDeleteShader
#define glShaderSource glad_glShaderSource
#define glCompileShader glad_glCompileShader
#define glGetShaderiv glad_glGetShaderiv
#define glGetShaderInfoLog glad_glGetShaderInfoLog

#define glCreateProgram glad_glCreateProgram
#define glDeleteProgram glad_glDeleteProgram
#define glAttachShader glad_glAttachShader
#define glLinkProgram glad_glLinkProgram
#define glUseProgram glad_glUseProgram
#define glGetProgramiv glad_glGetProgramiv
#define glGetProgramInfoLog glad_glGetProgramInfoLog

#define glGetUniformLocation glad_glGetUniformLocation
#define glUniform1i glad_glUniform1i
#define glUniform1f glad_glUniform1f
#define glUniform2f glad_glUniform2f
#define glUniform3f glad_glUniform3f
#define glUniform4f glad_glUniform4f
#define glUniformMatrix4fv glad_glUniformMatrix4fv

#define glGenVertexArrays glad_glGenVertexArrays
#define glDeleteVertexArrays glad_glDeleteVertexArrays
#define glBindVertexArray glad_glBindVertexArray

#define glGenBuffers glad_glGenBuffers
#define glDeleteBuffers glad_glDeleteBuffers
#define glBindBuffer glad_glBindBuffer
#define glBufferData glad_glBufferData

#define glVertexAttribPointer glad_glVertexAttribPointer
#define glEnableVertexAttribArray glad_glEnableVertexAttribArray
#define glDisableVertexAttribArray glad_glDisableVertexAttribArray

#define glDrawArrays glad_glDrawArrays
#define glDrawElements glad_glDrawElements

#define glGenTextures glad_glGenTextures
#define glDeleteTextures glad_glDeleteTextures
#define glBindTexture glad_glBindTexture
#define glTexImage2D glad_glTexImage2D
#define glTexParameteri glad_glTexParameteri
#define glActiveTexture glad_glActiveTexture

#define glGetIntegerv glad_glGetIntegerv
#define glGetError glad_glGetError

#ifdef __cplusplus
}
#endif

#endif /* GLAD_GL_H_ */
