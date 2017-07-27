
#define TRACK_SEGMENT_SIZE 12.0f

#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

#define LEVEL_LENGTH 0.5f

static float delta;
static m4 Projection = Projection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 100.0f, 60.0f);
static m4 InfiniteProjection = InfiniteProjection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 60.0f);

static i32 GlobalActionPressed = 0;

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

static inline
void InitStackAllocator(StackAllocator *allocator)
{      
   allocator->base = (u8 *)allocator + sizeof(StackAllocator);
   allocator->last = (StackAllocator::Chunk *)allocator->base;
   allocator->last->size = 0;
   allocator->last->base = (u8 *)allocator->last;
}

inline
u8 *StackAllocator::push(size_t allocation)
{
   Chunk *newChunk = (Chunk *)(last->base + last->size);
   newChunk->base = ((u8 *)newChunk) + sizeof(Chunk);
   newChunk->size = allocation;
   newChunk->last = last;

   last = newChunk;

   ++allocs;

   return newChunk->base;
}

inline
void StackAllocator::pop()
{
   last = last->last;
   --allocs;
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

   m4 view;
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

static
void InitCamera(Camera &camera)
{
   camera.position = V3(0.0f, 0.0f, 10.0f);
   camera.orientation = Rotation(V3(1.0f, 0.0f, 0.0f), 1.0f);
   camera.view = CameraMatrix(camera);   
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

TextProgram
CreateTextProgram(char *vertexSource, size_t vsize, char *fragmentSource, size_t fsize)
{
   TextProgram result;

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

   result.transformUniform = glGetUniformLocation(result.programHandle, "transform");
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
v3 *Normals(float *vertices, i32 count, StackAllocator *allocator)
{
   v3 *buffer = (v3 *)allocator->push(count * sizeof(v3));
   return Normals(vertices, buffer, count);
}

// Allocates Mesh object and vertex buffers
// NOT NECASSARY TO CREATING A MESH OBJECT
// Only for allocating dynamic gpu buffers
MeshObject AllocateMeshObject(i32 vertexCount, StackAllocator *allocator)
{   
   MeshObject result;
   result.mesh.vcount = vertexCount;
   result.mesh.vertices = (float *)allocator->push(sizeof(v3) * vertexCount);
   result.mesh.normals = (v3 *)allocator->push(sizeof(v3) * vertexCount);

   result.handles = AllocateMeshBuffers(result.mesh.vcount, 3);
   allocator->pop();
   allocator->pop();
   return result;
}

// Inits Mesh Object and uploads to vertex buffers
MeshObject InitMeshObject(char *filename, StackAllocator *allocator)
{
   size_t size = FileSize(filename);
   u8 *buffer = (u8 *)allocator->push(size);
   FileRead(filename, buffer, size);

   Mesh mesh;

   mesh.vcount = *((i32 *)buffer);
   mesh.vertices = (float *)(buffer + 4);
   mesh.normals = Normals(mesh.vertices, mesh.vcount, allocator);
   MeshBuffers handles = UploadStaticMesh(mesh.vertices, mesh.normals, mesh.vcount, 3);
   allocator->pop(); // pop normals
   allocator->pop(); // pop file

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
      edgeCountMask = 0x3,
      branch = 0x4,
      left = 0x8,
      right = 0x10,
      breaks = 0x20,
      reachable = 0x40,
      ancestorCountMask = 0x180,
      invisible = 0x200,
   };
   
   i32 flags;
   u32 e[2]; // each track can have at most 2 edges leading from it.
   u32 a[3]; // each track can have up to two "ancestors" (tracks leading to it).

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

   inline
   u32 ancestorCount()
   {
      return (flags & ancestorCountMask) >> 7;
   }

   inline
   u32 edgeCount()
   {
      return (flags & edgeCountMask);
   }

   inline
   void setAncestorCount(u32 count)
   {
      //assert(count <= 2);
      flags = (flags & ~ancestorCountMask) | (count << 7);
   }

   inline
   void setEdgeCount(u32 count)
   {      
      flags = (flags & ~edgeCountMask) | count;
   }

   inline void RemoveAncestor(i32 i);
   inline void AddAncestor(i32 i);
   inline void RemoveEdge(u32 i);
   inline void ReplaceEdge(u32 before, u32 after);
   inline void ReplaceAncestor(u32 before, u32 after);
};

inline
void TrackAttribute::ReplaceEdge(u32 before, u32 after)
{
   if(hasLeft())
   {
      if(getLeft() == before)
      {
	 e[0] = after;
	 return;
      }
   }

   if(hasRight())
   {
      if(getRight() == before)
      {
	 e[1] = after;
	 return;
      }
   }
   assert(false);
}

inline
void TrackAttribute::ReplaceAncestor(u32 before, u32 after)
{
   for(u32 i = 0; i < ancestorCount(); ++i)
   {
      if(a[i] == before)
      {
	 a[i] = after;
	 return;
      }	 
   }
   assert(false);
}


inline
void TrackAttribute::AddAncestor(i32 i)
{
   u32 count = ancestorCount();
   assert(count < 3);
   a[count] = i;
   setAncestorCount(count + 1);
}

inline
void TrackAttribute::RemoveAncestor(i32 i)
{
   if(i == 0 && (ancestorCount() > 1))
   {
      a[0] = a[1];      
   }
   else if(i == 1 && (ancestorCount() > 2))
   {
      a[1] = a[2];
   }

   setAncestorCount(ancestorCount() - 1);
}

inline
void TrackAttribute::RemoveEdge(u32 i)
{
   if(hasLeft())
   {
      if(getLeft() == i)
      {
	 flags &= ~left;
	 setEdgeCount(edgeCount() - 1);
	 return;
      }
   }
   
   if(hasRight())
   {
      if(getRight() == i)
      {
	 flags &= ~right;
	 setEdgeCount(edgeCount() - 1);
	 return;
      }	  
   }
}

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

   void RemoveTrack(u32 i);
};

void
TrackGraph::RemoveTrack(u32 i)
{
   --size;
   // de-attach ancestors
   for(u32 j = 0; j < adjList[i].ancestorCount(); ++j)
   {
      u32 ancestor = adjList[i].a[j];
      adjList[ancestor].RemoveEdge(i);
   }

   for(u32 j = 0; j < adjList[i].edgeCount(); ++j)
   {
      u32 edge = adjList[i].e[j];

      if(adjList[edge].ancestorCount())
      {
	 adjList[edge].RemoveAncestor(i);
      }
   }
}

template <typename T>
struct CircularQueue
{
   i32 max;
   i32 size;
   i32 begin;
   i32 end;
   T *elements;

   CircularQueue(i32 _max, StackAllocator *allocator);
   void Push(T e);
   T Pop();
   void ClearToZero();   

private:
   inline void IncrementEnd();
   inline void IncrementBegin();
};

template <typename T>
CircularQueue<T>::CircularQueue(i32 _max, StackAllocator *allocator)
{
   max = _max;
   size = 0;
   begin = 0;
   end = 0;
   elements = (T *)allocator->push(max * sizeof(T));
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
   u32 ancestor;
   i32 x, y;

   enum
   {
      dontBranch = 0x1,
      noAncestor = 0x2,
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
u32 HashVirtualCoord(VirtualCoord key)
{
   return key.x ^ key.y;
}

static
TrackGraph AllocateTrackGraph(i32 size, StackAllocator *allocator)
{
   TrackGraph result;
   result.capacity = size;
   result.adjList = (TrackAttribute *)allocator->push(sizeof(TrackAttribute) * size);
   result.elements = (Track *)allocator->push(sizeof(Track) * result.capacity);   
   return result;
}

static size_t trackGenTime = 0; // @DELETE

void CheckGraph(TrackGraph &graph)
{
      // ensure graph was formed properly
   for(u32 i = 0; i < (u32)graph.size; ++i)
   {
      for(u32 j = 0; j < graph.adjList[i].ancestorCount(); ++j)
      {
	 u32 ancestor = graph.adjList[i].a[j];

	 if(graph.adjList[ancestor].hasLeft())
	 {
	    if(i == graph.adjList[ancestor].getLeft()) continue;	    
	 }
	 if(graph.adjList[ancestor].hasRight())
	 {
	    if(i == graph.adjList[ancestor].getRight()) continue;
	 }
	 
	 assert(false);	 
      }

      if(graph.adjList[i].hasLeft())
      {
	 u32 left = graph.adjList[i].getLeft();

	 i32 good = 0;
	 for(u32 j = 0; j < graph.adjList[left].ancestorCount(); ++j)
	 {
	    if(i == graph.adjList[left].a[j])
	    {
	       good = true;
	       break;
	    }
	 }

	 assert(good);
      }
      
      if(graph.adjList[i].hasRight())
      {
	 u32 right = graph.adjList[i].getRight();

	 i32 good = 0;
	 for(u32 j = 0; j < graph.adjList[right].ancestorCount(); ++j)
	 {
	    if(i == graph.adjList[right].a[j])
	    {
	       good = true;
	       break;
	    }
	 }

	 assert(good);
      }
   }
}

static
void ReInitTrackGraph(TrackGraph &graph, StackAllocator *allocator)
{
   BEGIN_TIME();

   graph.head = 0;   
   graph.flags = TrackGraph::left;   

   for(i32 i = 0; i < graph.capacity; ++i)
   {
      graph.adjList[i] = {0};
      graph.elements[i] = {0};
   }

   // stored in hashmap
   enum
   {
      hasTrack = 0x1,
      isBranch = 0x2,
   };
   HashMap<VirtualCoord, u32, HashVirtualCoord, CompareVirtualCoords, 0> taken(1024, allocator); // lol   
   
   CircularQueue<TrackOrder> orders(graph.capacity, allocator);
   
   orders.Push({0, 0, 0, 0, TrackOrder::noAncestor}); // Push root segment.
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
	 if(graph.capacity - firstFree > 0 &&
	    !(leftFlags & (hasTrack)))
	 {
	    ++branches;
	    orders.Push({firstFree, item.index, item.x-1, item.y+1, TrackOrder::dontBranch}); // left side

	    taken.put({item.x-1, item.y+1}, (leftFlags & 0x3) | hasTrack | isBranch | (firstFree << 2));
	    graph.adjList[item.index].e[0] = firstFree++;
	    graph.adjList[item.index].flags |= TrackAttribute::left;
	 }

	 else if(leftFlags & (hasTrack)) // is a left track already in that space?
	 {
	    u32 index = leftFlags >> 2;
	    graph.adjList[item.index].e[0] = index;
	    graph.adjList[item.index].flags |= TrackAttribute::left;
	    graph.adjList[index].AddAncestor(item.index);
	    ++branches;
	 }
	 
	 // can we fit a right track?
	 u32 rightFlags = taken.get({item.x+1, item.y+1});
	 if(graph.capacity - firstFree > 0 &&	    
	    !(rightFlags & (hasTrack)))
	 {
	    ++branches;
	    orders.Push({firstFree, item.index, item.x+1, item.y+1, TrackOrder::dontBranch}); // right side

	    taken.put({item.x+1, item.y+1}, (rightFlags & 0x3) | hasTrack | isBranch | (firstFree << 2));
	    graph.adjList[item.index].e[1] = firstFree++;
	    graph.adjList[item.index].flags |= TrackAttribute::right;
	 }
	 else if(rightFlags & (hasTrack)) // is a right track already in that space
	 {
	    u32 index = rightFlags >> 2;
	    graph.adjList[item.index].e[1] = index;
	    graph.adjList[item.index].flags |= TrackAttribute::right;
	    graph.adjList[index].AddAncestor(item.index);
	    ++branches;
	 }
	 
	 graph.adjList[item.index].flags |= branches | TrackAttribute::branch;
	 if(!(item.rules & TrackOrder::noAncestor)) graph.adjList[item.index].AddAncestor(item.ancestor);

	 v2 position = VirtualToReal(item.x, item.y);
       
	 graph.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), &GlobalBranchCurve, BranchTrack);
	 graph.elements[item.index].flags = Track::branch | Track::left;	 
      }
      else // is linear
      {
	 u32 behindFlags = taken.get({item.x, item.y+1});
	 if((graph.capacity - firstFree) > 0 &&
	    !(behindFlags & hasTrack))
	 {
	    graph.adjList[item.index].flags = 1 | TrackAttribute::left; // size of 1, is linear so has a "left" track following
	    orders.Push({firstFree, item.index, item.x, item.y+1, 0});
	    taken.put({item.x, item.y+1}, (behindFlags & 0x3) | hasTrack | (firstFree << 2));
	    graph.adjList[item.index].e[0] = firstFree++; // When a Track is linear, the index of the next Track is e[0]
	 }
	 else if(behindFlags & hasTrack)
	 {
	    u32 index = behindFlags >> 2;
	    graph.adjList[item.index].e[0] = index;
	    graph.adjList[index].AddAncestor(item.index);
	 }
	 
	 v2 position = VirtualToReal(item.x, item.y);
	 graph.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve, LinearTrack);
	 graph.elements[item.index].flags = Track::staticMesh;	 

	 // when placing tracks after branches, if there is already
	 // a linear track behind the placed track, it will have to be manually
	 // connected here.p
	 u32 behind = taken.get({item.x, item.y-1});
	 if(behind)
	 {
	    u32 index = behind >> 2;

	    // if it is not a branch, and if it is not already connected
	    if(!(graph.adjList[index].flags & TrackAttribute::branch) && (graph.adjList[index].flags & TrackAttribute::edgeCountMask) == 0)
	    {
	       graph.adjList[index].flags |= 1 | TrackAttribute::left;
	       graph.adjList[index].e[0] = item.index;
	       graph.adjList[item.index].AddAncestor(index);
	    }	 
	 }

	 if(!(item.rules & TrackOrder::noAncestor)) graph.adjList[item.index].AddAncestor(item.ancestor);
      }      
   }

   graph.size = processed;
   CheckGraph(graph);

   // pop hashmap
   allocator->pop();

   //pop queue
   allocator->pop();
   
   END_TIME();

   trackGenTime = READ_TIME();
}

