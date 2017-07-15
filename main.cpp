
#define TRACK_SEGMENT_SIZE 12.0f

#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

#define LEVEL_LENGTH 0.5f

static float delta;
static m4 Projection = Projection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 100.0f, 60.0f);
static m4 InfiniteProjection = InfiniteProjection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 60.0f);

static const u32 RectangleAttribCount = 6;
static GLuint RectangleUVBuffer;
static float RectangleUVs[] =
{
   0.0f, 1.0f,
   1.0f, 1.0f,
   0.0f, 0.0f,

   1.0f, 1.0f,
   1.0f, 0.0f,
   0.0f, 0.0f
};

static GLuint RectangleVertBuffer;
static float RectangleVerts[] =
{
   -0.5f, -0.5f,
   0.5f, -0.5f,
   -0.5f, 0.5f,

   0.5f, -0.5f,
   0.5f, 0.5f,
   -0.5f, 0.5f
};

static GLuint ScreenVertBuffer;
static float ScreenVerts[] =
{
   -1.0f, -1.0f,
   1.0f, -1.0f,
   -1.0f, 1.0f,

   1.0f, -1.0f,
   1.0f, 1.0f,
   -1.0f, 1.0f
};

#define BEGIN_TIME() u64 LOCAL_TIME = __rdtsc()
#define END_TIME() LOCAL_TIME = __rdtsc() - LOCAL_TIME
#define READ_TIME() LOCAL_TIME

struct Arena
{
   size_t size;
   u8 *base;
   u8 *current;
};

struct StackAllocator
{
   struct Chunk
   {
      Chunk *last;
      u8 *base;
      size_t size;
   };

   Chunk *last;
   u8 *base;
   
   u8 *push(size_t size);
   void pop();
};

static inline
void InitStackAllocator(StackAllocator *allocator)
{      
   allocator->base = (u8 *)allocator + sizeof(StackAllocator);
   allocator->last = (StackAllocator::Chunk *)allocator->base;
   allocator->last->size = 0;
   allocator->last->base = (u8 *)allocator->last;
}

u8 *StackAllocator::push(size_t allocation)
{
   Chunk *newChunk = (Chunk *)(last->base + last->size);
   newChunk->base = ((u8 *)newChunk) + sizeof(Chunk);
   newChunk->size = allocation;
   newChunk->last = last;

   last = newChunk;

   return newChunk->base;
}

void StackAllocator::pop()
{
   last->size = 0;
   last->base = (u8 *)last;
   last = last->last;
}

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
};

static
m4 CameraMatrix(Camera &camera)
{
   //calculate the inverse by computing the conjugate
   //the normal of a quaternion is the same as the unit for a v4
   quat inverse;
   inverse.V4 = unit(camera.orientation.V4);
   inverse.x = -inverse.x;
   inverse.y = -inverse.y;
   inverse.z = -inverse.z;
   inverse.w = -inverse.w;

   m4 rotation = M4(inverse);
   m4 translation = Translate(-camera.position.x,
			      -camera.position.y,
			      -camera.position.z);

   return rotation * translation;
}

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

static MeshObject Sphere;
static MeshObject LinearTrack;
static MeshObject BranchTrack;

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

static ShaderProgram DefaultShader;

// Higher level Game Object abstraction
struct Object
{
   v3 worldPos;
   v3 scale;
   quat orientation;
   MeshObject meshBuffers;
   ShaderProgram *p;
};

union tri2
{
   v2 e[3];

   struct
   {
      v2 a, b, c;
   };
};

static inline
tri2 Tri2(v2 a, v2 b, v2 c)
{
   tri2 result;

   result.a = a;
   result.b = b;
   result.c = c;

   return result;
}

static inline
v3 V3(v2 a, float z)
{
   return {a.x, a.y, z}; 
}

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

static Curve GlobalLinearCurve;
static Curve GlobalBranchCurve;

static inline
Curve InvertX(Curve c)
{
   Curve result;
   result.p1 = V2(-c.p1.x, c.p1.y);
   result.p2 = V2(-c.p2.x, c.p2.y);
   result.p3 = V2(-c.p3.x, c.p3.y);
   result.p4 = V2(-c.p4.x, c.p4.y);
   return result;
}

static inline
Curve lerp(Curve a, Curve b, float t)
{
   Curve result;
   result.lerpables[0] = lerp(a.lerpables[0], b.lerpables[0], t);
   result.lerpables[1] = lerp(a.lerpables[1], b.lerpables[1], t);
   return result;
}

