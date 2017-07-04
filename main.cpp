
#define assert(x) ((x) ? 0 : *((char *)0) = 'x')

#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

#define LEVEL_LENGTH 0.5f

static float delta;
static m4 Projection = Projection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 100.0f, 60.0f);

struct Camera
{
   quat orientation;
   v3 position;
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
   
   GLint vertexAttrib;
   GLint normalAttrib;
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

//@ need to do this so that we can keep a consistent candy cane shape
//  pattern on different sized lines
void GenerateUVs(v2 *vertices, u32 count, v2 *uvBuffer, float length)
{
   for(u32 i = 0; i < count; ++i)
   {
      if(vertices[i].x < 0.0f)
      {
	 uvBuffer[i].x = 0.0f;

	 if(vertices[i].y < 0.0f)
	 {
	    uvBuffer[i].y = 4.0f * length;
	 }
	 else
	 {
	    uvBuffer[i].y = 0.0f;
	 }
      }
      else
      {
	 uvBuffer[i].x = 1.0f;

	 if(vertices[i].y < 0.0f)
	 {
	    uvBuffer[i].y = (4.0f * length) - 0.1f;
	 }
	 else
	 {
	    uvBuffer[i].y = -0.1f;
	 }
      }
   }
}

static inline
m3 RotationAboutZ(float rads)
{
   m3 result = {};

   result.e2[0][0] = cosf(rads);
   result.e2[0][1] = -sinf(rads);
   result.e2[1][0] = sinf(rads);
   result.e2[1][1] = cosf(rads);
   result.e2[2][2] = 1.0f;

   return result;
}

static inline
m3 Scale3(float x, float y, float z)
{
   m3 result = {};

   result.e2[0][0] = x;
   result.e2[1][1] = y;
   result.e2[2][2] = z;

   return result;
}

static inline
m3 Translate(float x, float y)
{
   m3 result = {};

   result.e2[2][0] = x;
   result.e2[2][1] = y;

   result.e2[0][0] = 1.0f;
   result.e2[1][1] = 1.0f;
   result.e2[2][2] = 1.0f;
   return result;
}

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

static inline
v2 CubicBezier(v2 p1, v2 p2, v2 p3, v2 p4, float t)
{
   // uses the quicker non matrix form of the equation
   v2 result;

   static float test = t;

   float degree1 = 1 - t;   
   float degree2 = degree1 * degree1; // (1 - t)^2
   float degree3 = degree2 * degree1; // (1 - t)^3
   float squared = t * t;   
   float cubed = squared * t;   

   result.x = degree3 * p1.x + 3 * t * degree2 * p2.x + 3 * squared * degree1 * p3.x + cubed * p4.x;
   result.y = degree3 * p1.y + 3 * t * degree2 * p2.y + 3 * squared * degree1 * p3.y + cubed * p4.y;

   return result;
}

static inline
v2 CubicBezier(Curve c, float t)
{
   return CubicBezier(c.p1, c.p2, c.p3, c.p4, t);
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
MeshBuffers
AllocateMeshBuffers(i32 count, i32 components = 3)
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

void RenderPushMesh(ShaderProgram p, MeshObject b, m4 &transform, m4 &view, v3 lightPos)
{
   glUseProgram(p.programHandle);

   static float time = 0.0f;
   time += delta;

   m4 lightRotation = M4(Rotation(V3(0.0f, 1.0f, 0.0f), -time / 50.0f));

   m4 projection = Projection;
   m4 mvp = projection * view * transform;
   
   glUniformMatrix4fv(p.MVPUniform, 1, GL_FALSE, mvp.e);
   glUniformMatrix4fv(p.modelUniform, 1, GL_FALSE, transform.e);
   glUniformMatrix4fv(p.viewUniform, 1, GL_FALSE, view.e);
   glUniform3fv(p.lightPosUniform, 1, lightPos.e);
   glBindVertexArray(b.handles.vao);
   glDrawArrays(GL_TRIANGLES, 0, b.mesh.vcount);
   glBindVertexArray(0);
   glUseProgram(0);
}

static
void RenderPushObject(Object &obj, m4 &camera, v3 lightPos)
{
   m4 orientation = M4(obj.orientation);
   m4 scale = Scale(obj.scale);
   m4 translation = Translate(obj.worldPos);
   
   m4 transform = translation * scale * orientation;
   
   RenderPushMesh(*obj.p, obj.meshBuffers, transform, camera, lightPos);
}

enum KeyState
{
   down,
   up,
};

struct Collectibles
{
   // Right now, this array is limited to 10 collectibles.
   Object *c;
   i32 size;

   void put(Object o);
   Object get(i32 i);
   void remove(i32 i);
   Collectibles();
};

Collectibles::Collectibles()
{
   c = (Object *)malloc(sizeof(Object) * 10);
   size = 0;
}

void
Collectibles::put(Object o)
{
   assert(size <= 10);

   c[size++] = o;   
}

Object
Collectibles::get(i32 i)
{
   return c[i];
}

void
Collectibles::remove(i32 i)
{
   assert(i < size && i < 10);

   for(int j = i; j < size-1; ++j)
   {
      c[j] = c[j + 1];
   }

   --size;
}

struct Track
{
   enum
   {
      staticMesh = 0x1,
      branch = 0x2,
      left = 0x8, // else, right
      lerping = 0x10
   };

   Object renderable;
   Curve bezier;
   i32 flags;

   Curve beginLerp;
   Curve endLerp;
   float t;
};

static inline
Track CreateTrack(v3 position, v3 scale, Curve bezier, MeshObject &buffers)
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
      return (flags & countMask) == 2;
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
   };