struct Player
{
   Object renderable;
   Track *currentTrack;
   i32 trackIndex;
   float t;
};

struct stbFont
{
   u8 *rawFile; // @: We might not have to keep this around.
   stbtt_fontinfo info;
   stbtt_packedchar *chars;
   u8 *map;
   u32 width;
   u32 height;
   GLuint textureHandle;
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

   TextProgram bitmapFontProgram;
   TextProgram fontProgram;

   stbFont bitmapFont;   

   enum
   {
      START,      
      INITGAME,
      RESET,
      LOOP,
   };

   i32 state;
};

static
ShaderProgram LoadFilesAndCreateProgram(char *vertex, char *fragment, StackAllocator *allocator)
{
   size_t vertSize = FileSize(vertex);
   size_t fragSize = FileSize(fragment);
   char *vertBuffer = (char *)allocator->push(vertSize);
   char *fragBuffer = (char *)allocator->push(fragSize);
   FileRead(vertex, (u8 *)vertBuffer, vertSize);
   FileRead(fragment, (u8 *)fragBuffer, fragSize);

   ShaderProgram result = CreateProgram(vertBuffer, vertSize, fragBuffer, fragSize);

   allocator->pop();
   allocator->pop();
   
   return result;
}

static
TextProgram LoadFilesAndCreateTextProgram(char *vertex, char *fragment, StackAllocator *allocator)
{
   size_t vertSize = FileSize(vertex);
   size_t fragSize = FileSize(fragment);
   char *vertBuffer = (char *)allocator->push(vertSize);
   char *fragBuffer = (char *)allocator->push(fragSize);
   FileRead(vertex, (u8 *)vertBuffer, vertSize);
   FileRead(fragment, (u8 *)fragBuffer, fragSize);

   TextProgram result = CreateTextProgram(vertBuffer, vertSize, fragBuffer, fragSize);

   allocator->pop();
   allocator->pop();

   return result;
}

