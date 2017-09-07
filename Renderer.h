
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
   DrawBreak,
   DrawString, //DrawText is taken :(
   DrawBlur,
   DrawButton,
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

struct LinearInstance
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
   LinearInstance *linearInstances;
   GLuint instanceMVPBuffer;
   GLuint instanceColorBuffer;
   GLuint instanceModelMatrixBuffer;
   GLuint instanceVao;

   CommandBase *first;
   CommandBase *last;
   void PushBindProgram(ProgramBase *base, StackAllocator *allocator);
   void PushDrawMesh(MeshObject mesh, v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   void PushDrawBreakTexture(v3 position, v3 scale, quat orientation, StackAllocator *allocator);
   void PushDrawLinear(Object obj, StackAllocator *allocator);
   inline void PushLinearInstance(Object obj, v3 color);
   void RenderLinearInstances(StackAllocator *allocator, v3 lightPos, m4 &view);
   void PushDrawSpeedup(Object obj, StackAllocator *allocator);
   void PushDrawBranch(Object obj, StackAllocator *allocator);
   void PushDrawBreak(Object obj, StackAllocator *allocator);
   void PushRenderBlur(StackAllocator *allocator);
   void PushRenderText(char *text, u32 textSize, v2 position, v2 scale, v3 color, StackAllocator *allocator);
   void PushDrawButton(v2 position, v2 scale, GLuint texture, StackAllocator *allocator);
   void ExecuteCommands(Camera &camera, v3 lightPos, stbFont &font, TextProgram &p, RenderState &renderer, StackAllocator *allocator);
   void Clean(StackAllocator *allocator);
};

struct RenderState
{
   GLuint fbo;
   GLuint mainColorTexture;
   GLuint blurTexture;
   GLuint depthBuffer;
   GLuint buttonVbo;

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