static __forceinline
v2 CubicBezier(v2 p1, v2 p2, v2 p3, v2 p4, float t)
{
   // uses the quicker non matrix form of the equation
   v2 result;   

   float degree1 = 1 - t;   
   float degree2 = degree1 * degree1; // (1 - t)^2
   float degree3 = degree2 * degree1; // (1 - t)^3
   float squared = t * t;   
   float cubed = squared * t;   

   result.x = degree3 * p1.x + 3 * t * degree2 * p2.x + 3 * squared * degree1 * p3.x + cubed * p4.x;
   result.y = degree3 * p1.y + 3 * t * degree2 * p2.y + 3 * squared * degree1 * p3.y + cubed * p4.y;

   return result;
}

static __forceinline
v2 CubicBezier(Curve c, float t)
{
   return CubicBezier(c.p1, c.p2, c.p3, c.p4, t);
}

static inline
GLuint UploadVertices(float *vertices, i32 count, i32 components = 3)
{
   GLuint result;
   glGenBuffers(1, &result);
   glBindBuffer(GL_ARRAY_BUFFER, result);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * count * components), (void *)vertices, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   return result;
}

MeshBuffers
UploadStaticMesh(float *vertices, v3 *normals, i32 count, i32 components = 3)
{
   MeshBuffers result;
   
   glGenVertexArrays(1, &result.vao);
   glBindVertexArray(result.vao);
   glGenBuffers(1, &result.vbo);
   glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * count * components), (void *)vertices, GL_STATIC_DRAW);   
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, components, GL_FLOAT, GL_FALSE, 0, 0);
   glGenBuffers(1, &result.nbo);
   glBindBuffer(GL_ARRAY_BUFFER, result.nbo);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * count * components), (void *)normals, GL_STATIC_DRAW);
   glEnableVertexAttribArray(NORMAL_LOCATION);
   glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
   glBindVertexArray(0);

   return result;
}

// like the previous function, but does not upload any data, just allocates the buffers
// NOT NECASSARY TO CREATING MESH BUFFERS (you may want to use the function above)
MeshBuffers AllocateMeshBuffers(i32 count, i32 components = 3)
{
   MeshBuffers result;
   
   glGenVertexArrays(1, &result.vao);
   glBindVertexArray(result.vao);
   glGenBuffers(1, &result.vbo);
   glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * count * components), 0, GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, components, GL_FLOAT, GL_FALSE, 0, 0);
   glGenBuffers(1, &result.nbo);
   glBindBuffer(GL_ARRAY_BUFFER, result.nbo);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * count * components), 0, GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(NORMAL_LOCATION);
   glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
   glBindVertexArray(0);

   return result;
}

ShaderProgram
CreateProgram(char *vertexSource, size_t vsize, char *fragmentSource, size_t fsize)
{
   ShaderProgram result;

   result.programHandle = glCreateProgram();
   result.vertexHandle = glCreateShader(GL_VERTEX_SHADER);
   result.fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(result.vertexHandle, 1, &vertexSource, (i32 *)&vsize);
   glShaderSource(result.fragmentHandle, 1, &fragmentSource, (i32 *)&fsize);

   glCompileShader(result.vertexHandle);
   glCompileShader(result.fragmentHandle);

   glAttachShader(result.programHandle, result.vertexHandle);
   glAttachShader(result.programHandle, result.fragmentHandle);

   glLinkProgram(result.programHandle);

   result.modelUniform = glGetUniformLocation(result.programHandle, "m");
   result.viewUniform = glGetUniformLocation(result.programHandle, "v");
   result.MVPUniform = glGetUniformLocation(result.programHandle, "mvp");
   result.lightPosUniform = glGetUniformLocation(result.programHandle, "lightPos");
   result.diffuseUniform = glGetUniformLocation(result.programHandle, "diffuseColor");

   result.texUniform = glGetUniformLocation(result.programHandle, "tex");

   result.vertexAttrib = 1;
   result.normalAttrib = 2;

   return result;
}

static
v3 *Normals(float *vertices, v3 *result, i32 count)
{
   tri *tris = (tri *)vertices;
   i32 faceCount = count / 3;

   for(i32 i = 0; i < faceCount; ++i)
   {
      v3 a = tris[i].b - tris[i].a;
      v3 b = tris[i].c - tris[i].a;

      v3 normal = unit(cross(a, b));
      result[(i * 3)] = normal;
      result[(i * 3) + 1] = normal;
      result[(i * 3) + 2] = normal;
   }

   return result;
}