// the globalName "LoadImage" is already taken
static 
Image LoadImageFile(char *filename, StackAllocator *allocator)
{
   size_t fileSize = FileSize(filename);
   // @leak
   u8 *buffer = allocator->push(fileSize);

   FileRead(filename, buffer, fileSize);

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
   size_t fileSize = FileSize(filename);

   u8 *buffer = allocator->push(fileSize);
   FileRead(filename, buffer, fileSize);

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

void UpdatePlayer(Player &player, TrackGraph &tracks, GameState &state)
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
	    //@TEMPORARY
	    state.state = GameState::RESET;
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
	    //@TEMPORARY
	    state.state = GameState::RESET;
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

   camera.view = CameraMatrix(camera);
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

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

GLuint UploadTexture(Image &image, i32 channels = 4)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   GLuint type;
   switch(channels)
   {
      case 1:
      {
	 type = GL_RED;
      }break;
      case 4:
      {
	 type = GL_RGBA;
      }break;
      default:
      {
	 assert(!"unsupported channel format");
	 type = 0; // shut the compiler up
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, type, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

static GLuint textVao;
static GLuint textUVVbo;
static GLuint textVbo;
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

   glGenBuffers(1, &textVbo);
   glBindBuffer(GL_ARRAY_BUFFER, textVbo);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * 12), 0, GL_STATIC_DRAW);
   glBindVertexArray(0);
}

