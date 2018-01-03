// flat normal mesh
struct Mesh
{
   v3i faces;
   float *vertices;
   i32 vcount;

   v3 *normals;
};

struct InstanceBuffers
{
   GLuint instanceVao;
   GLuint instanceMVPBuffer;
   GLuint instanceColorBuffer;
   GLuint instanceModelMatrixBuffer;
};

struct TextureInstanceBuffers
{
   GLuint instanceVao;
   GLuint instanceMVPBuffer;
};

struct OpenglState
{
   GLuint fbo;
   GLuint depthBuffer;

   GLuint backgroundProgram;
   GLuint fullScreenProgram;

   GLuint mainColorTexture;
   GLuint blockTex;
   GLuint guiTextureMap;

   GLuint buttonVbo;
   GLuint startButtonVbo;
   GLuint playButtonVbo;
   GLuint pauseButtonVbo;

   stbFont bitmapFont;

   InstanceBuffers instanceBuffers[3];
   TextureInstanceBuffers breakTextureInstanceBuffers;
};

struct MeshBuffers
{
   GLuint vbo;
   GLuint nbo;
   GLuint vao;
};

struct MeshObject
{
   Mesh mesh;
   MeshBuffers handles;
};

struct ProgramBase
{
   GLuint programHandle;
   GLuint vertexHandle;
   GLuint fragmentHandle;
};

struct TextProgram : ProgramBase
{
   GLint transformUniform;
   GLint texUniform;
   GLint vertexAttrib;
   GLint normalAttrib;
};

struct ShaderProgram : ProgramBase
{
   GLint modelUniform;
   GLint viewUniform;
   GLint lightPosUniform;
   GLint MVPUniform;
   GLint VUniform;
   GLint MUniform;
   GLint diffuseUniform;
   
   GLint vertexAttrib;
   GLint normalAttrib;

   GLint texUniform;
};