static
v3 *Normals(float *vertices, i32 count)
{
   v3 *buffer = (v3 *)malloc(count * sizeof(v3));
   return Normals(vertices, buffer, count);
}

// Allocates Mesh object and vertex buffers
// NOT NECASSARY TO CREATING A MESH OBJECT
// Only for allocating dynamic gpu buffers
MeshObject AllocateMeshObject(i32 vertexCount)
{   
   MeshObject result;
   result.mesh.vcount = vertexCount;
   result.mesh.vertices = (float *)malloc(sizeof(v3) * vertexCount);
   result.mesh.normals = (v3 *)malloc(sizeof(v3) * vertexCount);

   result.handles = AllocateMeshBuffers(result.mesh.vcount, 3);
   return result;
}

// Inits Mesh Object and uploads to vertex buffers
MeshObject InitMeshObject(char *filename)
{
   size_t size = WinFileSize(filename);
   u8 *buffer = (u8 *)malloc(size);
   WinReadFile(filename, buffer, size);

   Mesh mesh;

   mesh.vcount = *((i32 *)buffer);
   mesh.vertices = (float *)(buffer + 4);
   mesh.normals = Normals(mesh.vertices, mesh.vcount);
   MeshBuffers handles = UploadStaticMesh(mesh.vertices, mesh.normals, mesh.vcount, 3);

   MeshObject result;
   result.mesh = mesh;
   result.handles = handles;   

   return result;
}

void RenderPushMesh(ShaderProgram p, MeshObject b, m4 &transform, m4 &view, v3 lightPos, v3 diffuseColor = V3(0.3f, 0.3f, 0.3f))
{
   glUseProgram(p.programHandle);

   static float time = 0.0f;
   time += delta;

   m4 lightRotation = M4(Rotation(V3(0.0f, 1.0f, 0.0f), -time / 50.0f));
   m4 projection = InfiniteProjection;
   m4 mvp = projection * view * transform;
   
   glUniformMatrix4fv(p.MVPUniform, 1, GL_FALSE, mvp.e);
   glUniformMatrix4fv(p.modelUniform, 1, GL_FALSE, transform.e);
   glUniformMatrix4fv(p.viewUniform, 1, GL_FALSE, view.e);
   glUniform3fv(p.diffuseUniform, 1, diffuseColor.e);
   glUniform3fv(p.lightPosUniform, 1, lightPos.e);
   glBindVertexArray(b.handles.vao);
   glDrawArrays(GL_TRIANGLES, 0, b.mesh.vcount);
   glBindVertexArray(0);
   glUseProgram(0);
}

static
void RenderPushObject(Object &obj, m4 &camera, v3 lightPos, v3 diffuseColor = V3(0.3f, 0.3f, 0.3f))
{
   m4 orientation = M4(obj.orientation);
   m4 scale = Scale(obj.scale);
   m4 translation = Translate(obj.worldPos);
   
   m4 transform = translation * scale * orientation;
   
   RenderPushMesh(*obj.p, obj.meshBuffers, transform, camera, lightPos, diffuseColor);
}

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

static inline
Track CreateTrack(v3 position, v3 scale, Curve *bezier, MeshObject &buffers)
{
   Object obj;
   obj.worldPos = position;
   obj.scale = scale;
   obj.orientation = Quat(0.0f, 0.0f, 0.0f, 0.0f);
   obj.meshBuffers = buffers;
   obj.p = &DefaultShader;

   Track result;
   result.renderable = obj;
   result.bezier = bezier;

   return result;
}

struct TrackAttribute
{
   enum
   {
      countMask = 0x3,
      branch = 0x4,
      left = 0x8,
      right = 0x10,
      breaks = 0x20,
   };
   
   i32 flags;
   u32 e[2]; // each track can have at most 2 edges leading from it.

   inline
   i32 hasLeft()
   {
      return flags & left;
   }

   inline
   i32 hasRight()
   {
      return flags & right;
   }

   inline
   u32 getRight()
   {
      return e[1];
   }

   inline
   u32 getLeft()
   {
      return e[0];
   }
};

struct TrackGraph
{
   enum
   {
      left = 0x1,
      switching = 0x2,
   };

   Track *elements;
   TrackAttribute *adjList;
   i32 capacity;
   i32 size;
   i32 head;
   i32 flags;