static
stbFont InitFont_stb(char *fontFile, u32 width, u32 height, StackAllocator *allocator)
{
   stbFont result;

   result.width = width;
   result.height = height;

   size_t fileSize = FileSize(fontFile);

   result.rawFile = allocator->push(fileSize);
   FileRead(fontFile, result.rawFile, fileSize);
   
   if(!stbtt_InitFont(&result.info, result.rawFile, 0))
   {
      assert(false);
   }

   stbtt_pack_context pack;
   result.chars = (stbtt_packedchar *)allocator->push(sizeof(stbtt_packedchar) * 256);
   result.map = allocator->push(width * height);

   // @could we do this in the asset processor? and then use the stb_api to pull the quad from the generated texture?
   stbtt_PackBegin(&pack, result.map, width, height, width, 1, 0); // @should supply our own allocator instead of defaulting to malloc
   stbtt_PackSetOversampling(&pack, 4, 4);

   stbtt_PackFontRange(&pack, result.rawFile, 0, STBTT_POINT_SIZE(16.0f), 0, 256, result.chars);
   stbtt_PackEnd(&pack);

   glGenTextures(1, &result.textureHandle);
   glBindTexture(GL_TEXTURE_2D, result.textureHandle);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, result.map);
   glBindTexture(GL_TEXTURE_2D, 0);

   return result;
}

