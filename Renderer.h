
struct Mesh
{
   v3i faces;
   float *vertices;
   i32 vcount;

   v3 *normals;
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

struct TextProgram
{
   GLuint programHandle;
   GLuint vertexHandle;
   GLuint fragmentHandle;

   GLint transformUniform;
   GLint texUniform;
   GLint vertexAttrib;
   GLint normalAttrib;   
};

enum RenderCommand
{
   BindProgram,
   DrawMesh,
   DrawBreakTexture,
   DrawLinear,
   DrawSpeedup,
   DrawBranch,
   DrawBreak,
   DrawString, //DrawText is taken :(
   DrawBlur,
};

struct CommandBase
{
   RenderCommand command;
   CommandBase *next;
};

struct BindProgramCommand : public CommandBase
{
   GLuint program;   
};

struct DrawMeshCommand : public CommandBase
{
   MeshObject mesh;
   v3 position;
   v3 scale;
   quat orientation;
};

struct DrawBreakTextureCommand : public CommandBase
{
   v3 position;
   v3 scale;
   quat orientation;
};

// lol
struct DrawLinearCommand : public CommandBase
{
   Object obj;
};

struct DrawSpeedupCommand : public CommandBase
{
   Object obj;
};

struct DrawBranchCommand : public CommandBase
{
   Object obj;
};

struct DrawBreakCommand : public CommandBase
{
   Object obj;
};

struct DrawTextCommand : public CommandBase
{
   u32 textSize;
   v2 position;
   v2 scale;
   v3 color;

   inline char *GetString()
   {
      return (char *)(this) + sizeof(DrawTextCommand);
   }
};

/*
Different command we need.
BindProgram
Draw Mesh
Draw Bright break thing
 */

struct RenderState;
struct Camera;

struct CommandState
{
   GLuint currentProgram;
   u32 count;

   CommandBase *first;
   CommandBase *last;
   void PushBindProgram(GLuint program, StackAllocator *allocator);
   void PushDrawMesh(MeshObject mesh, v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   void PushDrawBreakTexture(v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   void PushDrawLinear(Object obj, StackAllocator *allocator);
   void PushDrawSpeedup(Object obj, StackAllocator *allocator);
   void PushDrawBranch(Object obj, StackAllocator *allocator);
   void PushDrawBreak(Object obj, StackAllocator *allocator);
   void PushRenderBlur(StackAllocator *allocator);
   void PushRenderText(char *text, u32 textSize, v2 position, v2 scale, v3 color, StackAllocator *allocator);
   void ExecuteCommands(Camera &camera, v3 lightPos, stbFont &font, TextProgram &p, RenderState &renderer);
   void Clean(StackAllocator *allocator);
};

struct RenderState
{
   GLuint fbo;
   GLuint mainColorTexture;
   GLuint blurTexture;
   GLuint normalTexture;
   GLuint depthBuffer;

   GLuint fullScreenProgram;
   GLuint blurProgram;

   GLuint horizontalFbo;
   GLuint horizontalColorBuffer;

   GLuint verticalFbo;
   GLuint verticalColorBuffer;

   CommandState commands;
};

// reduced frustum for track culling
// since every track is placed on the z plane,
// we only need the left and right planes
struct TrackFrustum
{
   v4 left, right;
};