   // switch lerp state 
   float switchDelta; // how much time has progressed (0 to 1) after the switch is flipped
   Curve beginLerp;
   Curve endLerp;
};

template <typename T>
struct CircularQueue
{
   i32 max;
   i32 size;
   i32 begin;
   i32 end;
   T *elements;

   CircularQueue(i32 _max);
   ~CircularQueue();
   void Push(T e);
   T Pop();
   void ClearToZero();   

private:
   inline void IncrementEnd();
   inline void IncrementBegin();
};

template <typename T>
CircularQueue<T>::CircularQueue(i32 _max)
{
   max = _max;
   size = 0;
   begin = 0;
   end = 0;
   elements = (T *)malloc(max * sizeof(T));
}

template <typename T>
CircularQueue<T>::~CircularQueue()
{
   free(elements);
}

template <typename T>
void CircularQueue<T>::Push(T e)
{
   assert(size <= max);

   elements[end] = e;
   ++size;
   IncrementEnd();
}

template <typename T>
T CircularQueue<T>::Pop()
{
   assert(size > 0);

   T pop = elements[begin];
   --size;
   IncrementBegin();

   return pop;
}

template <typename T>
void CircularQueue<T>::IncrementBegin()
{
   if(begin == max - 1)
   {
      begin = 0;
   }
   else
   {
      ++begin;
   }
}

template <typename T>
void CircularQueue<T>::IncrementEnd()
{
   if(end == max - 1)
   {
      end = 0;
   }
   else
   {
      ++end;
   }
}

template <typename T>
void CircularQueue<T>::ClearToZero()
{
   memset(elements, 0, max * sizeof(T));
}

// an "order" for a track to be placed
// used in the queue when creating the graph
struct TrackOrder
{
   u32 index;
   i32 x, y;

   enum
   {
      dontBranch = 0x1,
   };
   i32 rules;
};

struct VirtualTrackCoords
{
   i32 x, y;
};

static inline
v2 VirtualToReal(i32 x, i32 y)
{
   return V2((float)x * TRACK_SEGMENT_SIZE, (float)y * TRACK_SEGMENT_SIZE);
}

// returns a linear bezier curve
static inline
Curve LinearCurve(i32 x1, i32 y1,
		  i32 x2, i32 y2)
{
   v2 begin = VirtualToReal(x1, y1);
   v2 end = VirtualToReal(x2, y2);

   v2 direction = end - begin;
   
   // right now, just a straight line
   Curve result;
   result.p1 = V2(0.0f, 0.0f);
   result.p2 = 0.333333f * direction;
   result.p3 = 0.666666f * direction;
   result.p4 = direction;

   return result;
}

static inline
Curve BreakCurve(i32 x, i32 y)
{
   v2 begin = VirtualToReal(x, y);
   v2 end = V2(begin.x, begin.y + (TRACK_SEGMENT_SIZE / 2.0f));

   v2 direction = end - begin;

   Curve result;
   result.p1 = begin;
   result.p2 = 0.333333f * direction;
   result.p3 = 0.666666f * direction;
   result.p4 = direction;

   return result;
}

static inline
Curve BranchCurve(i32 x1, i32 y1,
		  i32 x2, i32 y2)
{
   v2 begin = VirtualToReal(x1, y1);
   v2 end = VirtualToReal(x2, y2);

   v2 direction = end - begin;

   // right now, just a straight line
   Curve result;
   result.p1 = V2(0.0f, 0.0f);
   result.p2 = V2(0.0f, direction.y * 0.666667f);
   result.p3 = V2(direction.x, direction.y * 0.333333f);
   result.p4 = direction;

   return result;
}

struct VirtualCoord
{
   i32 x;
   i32 y;
};

inline
i32 CompareVirtualCoords(VirtualCoord a, VirtualCoord b)
{
   return a.x == b.x && a.y == b.y;
}

inline
i32 HashVirtualCoord(VirtualCoord key)
{
   return key.x + key.y;
}