void GameInit(GameState &state)
{
   state.state = GameState::START;
   state.mainArena.base = AllocateSystemMemory(GIGABYTES(2), &state.mainArena.size);
   state.mainArena.current = state.mainArena.base;

   InitStackAllocator((StackAllocator *)state.mainArena.base);
   StackAllocator *stack = (StackAllocator *)state.mainArena.base;

   state.tracks = AllocateTrackGraph(1024, stack);

   assert(state.mainArena.base);

   srand((u32)__rdtsc());

   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   InitCamera(state.camera);

   DefaultShader = LoadFilesAndCreateProgram("assets\\default.vertp", "assets\\default.fragp",
					     stack);
   state.fontProgram = LoadFilesAndCreateTextProgram("assets\\text.vertp", "assets\\text.fragp",
						     stack);
   state.bitmapFontProgram = LoadFilesAndCreateTextProgram("assets\\bitmap_font.vertp", "assets\\bitmap_font.fragp",
							   stack);
   state.tempFontField = LoadImageFile("distance_field.bi", stack);   
   state.tempFontData = LoadFontFile("font_data.bf", stack);
   state.bitmapFont = InitFont_stb("c:/Windows/Fonts/arial.ttf", 1024, 1024,stack);
   
   state.fontTextureHandle = UploadTexture(state.tempFontField);
   state.keyState = up;
   Sphere = InitMeshObject("assets\\sphere.brian", stack);

   //@leak
   LinearTrack = AllocateMeshObject(80 * 3, stack);
   BranchTrack = AllocateMeshObject(80 * 3, stack);
   
   GlobalLinearCurve = LinearCurve(0, 0, 0, 1);
   GlobalBranchCurve = BranchCurve(0, 0,
				   -1, 1);

   GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);
   GenerateTrackSegmentVertices(LinearTrack, GlobalLinearCurve);

   RectangleUVBuffer = UploadVertices(RectangleUVs, 6, 2);
   RectangleVertBuffer = UploadVertices(RectangleVerts, 6, 2);
   ScreenVertBuffer = UploadVertices(ScreenVerts, 6, 2);

   InitTextBuffers();
   
   state.lightPos = state.camera.position;
   
   state.sphereGuy.renderable = SpherePrimitive(V3(0.0f, -2.0f, 5.0f),
						V3(1.0f, 1.0f, 1.0f),
						Quat(1.0f, 0.0f, 0.0f, 0.0f));

   state.sphereGuy.t = 0.0f;   
   state.sphereGuy.trackIndex = 0;

   ReInitTrackGraph(state.tracks, stack);
}

static inline
m3 TextProjection(float screenWidth, float screenHeight)
{
   return {2.0f / screenWidth, 0.0f, 0.0f,
	 0.0f, 2.0f / screenHeight, 0.0f,
	 -1.0f, -1.0f, 1.0f};
}

