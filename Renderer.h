
// flat normal mesh
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

enum RenderCommand
{
   BindProgram,
   DrawMesh,
   DrawBreakTexture,
   DrawLinear,
   DrawSpeedup,
   DrawBranch,
   DrawLockedBranch,
   DrawBreak,
   DrawString, //DrawText is taken :(
   DrawBlur,
   DrawButton,
   DrawLinearInstances,
   DrawBranchInstances,
   DrawBreakInstances,
};

struct CommandBase
{
   RenderCommand command;
   CommandBase *next;
};

struct BindProgramCommand : public CommandBase
{
   ProgramBase *program;   
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

// trying to replace with instanced renderer
struct DrawLinearCommand : public CommandBase
{
   Object obj;
};

struct TrackInstance
{
   v3 position;
   quat rotation;
   v3 scale;
   v3 color;
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

struct DrawButtonCommand : public CommandBase
{
   v2 position;
   v2 scale;
   GLuint texture;
};

struct DrawLockedBranchCommand : public DrawBranchCommand
{
   MeshObject *mesh;
};

struct DrawLinearInstancesCommand : public CommandBase
{
};

struct DrawBranchInstancesCommand : public CommandBase
{
};

struct DrawBreakInstancesCommand : public CommandBase
{
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
   ProgramBase *currentProgram;
   u32 count;

   u32 linearInstanceCount;
   TrackInstance *linearInstances;

   u32 branchInstanceCount;
   TrackInstance *branchInstances;

   u32 breakInstanceCount;
   TrackInstance *breakInstances;
   
   GLuint instanceMVPBuffer;
   GLuint instanceColorBuffer;
   GLuint instanceModelMatrixBuffer;
   GLuint linearInstanceVao;
   GLuint branchInstanceVao;
   GLuint breakInstanceVao;

   CommandBase *first;
   CommandBase *last;
   inline void PushBindProgram(ProgramBase *base, StackAllocator *allocator);
   inline void PushDrawMesh(MeshObject mesh, v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   inline void PushDrawBreakTexture(v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   inline void PushDrawLinear(Object obj, StackAllocator *allocator);
   inline void PushLinearInstance(Object obj, v3 color);
   inline void PushBranchInstance(Object obj, v3 color);
   inline void PushBreakInstance(Object obj, v3 color);
   inline void PushRenderLinearInstances(StackAllocator *allocator);
   inline void PushRenderBranchInstances(StackAllocator *allocator);
   inline void PushRenderBreakInstances(StackAllocator *allocator);
   void RenderTrackInstances(StackAllocator *allocator, v3 lightPos, m4 &view, u32 count, TrackInstance *instances, u32 vcount, GLuint instanceVao);
   inline void PushDrawSpeedup(Object obj, StackAllocator *allocator);
   inline void PushDrawBranch(Object obj, StackAllocator *allocator);
   inline void PushDrawLockedBranch(Object obj, MeshObject *buffers, StackAllocator *allocator);
   inline void PushDrawBreak(Object obj, StackAllocator *allocator);
   inline void PushRenderBlur(StackAllocator *allocator);
   inline void PushRenderText(char *text, u32 textSize, v2 position, v2 scale, v3 color, StackAllocator *allocator);
   inline void PushDrawButton(v2 position, v2 scale, GLuint texture, StackAllocator *allocator);
   void ExecuteCommands(Camera &camera, v3 lightPos, stbFont &font, TextProgram &p, RenderState &renderer, StackAllocator *allocator);
   inline void Clean(StackAllocator *allocator);
};

struct RenderState
{
   GLuint fbo;
   GLuint mainColorTexture;
   GLuint normalTexture;
   GLuint blurTexture;
   GLuint depthBuffer;
   GLuint buttonVbo;

   GLuint fullScreenProgram;
   GLuint blurProgram;
   GLuint outlineProgram;

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