static
TrackGraph InitTrackGraph(i32 initialSize)
{
   BEGIN_TIME();
   
   TrackGraph result;

   result.head = 0;   
   result.flags = TrackGraph::left;

   // @leak
   result.elements = (Track *)malloc(sizeof(Track) * initialSize);
   // @leak
   result.adjList = (TrackAttribute *)malloc(sizeof(TrackAttribute) * initialSize);

   for(i32 i = 0; i < initialSize; ++i)
   {
      result.adjList[i] = {0};
   }

   HashMap<VirtualCoord, u32, HashVirtualCoord, CompareVirtualCoords, 0> taken(1024); // lol

   enum
   {
      hasTrack = 0x1,
      isBranch = 0x2,
      hasBreak = 0x4,
   };

   result.capacity = initialSize;   
   CircularQueue<TrackOrder> orders(initialSize);
   
   orders.Push({0, 0, 0, 0}); // Push root segment.
   u32 firstFree = 1; // next element that can be added to the queue

   i32 processed = 0;
   while(orders.size > 0)
   {
      TrackOrder item = orders.Pop();
      ++processed;

      long roll = rand();
      if(!(item.rules & TrackOrder::dontBranch) && roll % 4 == 0) // is a branch
      {
	 // amount of subsequent branches
	 int branches = 0;      	 	 
	 
	 // Queue up subsequent branches
	 // can we fit a left track?
	 u32 leftFlags = taken.get({item.x-1, item.y+1});
	 if(initialSize - firstFree > 0 &&
	    !(leftFlags & (hasTrack | hasBreak)))
	 {
	    ++branches;
	    orders.Push({firstFree, item.x-1, item.y+1, TrackOrder::dontBranch}); // left side

	    taken.put({item.x-1, item.y+1}, leftFlags | hasTrack | isBranch | (firstFree << 2));
	    result.adjList[item.index].e[0] = firstFree++;
	    result.adjList[item.index].flags |= TrackAttribute::left;
	 }

	 else if(leftFlags & (hasTrack | hasBreak)) // is a left track already in that space?
	 {
	    result.adjList[item.index].e[0] = leftFlags >> 2;
	    result.adjList[item.index].flags |= TrackAttribute::left;
	    ++branches;
	 }
	 
	 // can we fit a right track?
	 u32 rightFlags = taken.get({item.x+1, item.y+1});
	 if(initialSize - firstFree > 0 &&	    
	    !(rightFlags & (hasTrack | hasBreak)))
	 {
	    ++branches;
	    orders.Push({firstFree, item.x+1, item.y+1, TrackOrder::dontBranch}); // right side

	    taken.put({item.x+1, item.y+1}, rightFlags | hasTrack | isBranch | (firstFree << 2));
	    result.adjList[item.index].e[1] = firstFree++;
	    result.adjList[item.index].flags |= TrackAttribute::right;
	 }
	 else if(rightFlags & (hasTrack | hasBreak)) // is a right track already in that space
	 {
	    result.adjList[item.index].e[1] = rightFlags >> 2;
	    result.adjList[item.index].flags |= TrackAttribute::right;
	    ++branches;
	 }
	 
	 result.adjList[item.index].flags |= branches | TrackAttribute::branch;

	 v2 position = VirtualToReal(item.x, item.y);
       
	 result.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), &GlobalBranchCurve, BranchTrack);
	 result.elements[item.index].flags = Track::branch | Track::left;	 
      }
      else // is linear
      {
	 u32 behindFlags = taken.get({item.x, item.y+1});
	 if((initialSize - firstFree) > 0 &&
	    !(behindFlags & hasTrack))
	 {
	    result.adjList[item.index].flags = 1 | TrackAttribute::left; // size of 1, is linear so has a "left" track following
	    orders.Push({firstFree, item.x, item.y+1, 0});
	    taken.put({item.x, item.y+1}, behindFlags | hasTrack | (firstFree << 2));
	    result.adjList[item.index].e[0] = firstFree++; // When a Track is linear, the index of the next Track is e[0]
	 }
	 else if(behindFlags & hasTrack)
	 {
	    result.adjList[item.index].e[0] = behindFlags << 2;
	 }
	 
	 v2 position = VirtualToReal(item.x, item.y);
	 result.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve, LinearTrack);
	 result.elements[item.index].flags = Track::staticMesh;

	 // when placing tracks after branches, if there is already
	 // a linear track behind the placed track, it will have to be manually
	 // connected here.
	 u32 behind = taken.get({item.x, item.y-1});
	 if(behind)
	 {
	    u32 index = behind >> 2;

	    // if it is not a branch, and if it is not already connected
	    if(!(result.adjList[index].flags & TrackAttribute::branch) && (result.adjList[index].flags & TrackAttribute::countMask) == 0)
	    {
	       result.adjList[index].flags |= 1 | TrackAttribute::left;
	       result.adjList[index].e[0] = item.index;
	    }	 
	 }
      }      
   }

   result.size = processed;
   END_TIME();
   
   return result;
}

