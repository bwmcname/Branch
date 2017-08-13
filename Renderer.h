
struct Camera
{
   enum
   {
      lerping,
   };

   quat orientation;
   v3 position;

   u32 flags;
   float t;

   m4 view;
};

struct Mesh
{
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

enum RenderCommand
{
   BindProgram,
   DrawMesh,
   DrawBreakTexture,
   DrawLinear,
   DrawBranch,
   DrawBreak,
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

struct DrawLinearCommand : public CommandBase
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

/*
Different command we need.
BindProgram
Draw Mesh
Draw Bright break thing
 */

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
   void PushDrawBranch(Object obj, StackAllocator *allocator);
   void PushDrawBreak(Object obj, StackAllocator *allocator);
   void ExecuteCommands(Camera &camera, v3 lightPos);
   void Clean(StackAllocator *allocator);
};

struct RenderState
{
   GLuint fbo;
   GLuint mainColorTexture;
   GLuint blurTexture;
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
