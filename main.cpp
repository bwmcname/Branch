
#define assert(x) ((x) ? 0 : *((char *)0) = 'x')

#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

#define LEVEL_LENGTH 0.5f

float delta;
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

   return translation * rotation;
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
static MeshObject TestTrack;

struct StaticLineBuffers
{
   Mesh mesh;
   MeshBuffers gpubuffers;

   GLint transform_uniform;
   GLint texture_uniform;
   GLint length_uniform;
   GLuint texture_handle;
};

StaticLineBuffers GlobalLine;

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
   MeshObject *meshBuffers;
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

#if 0
void InitLineBuffers(ShaderProgram &program)
{
   static float vertices[] = {
      -0.5f, -0.5f,
      -0.5f, 0.5f,
      0.5f, 0.5f,

      -0.5f, -0.5f,
      0.5f, 0.5f,
      0.5f, -0.5f
   };   

   static u32 LineTexture[] = {
      0xFF00FFFF, // yellow
      0xFF333333  // gray
   };

   GlobalLine.gpubuffers = UploadStaticMesh(vertices, 6);

   GlobalLine.mesh.vertices = vertices;
   GlobalLine.mesh.vcount = 6;   

   GlobalLine.transform_uniform = glGetUniformLocation(program.programHandle, "transform");
   GlobalLine.length_uniform = glGetUniformLocation(program.programHandle, "length");
   GlobalLine.texture_uniform = glGetUniformLocation(program.programHandle, "tex");

   glActiveTexture(GL_TEXTURE0);
   glGenTextures(1, &GlobalLine.texture_handle);
   glBindTexture(GL_TEXTURE_2D, GlobalLine.texture_handle);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA,
		1,
		2,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		LineTexture);

   
   glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

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
MeshObject AllocatMeshObject(i32 vertexCount)
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
   
   RenderPushMesh(*obj.p, *obj.meshBuffers, transform, camera, lightPos);
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
   Object renderable;
   Curve bezier;
};

struct GameState
{   
   Camera camera;
   Object sphereGuy;
   Track testTrack; //@DELETE
   Collectibles collectibles;
   KeyState keyState;

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
   result.meshBuffers = &Sphere;
   result.p = &DefaultShader;
   return result;
}

static inline
Track CreateTrack(v3 position, v3 scale, Curve bezier)
{
   Object obj;
   obj.worldPos = position;
   obj.scale = scale;
   obj.orientation = Quat(0.0f, 0.0f, 0.0f, 0.0f);
   obj.meshBuffers = &TestTrack;
   obj.p = &DefaultShader;

   Track result;
   result.renderable = obj;
   result.bezier = bezier;

   return result;
}

void UpdatePlayer(Object &player, Collectibles &collectibles)
{
   player.worldPos.y += delta * 0.01f;

   for(i32 i = 0; i < collectibles.size; ++i)
   {
      Object &other = collectibles.c[i];

      // this only works if scale.x == scale.y == scale.z
      float radiusOther = other.scale.x;
      float radiusThis = player.scale.x;
      float distance = fabsf(magnitude(player.worldPos - other.worldPos));

      if(distance < radiusOther + radiusThis)
      {
	 collectibles.remove(i);
	 --i;
      }
   }
}