static
void RenderText_stb(char *string, float x, float y, stbFont &font, TextProgram &p)
{
   // convert clip coords to device coords
   x = ((x + 1.0f) * 0.5f) * (float)SCREEN_WIDTH;
   y = ((y + 1.0f) * 0.5f) * (float)SCREEN_HEIGHT;
   
   glDisable(GL_DEPTH_TEST);

   glBindVertexArray(textVao);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   glUseProgram(p.programHandle);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, font.textureHandle);
   glUniform1i(p.texUniform, 0);

   glUniformMatrix3fv(p.transformUniform, 1, GL_FALSE, TextProjection(SCREEN_WIDTH, SCREEN_HEIGHT).e);

   stbtt_aligned_quad quad;
   
   for(i32 i = 0; string[i]; ++i)
   {
      char c = string[i];
      stbtt_GetPackedQuad(font.chars, font.width, font.height, c, &x, &y, &quad, 0);

      float uvs[] =
      {
	 quad.s0, quad.t1,
	 quad.s1, quad.t1,
	 quad.s0, quad.t0,

	 quad.s1, quad.t1,
	 quad.s1, quad.t0,
	 quad.s0, quad.t0,
      };
      
      float verts[] =
      {
	 quad.x0, quad.y0,
	 quad.x1, quad.y0,
	 quad.x0, quad.y1,

	 quad.x1, quad.y0,
	 quad.x1, quad.y1,
	 quad.x0, quad.y1,
      };

      glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 12, uvs);

      glBindBuffer(GL_ARRAY_BUFFER, textVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 12, verts);

      glDrawArrays(GL_TRIANGLES, 0, 6);
   }

   glBindVertexArray(0);
   glEnable(GL_DEPTH_TEST);
}

static
void DistanceRenderText(char *string, u32 length, float xpos, float ypos, float scale, FontData &font,
		TextProgram p, GLuint textureMap)
{

   xpos = ((xpos + 1.0f) * 0.5f) * (float)SCREEN_WIDTH;
   ypos = ((ypos + 1.0f) * 0.5f) * (float)SCREEN_HEIGHT;

   glDisable(GL_DEPTH_TEST);      

   float mapWidth = (float)font.mapWidth;
   float mapHeight = (float)font.mapHeight;

   glBindVertexArray(textVao);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   glUseProgram(p.programHandle);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureMap);
   glUniform1i(p.texUniform, 0);

   glUniformMatrix3fv(p.transformUniform, 1, GL_FALSE, TextProjection(SCREEN_WIDTH, SCREEN_HEIGHT).e);
   
   if(length == 0)
   {
      char *c = string;
      while(*(c++)) ++length;
   }

   for(u32 i = 0; i < length; ++i)
   {
      char c = string[i];
      CharInfo params = {};
      for(u32 j = 0; j < font.count; ++j)
      {
	 if(font.data[j].id == c)
	 {
	    params = font.data[j];
	    break;
	 }

	 assert(j != font.count-1);
      }

      //@ 0.001 to prevent texture bleeding
      // very noticable with the character 't'
      float x = ((float)params.x / mapWidth) + 0.001f;
      float y = (float)params.y / mapHeight + 0.001f;
      float width = ((float)params.width / mapWidth) - 0.002f;
      float height = (float)params.height / mapHeight - 0.002f;

      float uvs[] =
      {
	 x, y + height,
	 x + width, y + height,
	 x, y,

	 x + width, y + height,
	 x + width, y,
	 x, y,
      };

      glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 12, uvs);

      width = (float)params.width * scale;
      height = (float)params.height * scale;
      float xoffset = (float)params.xoffset * scale;
      float yoffset = ((float)params.yoffset * scale) - height;

      float xpen = (xpos + xoffset);
      float ypen = (ypos + yoffset);

      float verts[] =
      {
	 xpen, ypen,
	 xpen + width, ypen,
	 xpen, ypen + height,

	 xpen + width, ypen,
	 xpen + width, ypen + height,
	 xpen, ypen + height,
      };

      glBindBuffer(GL_ARRAY_BUFFER, textVbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 12, verts);

      float xadvance = (float)params.xadvance;
      xpos += xadvance * scale;
            
      glDrawArrays(GL_TRIANGLES, 0, 6);
   }

   glBindVertexArray(0);
   glEnable(GL_DEPTH_TEST);
}