   Track *elements;
   TrackAttribute *adjList;
   i32 size;
   i32 head;
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

struct TrackInfo
{
   u32 index;
   i32 x, y;
};

struct VirtualTrackCoords
{
   i32 x, y;
};

#if 0
static
Track *TestInitTracks()
{
   Track *vertices = (Track *)malloc(sizeof(Track) * 20);

   Curve line;
   line.p1 = V2(0.0f, -0.5f);
   line.p2 = V2(0.0f, -0.5f);
   line.p3 = V2(0.0f, 0.5f);
   line.p4 = V2(0.0f, 0.5f);
   
   for(i32 i = 0; i < 20; ++i)
   {
      vertices[i] = CreateTrack(V3(0.0f, (float)i * 5.0f, 0.0f), V3(0.5f, 5.0f, 0.5f),
				line, LinearTrack);
   }

   return vertices;
}
#endif

static inline
v2 VirtualToReal(i32 x, i32 y)
{
   return V2((float)x * 4.0f, (float)y * 4.0f);
}

static inline
Curve ConnectionsToCurve(i32 x1, i32 y1,
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

static
TrackGraph InitTrackGraph(i32 initialSize)
{
   TrackGraph result;

   result.head = 0;   
   result.flags = TrackGraph::left;

   // @leak
   result.elements = (Track *)malloc(sizeof(Track) * initialSize);
   // @leak
   result.adjList = (TrackAttribute *)malloc(sizeof(TrackAttribute) * initialSize);

   static u8 taken[20][20];
   for(u8 i = 0; i < 20; ++i)
   {
      for(u8 j = 0; j < 20; ++j)
      {
	 taken[i][j] = 0;
      }
   }

   enum
   {
      hasTrack = 0x1,
      hasBranch = 0x2,
   };

   result.size = 20;
   CircularQueue<TrackInfo> vertices(initialSize);
   
   vertices.Push({0, 0, 0}); // Push root segment.
   u32 firstFree = 1; // next element that can be added to the queue
   while(vertices.size)
   {
      TrackInfo item = vertices.Pop();
      if(rand() % 4 == 0) // is a branch
      {	
	 int branches = 0;      	 	 
	 
	 // Queue up subsequent branches
	 if(initialSize - firstFree > 0 && !(taken[item.x-1][item.y+1] & hasTrack))
	 {
	    ++branches;
	    vertices.Push({firstFree, item.x - 1, item.y + 1}); // left side

	    taken[item.x-1][item.y+1] |= hasTrack | (firstFree >> 2);
	    result.adjList[item.index].e[0] = firstFree++;	    
	 }
	 else if(taken[item.x-1][item.y+1] & hasTrack)
	 {
	    result.adjList[item.index].e[0] = (u32)taken[item.x-1][item.y+1] << 2;
	    ++branches;
	 }

	 if(initialSize - firstFree > 0 && !(taken[item.x+1][item.y+1] & hasTrack))
	 {
	    ++branches;	    
	    vertices.Push({firstFree, item.x + 1, item.y + 1}); // right side

	    taken[item.x+1][item.y+1] |= hasTrack | (firstFree >> 2);
	    result.adjList[item.index].e[1] = firstFree++;	    
	 }
	 else if(taken[item.x+1][item.y+1] & hasTrack)
	 {
	    result.adjList[item.index].e[1] = (u32)taken[item.x+1][item.y+1] << 2;
	    ++branches;
	 }
	 
	 result.adjList[item.index].flags = branches | TrackAttribute::branch | TrackAttribute::left;

	 Curve leftCurve = ConnectionsToCurve(item.x, item.y,
					      item.x - 1, item.y + 1);


	 MeshObject dynamic = AllocateMeshObject(80 * 3);

	 v2 position = VirtualToReal(item.x, item.y);

	 result.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), leftCurve, dynamic);
	 result.elements[item.index].flags = Track::branch | Track::left;	 
      }
      else // is linear
      {	
	 if((initialSize - firstFree) > 0 && !(taken[item.x][item.y+1] & hasTrack))
	 {
	    result.adjList[item.index].flags = 1 | TrackAttribute::left; // size of 1, is linear so has a "left" track following
	    vertices.Push({firstFree, item.x, item.y + 1});
	    taken[item.x][item.y+1] |= hasTrack | (firstFree >> 2);
	    result.adjList[item.index].e[0] = firstFree++; // When a Track is linear, the index of the next Track is e[0]
	 }
	 else if(taken[item.x][item.y+1] & hasTrack)
	 {
	    result.adjList[item.index].e[0] = taken[item.x][item.y+1] << 2;
	 }
	 else
	 {
	    result.adjList[item.index].flags = 0;
	 }

	 Curve linear = ConnectionsToCurve(item.x, item.y,
					   item.x, item.y + 1);

	 v2 position = VirtualToReal(item.x, item.y);
	 result.elements[item.index] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f), linear, LinearTrack);
	 result.elements[item.index].flags = Track::staticMesh;
      }      
   }

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
   Collectibles collectibles;
   KeyState keyState;
   TrackGraph tracks;

   v3 lightPos;
};

