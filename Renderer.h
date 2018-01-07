
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
   DrawGUI,
   DrawLinearInstances,
   DrawBranchInstances,
   DrawBreakInstances,
   DrawBreakTextureInstances
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

struct DrawGUICommand : public CommandBase
{
   v2 position;
   v2 scale;
   GLuint texture;
   GLuint uvs;
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

struct TextureInstance
{
   v3 position;
   quat orientation;
   v3 scale;
};

struct DrawSpeedupCommand : public CommandBase
{
   Object obj;
};

struct DrawBranchCommand : public CommandBase
{
   Object obj;
   v3 color;
};

struct DrawBreakCommand : public CommandBase
{
   Object obj;
};

struct DrawTextCommand : public CommandBase
{
   u32 numChars;
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
   GLuint uvs;
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

struct DrawBreakTextureInstancesCommand : public CommandBase
{
};

/*
Different commands we need.
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

   u32 breakTextureInstanceCount;
   TextureInstance *breakTextureInstances;

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
   inline void PushBreakTextureInstance(v3 position, v3 scale, quat orientation);
   inline void PushRenderBreakTextureInstances(StackAllocator *allocator);
   void RenderTrackInstances(StackAllocator *allocator, v3 lightPos, m4 &view, u32 count, TrackInstance *instances, u32 vcount,
			     InstanceBuffers buffers);
   void RenderBreakTextureInstances(StackAllocator *allocator, m4 &view,
				    u32 instanceCount, TextureInstance *instance,
				    GLuint texture, TextureInstanceBuffers buffers,
				    v3 color);
   inline void PushDrawSpeedup(Object obj, StackAllocator *allocator);
   inline void PushDrawBranch(Object obj, v3 color, StackAllocator *allocator);
   inline void PushDrawLockedBranch(Object obj, v3 color, MeshObject *buffers, StackAllocator *allocator);
   inline void PushDrawBreak(Object obj, StackAllocator *allocator);
   inline void PushRenderBlur(StackAllocator *allocator);
   inline void PushRenderText(char *text, u32 numChars, v2 position, v2 scale, v3 color, StackAllocator *allocator);
   inline void PushDrawGUI(v2 position, v2 scale, GLuint texture, GLuint uvs, StackAllocator *allocator);
   void ExecuteCommands(Camera &camera, v3 lightPos, stbFont &font, TextProgram &p, RenderState &renderer, StackAllocator *allocator, OpenglState &glState);
   inline void Clean(StackAllocator *allocator);
};

struct WorldPallette
{
   v3 closec;
   v3 farc;
   v3 trackc;
   v3 playerc;
   v3 blockc;
   v3 textc;
};

static WorldPallette colorTable[5];

struct RenderState
{
   float worldColorT;
   WorldPallette lastColors;
   WorldPallette currentColors;
   WorldPallette targetColors;
   CommandState commands;
};

// reduced frustum for track culling
// since every track is placed on the z plane,
// we only need the left and right planes
struct TrackFrustum
{
   v4 left, right;
};