struct Player
{
   Object renderable;
   Track *currentTrack;
   i32 trackIndex;
   float t;
};

struct GameState
{   
   Camera camera;
   Player sphereGuy;
   KeyState keyState;
   TrackGraph tracks;
   Arena mainArena;

   v3 lightPos;

   Image tempFontField; // @delete!
   FontData tempFontData; // @delete!
   GLuint fontTextureHandle; //@delete!

   ShaderProgram textureProgram;
};

static
ShaderProgram LoadFilesAndCreateProgram(char *vertex, char *fragment, StackAllocator *allocator)
{
   size_t vertSize = WinFileSize(vertex);
   size_t fragSize = WinFileSize(fragment);
   char *vertBuffer = (char *)allocator->push(vertSize);
   char *fragBuffer = (char *)allocator->push(fragSize);
   WinReadFile(vertex, (u8 *)vertBuffer, vertSize);
   WinReadFile(fragment, (u8 *)fragBuffer, fragSize);

   ShaderProgram result = CreateProgram(vertBuffer, vertSize, fragBuffer, fragSize);

   allocator->pop();
   allocator->pop();
   
   return result;
}

// the globalName "LoadImage" is already taken
static 
Image LoadImageFile(char *filename, StackAllocator *allocator)
{
   size_t fileSize = WinFileSize(filename);
   // @leak
   u8 *buffer = allocator->push(fileSize);

   WinReadFile(filename, buffer, fileSize);

   ImageHeader *header = (ImageHeader *)buffer;
   Image result;

   result.x = header->x;
   result.y = header->y;
   result.channels = header->channels;
   result.data = buffer + sizeof(header);

   return result;
}

static
FontData LoadFontFile(char *filename, StackAllocator *allocator)
{
   size_t fileSize = WinFileSize(filename);

   u8 *buffer = allocator->push(fileSize);
   WinReadFile(filename, buffer, fileSize);

   FontHeader *header = (FontHeader *)buffer;
   FontData result;

   result.count = header->count;
   result.mapWidth = header->mapWidth;
   result.mapHeight = header->mapHeight;
   result.data = (CharInfo *)(buffer + sizeof(FontHeader));

   return result;
}

static inline
Object SpherePrimitive(v3 position, v3 scale, quat orientation)
{
   Object result;
   result.worldPos = position;
   result.scale = scale;
   result.orientation = orientation;
   result.meshBuffers = Sphere;
   result.p = &DefaultShader;
   return result;
}

static __forceinline
v3 GetPositionOnTrack(Track &track, float t)
{   
   v2 displacement = CubicBezier(*track.bezier, t);
   return V3(displacement.x + track.renderable.worldPos.x,
	     displacement.y + track.renderable.worldPos.y,
	     track.renderable.worldPos.z);
}

void UpdatePlayer(Player &player, TrackGraph &tracks)
{
   player.t += 0.1f * delta;
   if(player.t > 1.0f)
   {
      player.t -= 1.0f;

      if((tracks.flags & TrackGraph::left) || (player.currentTrack->flags & Track::branch) == 0)
      {
	 if(tracks.adjList[player.trackIndex].hasLeft())
	 {
	    u32 newIndex = tracks.adjList[player.trackIndex].getLeft();
	    player.currentTrack = &tracks.elements[newIndex];
	    player.trackIndex = newIndex;
	 }
	 else
	 {
	    assert(false);
	 }
      }
      else
      {
	 if(tracks.adjList[player.trackIndex].hasRight())
	 {
	    i32 newIndex = tracks.adjList[player.trackIndex].getRight();
	    player.currentTrack = &tracks.elements[newIndex];
	    player.trackIndex = newIndex;
	 }
	 else
	 {
	    assert(false);
	 }
      }
   }   

   player.renderable.worldPos = GetPositionOnTrack(*player.currentTrack, player.t);
}

void UpdateCamera(Camera &camera, const Player &player, const TrackGraph &graph)
{
   v3 playerPosition = player.renderable.worldPos;
   camera.position.y = playerPosition.y - 5.0f;
   camera.position.x = playerPosition.x;
}