void UpdateCamera(Camera &camera, v3 playerPosition)
{
   // have camera follow player
   camera.position.y = playerPosition.y;
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

void GenerateTrackSegmentVertices(Track track)
{
   // 10 segments, 8 tris per segment --- 80 tries
   
   tri *tris = (tri *)track.renderable.meshBuffers->mesh.vertices; //lol

   v2 sample = CubicBezier(track.bezier, 0.0f);

   v3 top1 = V3(sample.x, sample.y, 0.5f);
   v3 right1 = V3(sample.x + 0.5f, sample.y, 0.0f);
   v3 bottom1 = V3(sample.x, sample.y, -0.5f);
   v3 left1 = V3(sample.x - 0.5f, sample.y, 0.0f);
   
   float t = 0.0f;
   for(i32 i = 0; i < 10; ++i)
   {
      t += 0.1f;
      sample = CubicBezier(track.bezier, t);

      v3 top2 = V3(sample.x, sample.y, 0.5f);
      v3 right2 = V3(sample.x + 0.5f, sample.y, 0.0f);
      v3 bottom2 = V3(sample.x, sample.y, -0.5f);
      v3 left2 = V3(sample.x - 0.5f, sample.y, 0.0f);

      i32 j = i * 8;

      // top left side
      tris[j] = Tri(top1, left2, left1);
      tris[j+1] = Tri(top1, top2, left2);

      // top right side
      tris[j+2] = Tri(top1, right1, top2);
      tris[j+3] = Tri(top2, right1, right2);

      // bottom right side
      tris[j+4] = Tri(bottom1, bottom2, right1);
      tris[j+5] = Tri(bottom2, right2, right1);

      // bottom left side
      tris[j+6] = Tri(bottom1, left1, left2);
      tris[j+7] = Tri(bottom1, left2, bottom2);

      top1 = top2;
      right1 = right2;
      left1 = left2;
      bottom1 = bottom2;
   }

   v3 *normals = track.renderable.meshBuffers->mesh.normals;
   Normals((float *)tris, (v3 *)normals, 80 * 3); // 3 vertices per tri

   glBindBuffer(GL_ARRAY_BUFFER, track.renderable.meshBuffers->handles.vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, (sizeof(tri) * 80), (void *)tris);
   glBindBuffer(GL_ARRAY_BUFFER, track.renderable.meshBuffers->handles.nbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri) * 80, (void *)normals);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   
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
   TestTrack = AllocatMeshObject(80 * 3); // Just a test @DELETE

   state.camera.position = V3(0.0f, 0.0f, 10.0f);
   state.camera.orientation = Quat(0.0f, 0.0f, 0.0f, 0.0f);
   state.lightPos = state.camera.position;
   
   state.sphereGuy = SpherePrimitive(V3(0.0f, -2.0f, 5.0f),
				     V3(1.0f, 1.0f, 1.0f),
				     Quat(0.0f, 0.0f, 0.0f, 0.0f));

   Curve curve;
   curve.p1 = V2(0.0f, -0.5f);
   curve.p2 = V2(2.0f, -0.3f);
   curve.p3 = V2(-2.0f, 0.3f);
   curve.p4 = V2(0.0f, 0.5f);

   // curve.p1 = V2(0.0f, -0.5f);
   // curve.p2 = V2(0.0f, -0.5f);
   // curve.p3 = V2(0.0f, 0.5f);
   // curve.p4 = V2(0.0f, 0.5f);
   state.testTrack = CreateTrack(V3(0.0f, 0.0f, 0.0f), V3(1.0f, 10.0f, 1.0f), curve);
   GenerateTrackSegmentVertices(state.testTrack);

   state.collectibles = Collectibles();
   CreateCollectibles(state.collectibles);
}

void GameLoop(GameState &state)
{
   m4 cameraTransform = CameraMatrix(state.camera);

   //UpdatePlayer(state.sphereGuy, state.collectibles);
   //UpdateCamera(state.camera, state.sphereGuy.worldPos);
   state.lightPos = state.camera.position;


   static float rotation = 0.0f;

   Curve curve;
   curve.p1 = V2(0.0f, -0.5f);
   curve.p2 = V2(2.0f, -0.3f);
   curve.p3 = V2(-2.0f, 0.3f);
   curve.p4 = V2(0.0f, 0.5f);

   Curve curve2;
   curve2.p1 = V2(0.0f, -0.5f);
   curve2.p2 = V2(-2.0f, -0.3f);
   curve2.p3 = V2(2.0f, 0.3f);
   curve2.p4 = V2(0.0f, 0.5f);

   state.testTrack.bezier = lerp(curve, curve2, (1.0f + sinf(rotation * 10.0f)) / 2.0f);
   GenerateTrackSegmentVertices(state.testTrack);

   state.testTrack.renderable.orientation = Rotation(V3(0.0f, 1.0f, 0.0f), rotation);
   rotation += delta / 100.0f;
   
   //RenderPushObject(state.sphereGuy, cameraTransform, state.lightPos);
   RenderPushObject(state.testTrack.renderable, cameraTransform, state.lightPos);

   /*
   for(i32 i = 0; i < state.collectibles.size; ++i)
   {
      RenderPushObject(state.collectibles.c[i], cameraTransform, state.lightPos); 
   }
   */
}

#if 0
void OnKeyDown(GameState &state)
{   
   // toggle branch
   if(state.keyState != down)
   {
      for(int i = 0; i < state.lines.n; ++i)
      {
	 Track &line = state.lines.list[i];
	 if(line.flags & BRANCH)
	 {
	    if(line.flags & RIGHT)
	    {
	       line.flags &= ~RIGHT;
	       line.flags |= LEFT;

	       if(line.left)
	       {
		  line.coords[1] = line.left->coords[0];
	       }
	    }
	    else
	    {
	       line.flags &= ~LEFT;
	       line.flags |= RIGHT;

	       if(line.right)
	       {
		  line.coords[1] = line.right->coords[0];
	       }
	    }
	 }
      }


      if(state.player.currentTrack->flags & BRANCH)
      {
	 state.camera.flags |= INTERPOLATING;	
	 state.camera.interpElapsed = 0.0f;	 
      }
   }
   state.keyState = down;
}
#endif