ShaderProgram
LoadFilesAndCreateProgram(char *vertex, char *fragment)
{
   size_t vertSize = WinFileSize(vertex);
   size_t fragSize = WinFileSize(fragment);
   char *vertBuffer = (char *)malloc(vertSize);
   char *fragBuffer = (char *)malloc(fragSize);
   WinReadFile(vertex, (u8 *)vertBuffer, vertSize);
   WinReadFile(fragment, (u8 *)fragBuffer, fragSize);

   ShaderProgram result = CreateProgram(vertBuffer, vertSize, fragBuffer, fragSize);

   free(vertBuffer);
   free(fragBuffer);
   
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

static inline
v3 GetPositionOnTrack(Track &track, float t)
{   
   v2 displacement = CubicBezier(track.bezier, t);
   return V3(displacement.x + track.renderable.worldPos.x,
	     displacement.y + track.renderable.worldPos.y,
	     track.renderable.worldPos.z);
}

void UpdatePlayer(Player &player, TrackGraph &tracks)
{
   player.t += 0.01f * delta;
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

void UpdateCamera(Camera &camera, v3 playerPosition)
{
   // have camera follow player
   camera.position.y = playerPosition.y - 5.0f;
   camera.position.x = playerPosition.x;
   // camera.position.y += delta * 0.1f;
}

void CreateCollectibles(Collectibles &collectibles)
{
   collectibles.c[0] = SpherePrimitive(V3(0.0f, 5.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(0.0f, 0.0f, 0.0f, 0.0f));

   collectibles.c[1] = SpherePrimitive(V3(0.0f, 10.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(0.0f, 0.0f, 0.0f, 0.0f));

   collectibles.c[2] = SpherePrimitive(V3(0.0f, 15.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(0.0f, 0.0f, 0.0f, 0.0f));

   collectibles.c[3] = SpherePrimitive(V3(0.0f, 20.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(0.0f, 0.0f, 0.0f, 0.0f));

   collectibles.c[4] = SpherePrimitive(V3(0.0f, 25.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(0.0f, 0.0f, 0.0f, 0.0f));

   collectibles.size = 5;   
}

// approximate tangent
// @ should replace with derivative form
static inline
v2 Tangent(Curve c, float t)
{
   float begin = min(0.0f, t - 0.000001f);
   float end = max(1.0f, t + 0.000001f);

   v2 a = CubicBezier(c, begin);
   v2 b = CubicBezier(c, end);

   return unit(b - a);
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
   GenerateTrackSegmentVertices(track.renderable.meshBuffers, track.bezier);   
}

void GameInit(GameState &state)
{
   srand((u32)__rdtsc());

   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   

   DefaultShader = LoadFilesAndCreateProgram("assets\\default.vertp", "assets\\default.fragp");
   state.keyState = up;

   Sphere = InitMeshObject("assets\\sphere.brian");
   LinearTrack = AllocateMeshObject(80 * 3);
   
   Curve line = ConnectionsToCurve(0, 0, 0, 1);
   
   v2 val = CubicBezier(line, 0.0f);
   float y = val.y;
   float difference = 0.0f;
   for(i32 i = 1; i < 11; ++i)
   {
      float t = (float)i * 0.1f;

      val = CubicBezier(line, t);
      difference = val.y - y;
      y = val.y;
   }

   GenerateTrackSegmentVertices(LinearTrack, line);

   state.camera.position = V3(0.0f, 0.0f, 10.0f);
   state.camera.orientation = Rotation(V3(1.0f, 0.0f, 0.0f), 1.0f);
   state.lightPos = state.camera.position;

   state.tracks = InitTrackGraph(20);
   
   state.sphereGuy.renderable = SpherePrimitive(V3(0.0f, -2.0f, 5.0f),
						V3(1.0f, 1.0f, 1.0f),
						Quat(1.0f, 0.0f, 0.0f, 0.0f));

   state.sphereGuy.t = 0.0f;
   state.sphereGuy.currentTrack = &state.tracks.elements[state.tracks.head];
   state.sphereGuy.trackIndex = 0;

   Curve curve;
   curve.p1 = V2(0.0f, -0.5f);
   curve.p2 = V2(2.0f, -0.3f);
   curve.p3 = V2(-2.0f, 0.3f);
   curve.p4 = V2(0.0f, 0.5f);

   //state.collectibles = Collectibles();
   //CreateCollectibles(state.collectibles);   

   MeshObject CurvyTrack = AllocateMeshObject(80 * 3);
   // state.testTrack = CreateTrack(V3(0.0f, 0.0f, 0.0f), V3(0.5f, 2.0f, 0.5f), curve, CurvyTrack);
   //GenerateTrackSegmentVertices(state.testTrack);
   
   for(i32 i = 0; i < state.tracks.size; ++i)
   {
      if(!(state.tracks.elements[i].flags & Track::staticMesh))
      {
	 GenerateTrackSegmentVertices(state.tracks.elements[i]);
      }      
   }   

   // state.testTracks = TestInitTracks();
}

void GameLoop(GameState &state)
{

   Track *tracks = state.tracks.elements;
   for(i32 i = 0; i < state.tracks.size; ++i)
   {
      if(tracks[i].flags & Track::lerping)
      {
	 tracks[i].t = min(tracks[i].t + (0.1f * delta), 1.0f);
	 tracks[i].bezier = lerp(tracks[i].beginLerp, tracks[i].endLerp, tracks[i].t);

	 GenerateTrackSegmentVertices(tracks[i]);

	 if(tracks[i].t == 1.0f)
	 {
	    tracks[i].flags &= ~Track::lerping;
	 }
      }
   }

   m4 cameraTransform = CameraMatrix(state.camera);

   UpdatePlayer(state.sphereGuy, state.tracks);
   UpdateCamera(state.camera, state.sphereGuy.renderable.worldPos);
   state.lightPos = state.camera.position;
   
   RenderPushObject(state.sphereGuy.renderable, cameraTransform, state.lightPos);
   /*
   for(i32 i = 0; i < state.collectibles.size; ++i)
   {
      RenderPushObject(state.collectibles.c[i], cameraTransform, state.lightPos); 
   }
   */

   
   for(i32 i = 0; i < state.tracks.size; ++i)
   {
      RenderPushObject(state.tracks.elements[i].renderable, cameraTransform, state.lightPos);
   }
   

   /*
   static float rot = 0.0f;
   state.testTrack.renderable.orientation = Rotation(V3(0.0f, 1.0f, 0.0f), rot);
   rot += delta / 100.0f;
   RenderPushObject(state.testTrack.renderable, cameraTransform, state.lightPos);
   */   
}

void OnKeyDown(GameState &state)
{
   Track *tracks = state.tracks.elements;

   state.tracks.flags ^= TrackGraph::left;

   for(i32 i = 0; i < state.tracks.size; ++i)
   {
      // if already lerping
      if((tracks[i].flags & (Track::branch | Track::lerping)) == (Track::branch | Track::lerping))
      {
	 Curve temp = tracks[i].beginLerp;
	 tracks[i].beginLerp = tracks[i].endLerp;
	 tracks[i].endLerp = temp;
	 tracks[i].t = 1.0f - tracks[i].t;
      }
      // else if branch but not already lerping
      else if(tracks[i].flags & Track::branch)
      {
	 tracks[i].flags |= Track::lerping;

	 tracks[i].beginLerp = tracks[i].bezier;
	 tracks[i].endLerp = InvertX(tracks[i].bezier);
	 tracks[i].t = 0.0f;
      }
   }
}
