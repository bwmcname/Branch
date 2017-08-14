#define TRACK_SEGMENT_SIZE 12.0f
static float delta;

struct ShaderProgram
{
   GLuint programHandle;
   GLuint vertexHandle;
   GLuint fragmentHandle;

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

union tri2
{
   v2 e[3];

   struct
   {
      v2 a, b, c;
   };
};

// cubic bezier control points
union Curve
{   
   struct
   {
      v2 p1, p2, p3, p4;
   };

   v2 e[4];
 
   v4 lerpables[2];
};

enum KeyState
{
   down,
   up,
};

struct Track
{
   enum
   {
      staticMesh = 0x1,
      branch = 0x2,
      left = 0x8, // else, right
      breaks = 0x10,
   };

   Object renderable;
   Curve *bezier;
   i32 flags;
};

template <typename T>
struct CircularQueue
{
   i32 max;
   i32 size;
   i32 begin;
   i32 end;
   T *elements;

   //CircularQueue(i32 _max, StackAllocator *allocator);
   void Push(T e);
   T Pop();
   void ClearToZero();

   inline i32 IncrementIndex(i32 index);
   
private:
   inline void IncrementEnd();
   inline void IncrementBegin();
};

// an "order" for a track to be placed
// used in the queue when creating the graph
struct TrackOrder
{
   u32 index;
   u32 ancestor;
   i32 x, y;

   enum
   {
      dontBranch = 0x1,     
   };
   i32 rules;
};

struct Attribute
{
   enum
   {
      hasLeftEdge = 0x1,
      hasRightEdge = 0x2,
      reachable = 0x4,
      invisible = 0x8,
      unused = 0x10,
      breaks = 0x20,
      branch = 0x40,
      linear = 0x80,
   };

   u16 id;
   u16 flags;
   u16 edgeCount;
   u16 ancestorCount;

   u16 edges[2];
   u16 ancestors[3];

   inline u16 leftEdge()
   {
      return edges[0];
   }

   inline u16 rightEdge()
   {
      return edges[1];
   }

   inline u16 hasLeft()
   {
      return flags & hasLeftEdge;
   }

   inline u16 hasRight()
   {
      return flags & hasRightEdge;
   }

   inline i32 isBranch()
   {
      return flags & branch; 
   }

   inline void addAncestor(u16 ancestorID);
   inline void removeAncestor(u16 ancestorID);
   inline void removeEdge(u16 edgeID);
};

struct NewTrackOrder
{
   enum
   {
      left = 0x1,
      right = 0x2,
      sideMask = 0x3,
      dontBranch = 0x4,
      dontBreak = 0x8,
   };

   u16 ancestorID;
   u16 flags;

   i32 x;
   i32 y;

   inline u16 Side()
   {
      return (flags & sideMask) - 1;
   }
};

struct NewTrackGraph
{
   enum
   {
      left = 0x1,
      switching = 0x2,
   };   

   static const u32 capacity = 1024;
   Attribute *adjList;
   Track *elements;
   CircularQueue<u16> availableIDs;
   CircularQueue<NewTrackOrder> orders;
   VirtualCoordHashTable taken;

   u16 *IDtable;
   u16 *reverseIDtable;
   u8 flags;

   float switchDelta;
   Curve beginLerp;
   Curve endLerp;

   void RemoveTrackActual(u16 id);
   inline Attribute &GetTrack(u16 id);
   inline void Swap2(u16 a, u16 b);
   inline void Move(u16 src, u16 dst);
   inline void SetID(u16 virt, u16 actual);
   inline u16 GetActualID(u16 virt);
   inline u16 GetVirtualID(u16 actual);

#ifdef DEBUG
   void VerifyGraph();
   void VerifyIDTables();
#endif
};

struct Player
{
   Object renderable;
   u16 trackIndex;
   float t;
   MeshObject mesh;
};

struct GameState
{
   RenderState renderer;

   Camera camera;
   Player sphereGuy;
   KeyState keyState;
   NewTrackGraph tracks;
   Arena mainArena;

   v3 lightPos;

   Image tempFontField; // @delete!
   FontData tempFontData; // @delete!
   GLuint fontTextureHandle; //@delete!  

   TextProgram bitmapFontProgram;
   TextProgram fontProgram;
   GLuint backgroundProgram;

   stbFont bitmapFont;   

   enum
   {
      START,      
      INITGAME,
      RESET,
      LOOP,
   };

   i32 state;

   #ifdef TIMERS
   u64 TrackRenderTime;
   u64 TrackSortTime;
   u64 TrackGenTime;
   u64 GameLoopTime;
   #endif
};