template <typename int_type>
static inline void IntToString(char *dest, int_type num)
{
   static char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

   if(num == 0)
   {
      dest[0] = '0';
      dest[1] = '\0';
      return;
   }
   
   int_type count = 0;
   for(char *c = dest; num; ++c)
   {
      int_type digit = num % 10;
      dest[count] = table[digit];
      ++count;
      num /= 10;
   }

   for(int_type i = 0; i < (count / 2); ++i)
   {
      SWAP(char, dest[i], dest[count - i - 1]);
   }

   dest[count] = '\0';
}

i32 RenderTracks(GameState &state)
{
   i32 rendered = 0;
   for(i32 i = 0; i < state.tracks.size; ++i)
   {	    
      if(!(state.tracks.adjList[i].flags & TrackAttribute::invisible))
      {	 
	 if(state.tracks.adjList[i].flags & TrackAttribute::reachable)
	 {
	    RenderPushObject(state.tracks.elements[i].renderable, state.camera.view, state.lightPos, V3(1.0f, 0.0f, 0.0f));
	 }
	 else
	 {
	    RenderPushObject(state.tracks.elements[i].renderable, state.camera.view, state.lightPos, V3(0.0f, 0.0f, 1.0f));
	 }
	 ++rendered;
      }
   }

   return rendered;
}

#if 1
static
void SetReachable(TrackAttribute *attributes, i32 start, StackAllocator *allocator)
{
   u32 *stack = (u32 *)allocator->push(1024);
   u32 top = 1;

   stack[0] = start;

   while(top > 0)
   {
      u32 index = stack[--top];

      if(!(attributes[index].flags & TrackAttribute::reachable))
      {
	 attributes[index].flags |= TrackAttribute::reachable;

	 if(attributes[index].hasLeft())
	 {
	    stack[top++] = attributes[index].getLeft();
	 }

	 if(attributes[index].hasRight())
	 {
	    stack[top++] = attributes[index].getRight();
	 }
      }
   }

   allocator->pop();
}
#else
static
   void SetReachable(TrackAttribute *attributes, i32 track, StackAllocator *allocator)
{
   if(!(attributes[track].flags & TrackAttribute::reachable))
   {
      attributes[track].flags |= TrackAttribute::reachable

      if(attributes[track].hasLeft())
      {
	 SetReachable(attributes, attributes[track].getLeft(), allocator);
      }

      if(attributes[track].hasRight())
      {
	 SetReachable(attributes, attributes[track].getRight(), allocator);
      }
   }
}
#endif

#define NotReachableVisible(flags) (flags & TrackAttribute::invisible) && !(flags & TrackAttribute::reachable)

void SortTracks(TrackGraph *tracks)
{
   u32 b = tracks->size-1;

   for(u32 a = 0; a < (u32)tracks->size; ++a)
   {
      if(NotReachableVisible(tracks->adjList[a].flags))
      {
	 while(NotReachableVisible(tracks->adjList[b].flags))
	 {
	    tracks->RemoveTrack(b);
	    --b;	    
	    
	    if(a > b)
	    {
	       return;
	    }
	 }

	 tracks->RemoveTrack(a);

	 if(tracks->adjList[b].hasLeft())
	 {
	    u32 left = tracks->adjList[b].getLeft();

	    tracks->adjList[left].ReplaceAncestor(b, a);
	 }
	 
	 if(tracks->adjList[b].hasRight())
	 {
	    u32 right = tracks->adjList[b].getRight();

	    tracks->adjList[right].ReplaceAncestor(b, a);
	 }
	 
	 for(u32 i = 0; i < tracks->adjList[b].ancestorCount(); ++i)
	 {
	    u32 ancestor = tracks->adjList[b].a[i];

	    tracks->adjList[ancestor].ReplaceEdge(b, a);
	 }

	 tracks->adjList[a] = tracks->adjList[b];
	 tracks->elements[a] = tracks->elements[b];
	 
	 --b;
	 // --tracks->size;
      }
   }

   // ensure graph was formed properly
   for(u32 i = 0; i < (u32)tracks->size; ++i)
   {
      for(u32 j = 0; j < tracks->adjList[i].ancestorCount(); ++j)
      {
	 u32 ancestor = tracks->adjList[i].a[j];

	 if(tracks->adjList[ancestor].hasLeft())
	 {
	    if(i == tracks->adjList[ancestor].getLeft()) continue;	    
	 }
	 if(tracks->adjList[ancestor].hasRight())
	 {
	    if(i == tracks->adjList[ancestor].getRight()) continue;
	 }
	 
	 {
	    assert(false);
	 }
      }
   }
}