static __forceinline
v2 Tangent(Curve c, float t)
{
   v2 result;

   float minust = 1.0f - t;

   result.x = 3 * (minust * minust) * (c.p2.x - c.p1.x) + 6.0f * minust * t * (c.p3.x - c.p2.x) + 3.0f * t * t * (c.p4.x - c.p3.x);
   result.y = 3 * (minust * minust) * (c.p2.y - c.p1.y) + 6.0f * minust * t * (c.p3.y - c.p2.y) + 3.0f * t * t * (c.p4.y - c.p3.y);

   return unit(result);
}

static
void GenerateTrackSegmentVertices(MeshObject &meshBuffers, Curve bezier)
{
   // 10 segments, 8 tris per segment --- 80 tries   
   tri *tris = (tri *)meshBuffers.mesh.vertices;

   float t = 0.0f;
   v2 sample = CubicBezier(bezier, t);

   v2 direction = Tangent(bezier, t);
   v2 perpindicular = V2(-direction.y, direction.x) * 0.5f;         

   v3 top1 = V3(sample.x, sample.y, 0.5f);
   v3 right1 = V3(sample.x + perpindicular.x, sample.y, 0.0f);
   v3 bottom1 = V3(sample.x, sample.y, -0.5f);
   v3 left1 = V3(sample.x - perpindicular.x, sample.y, 0.0f);   
   
   for(i32 i = 0; i < 10; ++i)
   {
      t += 0.1f;
      sample = CubicBezier(bezier, t);

      direction = Tangent(bezier, t);
      
      perpindicular = V2(-direction.y, direction.x) * 0.5f;      

      v3 top2 = V3(sample.x, sample.y, 0.5f);
      v3 right2 = V3(sample.x + perpindicular.x, sample.y + perpindicular.y, 0.0f);
      v3 bottom2 = V3(sample.x, sample.y, -0.5f);
      v3 left2 = V3(sample.x - perpindicular.x, sample.y - perpindicular.y, 0.0f);
      
      i32 j = i * 8;

      // top left side
      tris[j] = Tri(top1, left1, left2);
      tris[j+1] = Tri(top1, left2, top2);

      // top right side
      tris[j+2] = Tri(top1, top2, right1);
      tris[j+3] = Tri(top2, right2, right1);

      // bottom right side
      tris[j+4] = Tri(bottom1, right1, bottom2);
      tris[j+5] = Tri(bottom2, right1, right2);

      // bottom left side
      tris[j+6] = Tri(bottom1, left2, left1);
      tris[j+7] = Tri(bottom1, bottom2, left2);

      top1 = top2;
      right1 = right2;
      left1 = left2;
      bottom1 = bottom2;
   }

   v3 *normals = meshBuffers.mesh.normals;
   Normals((float *)tris, (v3 *)normals, 80 * 3); // 3 vertices per tri

   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, (sizeof(tri) * 80), (void *)tris);
   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.nbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri) * 80, (void *)normals);
   glBindBuffer(GL_ARRAY_BUFFER, 0);   
}

static inline
void GenerateTrackSegmentVertices(Track &track)
{
   GenerateTrackSegmentVertices(track.renderable.meshBuffers, *track.bezier);   
}

void RenderTexture(GLuint texture, ShaderProgram &program)
{
   glUseProgram(program.programHandle);

   assert(glIsTexture(texture));

   glBindBuffer(GL_ARRAY_BUFFER, RectangleVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture);
   glUniform1i(glGetUniformLocation(program.programHandle, "tex"), 0);

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glUseProgram(0);
}

GLuint UploadDistanceTexture(Image &image)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

