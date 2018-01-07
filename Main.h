#define TRACK_SEGMENT_SIZE 10.0f

#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

static float delta;

struct Camera
{
   quat orientation;
   v3 position;
   m4 view;
   v3 forward;

   float distanceFromPlayer;

   void UpdateView();
};

struct BBox
{
   v3 position;
   v3 magnitude;
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
   
#define LEFT_CURVE  BranchCurve(0, 0, -1, 1)
#define RIGHT_CURVE BranchCurve(0, 0, 1, 1)

static Curve GlobalLinearCurve;
static Curve GlobalBranchCurve;
static Curve GlobalBreakCurve;
static Curve GlobalRightCurve;
static Curve GlobalLeftCurve; 

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
      speedup = 0x100,
      lockedLeft = 0x200,
      lockedRight = 0x400,
      lockedMask = 0x600
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
   inline void addEdge(u16 edgeID);
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
      speedup = 0x10
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
   CircularQueue<u16> newBranches;
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
   inline u16 HasLinearAncestor(u16 actual);

#ifdef DEBUG
   void VerifyGraph();
   void VerifyIDTables();
#endif
};

struct Player
{
   enum
   {
      Force_Left = 0x1,
      Force_Right = 0x2,
   };

   Object renderable;
   u16 trackIndex;
   u16 tracksTraversedSequence;
   u16 timesAccelerated;
   float t;
   float velocity;
   MeshObject mesh;
   u8 forceDirection;

   struct PlayerAnimation
   {
      float t;
   };

   PlayerAnimation animation;

   inline bool OnSwitch(NewTrackGraph &tracks);
};

inline bool
Player::OnSwitch(NewTrackGraph &tracks)
{
   return tracks.adjList[tracks.GetActualID(trackIndex)].flags & Attribute::branch;
}

struct GameState
{
   RenderState renderer;
   PlatformInputState input;

   Camera camera;
   Player sphereGuy;
   KeyState keyState;
   NewTrackGraph tracks;
   Arena mainArena;
   AssetManager assetManager;

   v3 lightPos;

   TextProgram bitmapFontProgram;
   TextProgram fontProgram;   

   OpenglState glState;

   bool paused;
   u32 maxDistance;

   u32 framerate;
   
   enum
   {
      START,
      PAUSE,
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

   #ifdef ANDROID_BUILD
   AndroidState *android;
   #endif
};

struct RebuildState
{
   Camera camera;
   Player player;
   v3 lightPos;
   Attribute trackAttributes[1024];
   Track tracks[1024];

   // max = 1024
   i32 availableIDsBegin;
   i32 availableIDsEnd;
   i32 availableIDsSize;
   u16 availableIDs[1024];

   // max = 1024
   i32 ordersBegin;
   i32 ordersEnd;
   i32 ordersSize;
   NewTrackOrder orders[1024];

   // max = 1024
   i32 newBranchesBegin;
   i32 newBranchesEnd;
   i32 newBranchesSize;
   u16 newBranches[256];

   // capacity = 1024
   Element takenElements[1024];
   u32 takenSize;

   u16 IDtable[1024];
   u16 reverseIDtable[1024];
   u8 trackGraphFlags;

   float switchDelta;
   Curve beginLerp;
   Curve endLerp;
};

void ReloadState(RebuildState *saved, GameState &result);
static B_INLINE Curve BranchCurve(i32 x1, i32 y1, i32 x2, i32 y2);
static B_INLINE Curve BreakCurve();
static B_INLINE Curve LinearCurve(i32 x1, i32 y1, i32 x2, i32 y2);
static B_INLINE v2 CubicBezier(v2 p1, v2 p2, v2 p3, v2 p4, float t);
static B_INLINE v2 CubicBezier(Curve c, float t);
static B_INLINE v2 Tangent(Curve c, float t);
void AllocateTrackGraphBuffers(NewTrackGraph &g, StackAllocator *allocator);
void GameLoop(GameState &state);
void GameInit(GameState &state, RebuildState *rebuild, size_t rebuildSize);
void GameEnd(GameState &state);
RebuildState *SaveState(GameState *state);
B_INLINE v2 ScreenToClip(v2i input);