void UpdateTracks(TrackGraph *tracks, Player *player, StackAllocator *allocator)
{
   // reset reachable flag
   for(i32 i = 0; i < tracks->size; ++i)
   {
      tracks->adjList[i].flags &= ~TrackAttribute::reachable;
   }
   
   SetReachable(tracks->adjList, player->trackIndex, allocator);

   float cutoff = player->renderable.worldPos.y - (TRACK_SEGMENT_SIZE * 2.0f);

   for(i32 i = 0; i < tracks->size; ++i)
   {
      if(tracks->elements[i].renderable.worldPos.y < cutoff)
      {
	 tracks->adjList[i].flags |= TrackAttribute::invisible;
      }
   }

   SortTracks(tracks);
   // if a track is invisible and unreachable,
   // then we want to remove it from the graph
}

void GameLoop(GameState &state)
{
   switch(state.state)
  { 
      case GameState::LOOP:
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

	 UpdateTracks(&state.tracks, &state.sphereGuy, (StackAllocator *)state.mainArena.base);
	 UpdatePlayer(state.sphereGuy, state.tracks, state);
	 UpdateCamera(state.camera, state.sphereGuy, state.tracks);	 
	 state.lightPos = state.camera.position;
   
	 RenderPushObject(state.sphereGuy.renderable, state.camera.view, state.lightPos, V3(1.0f, 0.0f, 0.0f));

	 i32 rendered = RenderTracks(state);

	 static char onScreen[8];
	 IntToString(onScreen, rendered);
	 DistanceRenderText(onScreen, 0, -0.8f, 0.6f, 0.1f, state.tempFontData, state.fontProgram, state.fontTextureHandle);

	 static char trackTime[19];
	 IntToString(trackTime, trackGenTime);
	 DistanceRenderText(trackTime, 0, -0.8f, 0.4f, 0.1f, state.tempFontData, state.fontProgram, state.fontTextureHandle);

	 static char framerate[8];
	 static float time = 120.0f;
	 time += delta;
	 if(time > 30.0f)
	 {
	    time = 0.0f;
	    IntToString(framerate, (i32)((1.0f / delta) * 60.0f));
	 }

	 RenderText_stb(framerate, 0.0f, 0.0f, state.bitmapFont, state.bitmapFontProgram);
      }break;

      case GameState::RESET:
      {	 
	 state.state = GameState::START;
	 ReInitTrackGraph(state.tracks, (StackAllocator *)state.mainArena.base);
      }break;

      case GameState::START:
      {
	 static float position = 0.0f;
	 position += delta * 0.01f;	 

	 if(GlobalActionPressed)
	 {
	    state.state = GameState::LOOP;	    
	    state.sphereGuy.currentTrack = &state.tracks.elements[state.tracks.head];
	    state.sphereGuy.trackIndex = 0;
	    state.sphereGuy.t = 0.0f;

	    GlobalBranchCurve = BranchCurve(0, 0,
					    -1, 1);

	    GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);
	 }

	 RenderTracks(state);

	 DistanceRenderText("Butts", 0, (sinf(position) * 0.6f) - 0.35f, 0.0f, 0.5f, state.tempFontData, state.fontProgram, state.fontTextureHandle);
      }break;      
      
      case GameState::INITGAME:
      {
      }break;
      
      default:
      {
	 assert(!"invalid state");
      }
   }
}

void OnKeyDown(GameState &state)
{
   GlobalActionPressed = 1;

   if(state.state == GameState::LOOP)
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
}

void OnKeyUp(GameState &state)
{
   GlobalActionPressed = 0;
}
   
void GameEnd(GameState &state)
{
   FreeSystemMemory(state.mainArena.base);

   glDeleteBuffers(1, &textUVVbo);
   glDeleteBuffers(1, &textVbo);
   glDeleteBuffers(1, &RectangleUVBuffer);
   glDeleteBuffers(1, &RectangleVertBuffer);
   glDeleteBuffers(1, &ScreenVertBuffer);
   glDeleteBuffers(1, &Sphere.handles.vbo);
   glDeleteBuffers(1, &Sphere.handles.nbo);
   glDeleteBuffers(1, &LinearTrack.handles.vbo);
   glDeleteBuffers(1, &BranchTrack.handles.nbo);
   glDeleteBuffers(1, &LinearTrack.handles.nbo);
   glDeleteBuffers(1, &BranchTrack.handles.vbo);

   glDeleteTextures(1, &state.fontTextureHandle);
}
