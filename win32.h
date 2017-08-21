#include <stddef.h>

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_ARB_framebuffer_sRGB 0x20A9
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0X2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042


#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30

#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8

#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D

#define GL_TEXTURE_WRAP_R 0x8072

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3

#define GL_MULTISAMPLE 0x809D

#define GL_FRAMEBUFFER 0x8D40
#define GL_RGBA16F 0x881A

#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2

#define GL_FRAMEBUFFER_COMPLETE 0x8CD5

#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER 0x8CA8

#define GL_RENDERBUFFER 0x8D41
#define GL_DEPTH24_STENCIL8 0x88F0

#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A

#define GL_FRAMEBUFFER_SRGB 0x8DB9

typedef uint32_t GLenum;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef uint32_t GLuint;
typedef char GLchar;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef int GLsizei;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef BOOL (*wglChoosePixelFormatARB_t)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef GLuint (*glCreateProgram_t)(void);
typedef GLuint (*glCreateShader_t)(GLuint);
typedef void (*glShaderSource_t)(GLuint, GLsizei, const GLchar *const*, const GLint *);
typedef void (*glAttachShader_t)(GLuint, GLuint);
typedef GLint (*glGetAttribLocation_t)(GLuint, const GLchar *);
typedef GLint (*glGetUniformLocation_t)(GLuint, const GLchar *);
typedef void (*glLinkProgram_t)(GLuint);
typedef void (*glUseProgram_t)(GLuint);
typedef void (*glVertexAttrib4fv_t)(GLuint, const GLfloat *);
typedef void (*glVertexAttribPointer_t)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void (*glCompileShader_t)(GLuint);
typedef void (*glGenVertexArrays_t)(GLsizei, GLuint *);
typedef void (*glBindVertexArray_t)(GLuint);
typedef void (*glGenBuffers_t)(GLsizei, GLuint *);
typedef void (*glBindBuffer_t)(GLenum, GLuint);
typedef void (*glBufferData_t)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void (*glEnableVertexAttribArray_t)(GLuint);
typedef void (*glUniform2ui_t)(GLint, GLuint, GLuint);
typedef void (*glUniform2f_t)(GLint, GLfloat, GLfloat);
typedef void (*glUniform3fv_t)(GLint, GLsizei, const GLfloat *);
typedef void (*glUniformMatrix2fv_t)(GLint, GLsizei, GLboolean, GLfloat *);
typedef void (*glUniformMatrix3fv_t)(GLint, GLsizei, GLboolean, GLfloat *);
typedef void (*glUniformMatrix4fv_t)(GLint, GLsizei, GLboolean, GLfloat *);
typedef void (*glUniform1i_t)(GLint, GLint);
typedef void (*glActiveTexture_t)(GLenum);
typedef void (*glUniform1f_t)(GLint, GLfloat);
typedef void (*glBufferSubData_t) (GLenum, GLintptr, GLsizeiptr, const void *);
typedef void (*glDeleteBuffers_t) (GLsizei, const GLuint *);
typedef void (*glGenFramebuffers_t) (GLsizei n, GLuint *);
typedef void (*glBindFramebuffer_t) (GLenum, GLuint);
typedef void (*glFramebufferTexture2D_t) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (*glCheckFramebufferStatus_t) (GLenum);
typedef void (*glBlitFramebuffer_t) (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef void (*glGenRenderbuffers_t) (GLsizei, GLuint *);
typedef void (*glBindRenderbuffer_t) (GLenum, GLuint);
typedef void (*glRenderbufferStorage_t) (GLenum, GLenum, GLsizei, GLsizei); 
typedef void (*glFramebufferRenderbuffer_t) (GLenum, GLenum, GLenum, GLuint);
typedef void (*glDrawBuffers_t) (GLsizei, const GLenum *);

wglChoosePixelFormatARB_t wglChoosePixelFormatARB;
glCreateProgram_t glCreateProgram;
glCreateShader_t glCreateShader;
glShaderSource_t glShaderSource;
glAttachShader_t glAttachShader;
glGetAttribLocation_t glGetAttribLocation;
glGetUniformLocation_t glGetUniformLocation;
glLinkProgram_t glLinkProgram;
glUseProgram_t glUseProgram;
glVertexAttrib4fv_t glVertexAttrib4fv;
glVertexAttribPointer_t glVertexAttribPointer;
glCompileShader_t glCompileShader;
glGenVertexArrays_t glGenVertexArrays;
glBindVertexArray_t glBindVertexArray;
glGenBuffers_t glGenBuffers;
glBindBuffer_t glBindBuffer;
glBufferData_t glBufferData;
glEnableVertexAttribArray_t glEnableVertexAttribArray;
glUniform2ui_t glUniform2ui;
glUniform2f_t glUniform2f;
glUniform3fv_t glUniform3fv;
glUniformMatrix2fv_t glUniformMatrix2fv;
glUniformMatrix3fv_t glUniformMatrix3fv;
glUniformMatrix4fv_t glUniformMatrix4fv;
glUniform1i_t glUniform1i;
glActiveTexture_t glActiveTexture;
glUniform1f_t glUniform1f;
glBufferSubData_t glBufferSubData;
glDeleteBuffers_t glDeleteBuffers;
glGenFramebuffers_t glGenFramebuffers;
glBindFramebuffer_t glBindFramebuffer;
glFramebufferTexture2D_t glFramebufferTexture2D;
glCheckFramebufferStatus_t glCheckFramebufferStatus;
glBlitFramebuffer_t glBlitFramebuffer;
glGenRenderbuffers_t glGenRenderbuffers;
glBindRenderbuffer_t glBindRenderbuffer;
glRenderbufferStorage_t glRenderbufferStorage;
glFramebufferRenderbuffer_t glFramebufferRenderbuffer;
glDrawBuffers_t glDrawBuffers;

HMODULE glDll = 0;

#define WinGetGlExtension(name)			\
   name = (name##_t)GetGlExtension(#name);	\
   if(!name)					\
   {						\
      ErrorDialogue("name not found");		\
   }						


static size_t WinFileSize(char *);
static b32 WinReadFile(char *, u8 *, size_t);
static u8 *Win32AllocateMemory(size_t, size_t*);
static b32 Win32FreeMemory(u8 *);