GLuint UploadTexture(Image &image)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}
/*
static GLuint textVao;
static GLuint textUVVbo;
static
void InitTextBuffers()
{
   glGenVertexArrays(1, &textVao);
   glBindVertexArray(textVao);
   glGenBuffers(1, &textUVVbo);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * 12), 0, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBindVertexArray(0);
}
*/
void GameInit(GameState &state)
{
   state.mainArena.base = Win32AllocateMemory(GIGABYTES(2), &state.mainArena.size);
   state.mainArena.current = state.mainArena.base;

   InitStackAllocator((StackAllocator *)state.mainArena.base);

   assert(state.mainArena.base);

   srand((u32)__rdtsc());

   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   

   DefaultShader = LoadFilesAndCreateProgram("assets\\default.vertp", "assets\\default.fragp",
					     (StackAllocator *)state.mainArena.base);

   state.textureProgram = LoadFilesAndCreateProgram("assets\\texture.vertp", "assets\\texture.fragp",
						    (StackAllocator *)state.mainArena.base);   

   state.tempFontField = LoadImageFile("distance_field.bi", (StackAllocator *)state.mainArena.base);
   state.tempFontData = LoadFontFile("distance_field.bf", (StackAllocator *)state.mainArena.base);   

   state.fontTextureHandle = UploadDistanceTexture(state.tempFontField);
   state.keyState = up;
   Sphere = InitMeshObject("assets\\sphere.brian");

   //@leak
   LinearTrack = AllocateMeshObject(80 * 3);
   BranchTrack = AllocateMeshObject(80 * 3);
   
   GlobalLinearCurve = LinearCurve(0, 0, 0, 1);
   GlobalBranchCurve = BranchCurve(0, 0,
				   -1, 1);

   GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);
   GenerateTrackSegmentVertices(LinearTrack, GlobalLinearCurve);

   RectangleUVBuffer = UploadVertices(RectangleUVs, 6, 2);
   RectangleVertBuffer = UploadVertices(RectangleVerts, 6, 2);
   ScreenVertBuffer = UploadVertices(ScreenVerts, 6, 2);

   //InitTextBuffers();
      
   state.camera.position = V3(0.0f, 0.0f, 10.0f);
   state.camera.orientation = Rotation(V3(1.0f, 0.0f, 0.0f), 1.0f);
   state.lightPos = state.camera.position;

   state.tracks = InitTrackGraph(1024);
   
   state.sphereGuy.renderable = SpherePrimitive(V3(0.0f, -2.0f, 5.0f),
						V3(1.0f, 1.0f, 1.0f),
						Quat(1.0f, 0.0f, 0.0f, 0.0f));

   state.sphereGuy.t = 0.0f;
   state.sphereGuy.currentTrack = &state.tracks.elements[state.tracks.head];
   state.sphereGuy.trackIndex = 0;
}
/*
static
void RenderText(char *string, u32 length, v2 clipCoord, FontData &font,
		ShaderProgram p)
{
   float mapWidth = (float)font.mapWidth;
   float mapHeight = (float)font.mapHeight;

   glBindVertexArray(textVao);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   
   for(u32 i = 0; i < length; ++i)
   {
      char c = string[i];
      CharInfo params;
      for(u32 j = 0; j < font.count; ++j)
      {
	 if(font.data[j].id == c)
	 {
	    params = font.data[j];
	    break;
	 }
      }
   }

   glBindVertexArray(0);
}
*/
void GameLoop(GameState &state)
{   
   if(state.tracks.flags & TrackGraph::switching)
   {
      state.tracks.switchDelta = min(state.tracks.switchDelta + 0.1f * delta, 1.0f);
      GlobalBranchCurve = lerp(state.tracks.beginLerp, state.tracks.endLerp, state.tracks.switchDelta);
      GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);

      if(state.tracks.switchDelta == 1.0f)
      {
	 state.tracks.flags &= ~TrackGraph::switching;
      }
   }

   m4 cameraTransform = CameraMatrix(state.camera);

   UpdatePlayer(state.sphereGuy, state.tracks);
   UpdateCamera(state.camera, state.sphereGuy, state.tracks);
   state.lightPos = state.camera.position;
   
   RenderPushObject(state.sphereGuy.renderable, cameraTransform, state.lightPos, V3(1.0f, 0.0f, 0.0f));
   
   for(i32 i = 0; i < state.tracks.size; ++i)
   {
      RenderPushObject(state.tracks.elements[i].renderable, cameraTransform, state.lightPos, V3(0.0f, 0.0f, 1.0f));
   }

   RenderTexture(state.fontTextureHandle, state.textureProgram);
}

void OnKeyDown(GameState &state)
{
   state.tracks.flags ^= TrackGraph::left;

   // if already lerping
   if(state.tracks.flags & TrackGraph::switching)
   {
      state.tracks.beginLerp = state.tracks.endLerp;
      state.tracks.endLerp = InvertX(state.tracks.beginLerp);
      state.tracks.switchDelta = 1.0f - state.tracks.switchDelta;
   }
   else
   {
      state.tracks.flags |= TrackGraph::switching;

      state.tracks.beginLerp = GlobalBranchCurve;
      state.tracks.endLerp = InvertX(GlobalBranchCurve);
      state.tracks.switchDelta = 0.0f;
   }   
}
   
void GameEnd(GameState &state)
{
   Win32FreeMemory(state.mainArena.base);
}
