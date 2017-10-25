
#define NORMAL_COLOR  (V3(0.0f, 1.0f, 1.0f))
#define SPEEDUP_COLOR (V3(0.0f, 1.0f, 0.0f))
 
static m4 Projection;
static m4 InfiniteProjection;

static ShaderProgram BreakBlockProgram;
static ShaderProgram ButtonProgram;
static ShaderProgram SuperBrightProgram;

static const u32 RectangleAttribCount = 6;
static GLuint RectangleUVBuffer;
static GLuint textVao;
static GLuint textUVVbo;
static GLuint textVbo;

static MeshObject Sphere;
static MeshObject LinearTrack;
static MeshObject BranchTrack;
static MeshObject BreakTrack;
static MeshObject LeftBranchTrack;
static MeshObject RightBranchTrack;
static MeshObject *LockedBranchTrack;

static ShaderProgram DefaultShader;
static ShaderProgram DefaultInstanced;

static float RectangleUVs[] =
{
   0.0f, 0.0f,
   1.0f, 0.0f,
   0.0f, 1.0f,

   1.0f, 0.0f,
   1.0f, 1.0f,
   0.0f, 1.0f,
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

inline static
TrackFrustum CreateTrackFrustum(m4 worldToClip)
{
   TrackFrustum result;
   v4 a = V4(worldToClip.e2[0][0],
	     worldToClip.e2[1][0],
	     worldToClip.e2[2][0],
	     worldToClip.e2[3][0]);
   v4 b = V4(worldToClip.e2[0][3],
	     worldToClip.e2[1][3],
	     worldToClip.e2[2][3],
	     worldToClip.e2[3][3]);

   result.left = b + a;
   result.right = b - a;

   return result;
}

inline static
bool PointFrustumTest(v3 point, TrackFrustum frustum)
{
   v4 point4 = V4(point.x, point.y, point.z, 1.0f);
   float left = dot(frustum.left, point4);

   if(left >= 0.0f)
   {
      float right = dot(frustum.right, point4);

      if(right >= 0.0f)
      {
	 return true;
      }
   }

   return false;
}

//@ NEEDS TO BE TESTED!!
static inline
BBox BranchBBox(Curve &c, v3 beginPoint)
{
   BBox result;

   float radius = 0.5f;
   if(c.p4.x > 0.0f)
   {
      radius = -radius;
   }
   
   v3 endPoint = V3(beginPoint.x + c.p4.x - radius, beginPoint.y + c.p4.y, -radius);
   beginPoint.x += radius;
   beginPoint.z += radius;
   v3 midPoint = beginPoint + (0.5f * (endPoint - beginPoint));
   v3 magnitude = endPoint - midPoint;

   result.position = midPoint;
   result.magnitude = magnitude;
   

   return result;
}

static inline
BBox LinearBBox(v3 beginPoint)
{
   BBox result;

   float radius = 0.5f;

   v3 endPoint = V3(beginPoint.x, beginPoint.y + TRACK_SEGMENT_SIZE, 0.0f);
   v3 midPoint = beginPoint + (0.5f * (endPoint - beginPoint));
   v3 magnitude = endPoint - midPoint;

   result.position = midPoint;
   result.magnitude = magnitude;
   result.magnitude.x += radius;
   result.magnitude.z += radius;

   return result;
}

static inline
BBox BreakBBox(v3 beginPoint)
{
   BBox result;

   float radius = 2.5f;
   
   v3 endPoint = V3(beginPoint.x, beginPoint.y + (0.5f * TRACK_SEGMENT_SIZE), 0.0f);
   v3 midPoint = beginPoint + (0.5f * (endPoint - beginPoint));
   v3 magnitude = endPoint - midPoint;

   magnitude.x += radius;
   magnitude.z += radius;
   result.position = midPoint;
   result.magnitude = magnitude;

   return result;
}

static
bool BBoxFrustumTest(TrackFrustum &f, BBox &b)
{
   if(PointFrustumTest(V3(b.position.x + b.magnitude.x, b.position.y + b.magnitude.y, b.position.z + b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x - b.magnitude.x, b.position.y + b.magnitude.y, b.position.z + b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x - b.magnitude.x, b.position.y - b.magnitude.y, b.position.z + b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x + b.magnitude.x, b.position.y - b.magnitude.y, b.position.z + b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x + b.magnitude.x, b.position.y + b.magnitude.y, b.position.z - b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x - b.magnitude.x, b.position.y + b.magnitude.y, b.position.z - b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x - b.magnitude.x, b.position.y - b.magnitude.y, b.position.z - b.magnitude.z), f)) return true;
   if(PointFrustumTest(V3(b.position.x + b.magnitude.x, b.position.y - b.magnitude.y, b.position.z - b.magnitude.z), f)) return true;
   
   return false;
}

CommandState InitCommandState(StackAllocator *allocator)
{
   CommandState result;
   result.currentProgram = 0;
   result.count = 0;
   result.first = 0;   
   result.last = 0;
   // 1024 for now, but we may be able to find tighter bounds!
   result.linearInstances = (LinearInstance *)allocator->push(sizeof(LinearInstance) * 1024);
   result.linearInstanceCount = 0;

   glGenBuffers(1, &result.instanceMVPBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, result.instanceMVPBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(m4) * 1024, 0, GL_STREAM_DRAW);

   glGenBuffers(1, &result.instanceModelMatrixBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, result.instanceModelMatrixBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(m4) * 1024, 0, GL_STREAM_DRAW);

   glGenBuffers(1, &result.instanceColorBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, result.instanceColorBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(v3) * 1024, 0, GL_STREAM_DRAW);

   glGenVertexArrays(1, &result.instanceVao);
   glBindVertexArray(result.instanceVao);
   
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, LinearTrack.handles.vbo);
   glVertexAttribPointer(VERTEX_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
   
   glEnableVertexAttribArray(NORMAL_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, LinearTrack.handles.nbo);
   glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
      
   glEnableVertexAttribArray(COLOR_INPUT_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, result.instanceColorBuffer);
   glVertexAttribPointer(COLOR_INPUT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
   
   glBindBuffer(GL_ARRAY_BUFFER, result.instanceMVPBuffer);
   glEnableVertexAttribArray(MATRIX1_LOCATION);
   glVertexAttribPointer(MATRIX1_LOCATION, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), 0);
   glEnableVertexAttribArray(MATRIX1_LOCATION + 1);
   glVertexAttribPointer(MATRIX1_LOCATION + 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)sizeof(v4));
   glEnableVertexAttribArray(MATRIX1_LOCATION + 2);
   glVertexAttribPointer(MATRIX1_LOCATION + 2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)(2 * sizeof(v4)));
   glEnableVertexAttribArray(MATRIX1_LOCATION + 3);
   glVertexAttribPointer(MATRIX1_LOCATION + 3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)(3 * sizeof(v4)));

   glBindBuffer(GL_ARRAY_BUFFER, result.instanceModelMatrixBuffer);
   glEnableVertexAttribArray(MATRIX2_LOCATION);
   glVertexAttribPointer(MATRIX2_LOCATION, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), 0);
   glEnableVertexAttribArray(MATRIX2_LOCATION + 1);
   glVertexAttribPointer(MATRIX2_LOCATION + 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)sizeof(v4));
   glEnableVertexAttribArray(MATRIX2_LOCATION + 2);
   glVertexAttribPointer(MATRIX2_LOCATION + 2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)(2 * sizeof(v4)));
   glEnableVertexAttribArray(MATRIX2_LOCATION + 3);
   glVertexAttribPointer(MATRIX2_LOCATION + 3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(v4), (void *)(3 * sizeof(v4)));

   glVertexAttribDivisor(VERTEX_LOCATION, 0);
   glVertexAttribDivisor(NORMAL_LOCATION, 0);
   glVertexAttribDivisor(COLOR_INPUT_LOCATION, 1);
   glVertexAttribDivisor(MATRIX1_LOCATION, 1);
   glVertexAttribDivisor(MATRIX1_LOCATION+1, 1);
   glVertexAttribDivisor(MATRIX1_LOCATION+2, 1);
   glVertexAttribDivisor(MATRIX1_LOCATION+3, 1);
   glVertexAttribDivisor(MATRIX2_LOCATION, 1);
   glVertexAttribDivisor(MATRIX2_LOCATION+1, 1);
   glVertexAttribDivisor(MATRIX2_LOCATION+2, 1);
   glVertexAttribDivisor(MATRIX2_LOCATION+3, 1);   

   glBindVertexArray(0);

   return result;
}

inline void
CommandState::PushBindProgram(ProgramBase *program, StackAllocator *allocator)
{
   if(currentProgram != program)
   {
      BindProgramCommand *command;
      if(first)
      {
	 last->next = (CommandBase *)allocator->push(sizeof(BindProgramCommand));
	 command = (BindProgramCommand *)last->next;
      }
      else
      {
	 first = (CommandBase *)allocator->push(sizeof(BindProgramCommand));
	 command = (BindProgramCommand *)first;
      }
   
      command->command = BindProgram;
      command->program = program;
      command->next = 0;
      last = command;
      currentProgram = program;
      ++count;
   }
}

inline void
CommandState::PushDrawLinear(Object obj, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawLinearCommand));
   DrawLinearCommand *command = (DrawLinearCommand *)last->next;
   command->command = DrawLinear;
   command->obj = obj;
   last = command;
   ++count;
}

inline
void CommandState::PushLinearInstance(Object obj, v3 color)
{
   B_ASSERT(linearInstanceCount < 1024);
   linearInstances[linearInstanceCount++] = {obj.worldPos,
					     obj.orientation,
					     obj.scale,
					     color};
}

inline void
CommandState::PushDrawButton(v2 position, v2 scale, GLuint texture, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawButtonCommand));
   DrawButtonCommand *command = (DrawButtonCommand *)last->next;
   command->command = DrawButton;
   command->position = position;
   command->scale = scale;
   command->texture = texture;
   last = command;
   ++count;
}

inline void
CommandState::PushDrawSpeedup(Object obj, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawSpeedupCommand));
   DrawSpeedupCommand *command = (DrawSpeedupCommand *)last->next;
   command->command = DrawSpeedup;
   command->obj = obj;
   last = command;
   ++count;
}

inline void
CommandState::PushDrawBranch(Object obj, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawBranchCommand));
   DrawBranchCommand *command = (DrawBranchCommand *)last->next;
   command->command = DrawBranch;
   command->obj = obj;
   last = command;
   ++count;
}

inline void
CommandState::PushDrawLockedBranch(Object obj, MeshObject *buffers, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawLockedBranchCommand));
   DrawLockedBranchCommand *command = (DrawLockedBranchCommand *)last->next;
   command->command = DrawLockedBranch;
   command->obj = obj;
   command->mesh = buffers;
   last = command;
   ++count;
}

inline void
CommandState::PushDrawBreak(Object obj, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawBreakCommand));
   DrawBreakCommand *command = (DrawBreakCommand *)last->next;
   command->command = DrawBreak;
   command->obj = obj;
   last = command;
   ++count;
}


inline void
CommandState::PushDrawMesh(MeshObject mesh, v3 position, v3 scale, quat orientation, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawMeshCommand));
   DrawMeshCommand *command = (DrawMeshCommand *)last->next;
   command->command = DrawMesh;
   command->mesh = mesh;
   command->position = position;
   command->scale = scale;
   command->orientation = orientation;
   last = command;
   ++count;
}

inline void
CommandState::PushDrawBreakTexture(v3 position, v3 scale, quat orientation, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawBreakTextureCommand));
   DrawBreakTextureCommand *command = (DrawBreakTextureCommand *)last->next;
   command->command = DrawBreakTexture;
   command->position = position;
   command->scale = scale;
   command->orientation = orientation;
   last = command;
   ++count;
}

inline void
CommandState::PushRenderText(char *text, u32 textSize, v2 position, v2 scale, v3 color, StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawTextCommand) + textSize);
   DrawTextCommand *command = (DrawTextCommand *)last->next;
   command->command = DrawString;
   command->textSize = textSize;
   command->position = position;
   command->scale = scale;
   command->color = color;

   // copy string into the end of the command
   char *string = command->GetString();
   for(u32 i = 0; i < textSize; ++i)
   {
      string[i] = text[i];
   }

   last = command;
   ++count;
}

inline void
CommandState::PushRenderLinearInstances(StackAllocator *allocator)
{
   B_ASSERT(first);
   B_ASSERT(currentProgram);

   last->next = (CommandBase *)allocator->push(sizeof(DrawLinearInstancesCommand));
   DrawLinearInstancesCommand *command = (DrawLinearInstancesCommand *)last->next;
   command->command = DrawLinearInstances;

   last = command;
   ++count;
}

inline void
CommandState::PushRenderBlur(StackAllocator *allocator)
{
   last->next = (CommandBase *)allocator->push(sizeof(CommandBase));
   last->next->command = DrawBlur;
   last = last->next;
   ++count;
}

inline void
CommandState::Clean(StackAllocator *allocator)
{
   while(count--)
   {
      allocator->pop();
   }
   currentProgram = 0;
   first = 0;
   last = 0;
   count = 0;
}

static void
LoadProgramFiles(char *vert, char *frag,
		 size_t *outVertSize, size_t *outFragSize, char **outVertSource, char **outFragSource,
		 StackAllocator *allocator)
{
   *outVertSize = FileSize(vert);
   *outFragSize = FileSize(frag);
   *outVertSource = (char *)allocator->push(*outVertSize);
   *outFragSource = (char *)allocator->push(*outFragSize);
   FileRead(vert, (u8 *)(*outVertSource), *outVertSize);
   FileRead(frag, (u8 *)(*outFragSource), *outFragSize);   
}

GLuint
MakeProgram(char *vertexSource, size_t vsize, char *fragmentSource, size_t fsize,
	    GLuint *outVertexHandle, GLuint *outFragmentHandle)
{
   GLuint result = glCreateProgram();
   *outVertexHandle = glCreateShader(GL_VERTEX_SHADER);
   *outFragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);
   
   glShaderSource(*outVertexHandle, 1, &vertexSource, (i32 *)&vsize);
   glShaderSource(*outFragmentHandle, 1, &fragmentSource, (i32 *)&fsize);

   glCompileShader(*outVertexHandle);
   glCompileShader(*outFragmentHandle);
   
   glAttachShader(result, *outVertexHandle);
   glAttachShader(result, *outFragmentHandle);

   glLinkProgram(result);

   return result;
}

static B_INLINE
GLuint CreateSimpleProgramFromAssets(Asset &vert, Asset &frag)
{
   GLuint dummyVert = 0;
   GLuint dummyFrag = 0;
   return MakeProgram((char *)vert.mem, vert.size, (char *)frag.mem, frag.size,
		      &dummyVert, &dummyFrag);
}

ShaderProgram
CreateProgram(char *vertexSource, size_t vsize, char *fragmentSource, size_t fsize)
{
   ShaderProgram result;

   result.programHandle = MakeProgram(vertexSource, vsize, fragmentSource, fsize,
				      &result.vertexHandle, &result.fragmentHandle);

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

static B_INLINE
ShaderProgram CreateProgramFromAssets(Asset &vert, Asset &frag)
{
   ShaderProgram result = CreateProgram((char *)vert.mem, vert.size,
					(char *)frag.mem, frag.size);
   vert.flags |= Asset::OnGpu;
   frag.flags |= Asset::OnGpu;

   return result;
}

static
ShaderProgram LoadFilesAndCreateProgram(char *vertex, char *fragment, StackAllocator *allocator)
{
   size_t vertSize;
   size_t fragSize;
   char *vertBuffer;
   char *fragBuffer;
   LoadProgramFiles(vertex, fragment, &vertSize, &fragSize, &vertBuffer, &fragBuffer, allocator);

   ShaderProgram result = CreateProgram(vertBuffer, vertSize, fragBuffer, fragSize);

   allocator->pop();
   allocator->pop();
   
   return result;
}

TextProgram
CreateTextProgram(char *vertexSource, size_t vsize, char *fragmentSource, size_t fsize)
{
   TextProgram result;

   static int called = 0;


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

static B_INLINE
TextProgram CreateTextProgramFromAssets(Asset &vert, Asset &frag)
{
   TextProgram result = CreateTextProgram((char *)vert.mem, vert.size,
					  (char *)frag.mem, frag.size);

   vert.flags |= Asset::OnGpu;
   frag.flags |= Asset::OnGpu;

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

static
RenderState InitRenderState(StackAllocator *stack, AssetManager &assetManager)
{
   RenderState result;      

   LOG_WRITE("WIDTH: %d, HEIGHT: %d", SCREEN_WIDTH, SCREEN_HEIGHT);

   
   // init scene framebuffer with light attachment
   glGenFramebuffers(1, &result.fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);   
   GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
   glDrawBuffers(3, attachments);
   
   glGenTextures(1, &result.mainColorTexture);
   glBindTexture(GL_TEXTURE_2D, result.mainColorTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.mainColorTexture, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenRenderbuffers(1, &result.depthBuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, result.depthBuffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, result.depthBuffer);
      
   glGenTextures(1, &result.blurTexture);   
   glBindTexture(GL_TEXTURE_2D, result.blurTexture);   
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, result.blurTexture, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   
   glGenTextures(1, &result.normalTexture);   
   glBindTexture(GL_TEXTURE_2D, result.normalTexture);   
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);   
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, result.normalTexture, 0);   
   glBindTexture(GL_TEXTURE_2D, 0);   

   DEBUG_DO(GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
   LOG_WRITE("%X", status);

   B_ASSERT(status == GL_FRAMEBUFFER_COMPLETE);

   // init horizontal and vertical blur framebuffers
   glGenFramebuffers(1, &result.horizontalFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.horizontalFbo);
   glDrawBuffers(1, attachments);   
   glGenTextures(1, &result.horizontalColorBuffer);
   glBindTexture(GL_TEXTURE_2D, result.horizontalColorBuffer);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.horizontalColorBuffer, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   DEBUG_DO(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
   LOG_WRITE("%X", status);

   B_ASSERT(status == GL_FRAMEBUFFER_COMPLETE);

   glGenFramebuffers(1, &result.verticalFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.verticalFbo);
   glDrawBuffers(1, attachments);
   glGenTextures(1, &result.verticalColorBuffer);
   glBindTexture(GL_TEXTURE_2D, result.verticalColorBuffer);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.verticalColorBuffer, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   DEBUG_DO(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
   LOG_WRITE("%X", status);
   
   B_ASSERT(status == GL_FRAMEBUFFER_COMPLETE);
   
   {

      result.fullScreenProgram = CreateSimpleProgramFromAssets(assetManager.LoadStacked(AssetHeader::ScreenTexture_vert_ID),
							       assetManager.LoadStacked(AssetHeader::ScreenTexture_frag_ID));

      assetManager.PopStacked(AssetHeader::ScreenTexture_vert_ID);
      assetManager.PopStacked(AssetHeader::ScreenTexture_frag_ID);
   }

   {
      result.outlineProgram = CreateSimpleProgramFromAssets(assetManager.LoadStacked(AssetHeader::ScreenTexture_vert_ID),
							    assetManager.LoadStacked(AssetHeader::outline_frag_ID));

      assetManager.PopStacked(AssetHeader::ScreenTexture_vert_ID);
      assetManager.PopStacked(AssetHeader::outline_frag_ID);
   }

   {
      result.blurProgram = CreateSimpleProgramFromAssets(assetManager.LoadStacked(AssetHeader::ApplyBlur_vert_ID),
							 assetManager.LoadStacked(AssetHeader::ApplyBlur_frag_ID));

      assetManager.PopStacked(AssetHeader::ApplyBlur_vert_ID);
      assetManager.PopStacked(AssetHeader::ApplyBlur_frag_ID);
   }
   

   glGenBuffers(1, &result.buttonVbo);
   glBindBuffer(GL_ARRAY_BUFFER, result.buttonVbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * 6, 0, GL_STATIC_DRAW);
   

   result.commands = InitCommandState(stack);
   
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

// n^2 operation
// flat normals -> smooth normals
v3 *SmoothNormals(v3 *verts, v3 *normals, v3 *outNormals, u32 count, StackAllocator *alloc)
{
   float *times = (float *)alloc->push(count * sizeof(float));

   for(u32 i = 0; i < count; ++i)
   {
      times[i] = 1.0f;
      outNormals[i] = normals[i];
   }

   for(u32 i = 0; i < count; ++i)
   {
      for(u32 j = i+1; j < count; ++j)
      {
	 if(Approx(verts[i], verts[j]))
	 {
	    outNormals[i] = outNormals[i] + normals[j];
	    outNormals[j] = outNormals[j] + normals[i];
	    times[i] += 1.0f;
	    times[j] += 1.0f;
	 }
      }
   }

   for(u16 i = 0; i < count; ++i) {
      outNormals[i] = outNormals[i] / times[i];
   }

   alloc->pop();
   return outNormals;
}

static inline
m3 TextProjection(float screenWidth, float screenHeight)
{
   return {2.0f / screenWidth, 0.0f, 0.0f,
	 0.0f, -2.0f / screenHeight, 0.0f,
	 -1.0f, 1.0f, 1.0f};
}

void RenderBackground(GameState &state)
{   
   glDisable(GL_DEPTH_TEST);

   glUseProgram(state.backgroundProgram);

   glEnableVertexAttribArray(VERTEX_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);   
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glUseProgram(0);   
}

void BeginFrame(GameState &state)
{   
   glBindFramebuffer(GL_FRAMEBUFFER, state.renderer.fbo); // @Android merging!!

   // @we really only need to clear the color of the secondary buffer, can we do this?
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);   
   RenderBackground(state);
   glEnable(GL_DEPTH_TEST);
}

void RenderBlur(RenderState &renderer, Camera &camera)
{
   glDisable(GL_DEPTH_TEST);

   glBindFramebuffer(GL_FRAMEBUFFER, renderer.horizontalFbo);
   
   glUseProgram(renderer.fullScreenProgram);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, renderer.blurTexture);
   glUniform1i(glGetUniformLocation(renderer.fullScreenProgram, "image2"), 0);

   glUniform1f(glGetUniformLocation(renderer.fullScreenProgram, "xstep"), 1.0f / (float)(SCREEN_WIDTH));
   glUniform1f(glGetUniformLocation(renderer.fullScreenProgram, "ystep"), 0.0f);

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);

   glBindFramebuffer(GL_FRAMEBUFFER, renderer.verticalFbo);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, renderer.horizontalColorBuffer);
   glUniform1i(glGetUniformLocation(renderer.fullScreenProgram, "image2"), 0);

   glUniform1f(glGetUniformLocation(renderer.fullScreenProgram, "xstep"), 0.0f);
   glUniform1f(glGetUniformLocation(renderer.fullScreenProgram, "ystep"), 1.0f / (float)(SCREEN_HEIGHT));

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   
   // finally blit it to the screen
   // @should change programs!!!   
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glUseProgram(renderer.blurProgram);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, renderer.mainColorTexture);
   glUniform1i(glGetUniformLocation(renderer.blurProgram, "scene"), 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, renderer.verticalColorBuffer);
   glUniform1i(glGetUniformLocation(renderer.blurProgram, "blur"), 1);
   
   // temporary to get Android build to work
   // @Support non SRGB framebuffers!
   #ifdef WIN32_BUILD
   glEnable(GL_FRAMEBUFFER_SRGB);
   #endif
   
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);

   #ifdef WIN32_BUILD
   glDisable(GL_FRAMEBUFFER_SRGB);
   #endif

   // maybe a good opportunity to do this at the same time as the blur
   /*
   glUseProgram(renderer.outlineProgram);

   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, renderer.normalTexture);
   glUniform1i(glGetUniformLocation(renderer.outlineProgram, "normals"), 0);
   glUniform3fv(glGetUniformLocation(renderer.outlineProgram, "forward"), 1, camera.forward.e);
   glDisable(GL_DEPTH_TEST);
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glEnable(GL_DEPTH_TEST);
   */
}

void RenderMesh(ShaderProgram *p, MeshObject b, m4 &transform, m4 &view, v3 lightPos, v3 diffuseColor = V3(0.3f, 0.3f, 0.3f))
{
   static float time = 0.0f;
   time += delta;

   m4 lightRotation = M4(Rotation(V3(0.0f, 1.0f, 0.0f), -time / 50.0f));
   m4 projection = InfiniteProjection;
   m4 mvp = projection * view * transform;
   
   glUniformMatrix4fv(p->MVPUniform, 1, GL_FALSE, mvp.e);
   glUniformMatrix4fv(p->modelUniform, 1, GL_FALSE, transform.e);
   glUniformMatrix4fv(p->viewUniform, 1, GL_FALSE, view.e);
   glUniform3fv(p->diffuseUniform, 1, diffuseColor.e);
   glUniform3fv(p->lightPosUniform, 1, lightPos.e);
   glBindVertexArray(b.handles.vao);
   glDrawArrays(GL_TRIANGLES, 0, b.mesh.vcount);
   glBindVertexArray(0);
}

static
void RenderObject(Object &obj, MeshObject buffers, ShaderProgram *program, m4 &camera, v3 lightPos, v3 diffuseColor)
{
   m4 orientation = M4(obj.orientation);
   m4 scale = Scale(obj.scale);
   m4 translation = Translate(obj.worldPos);
   
   m4 transform = translation * scale * orientation;
   
   RenderMesh(program, buffers, transform, camera, lightPos, diffuseColor);
}

static
void RenderBranch(DrawBranchCommand *command, Camera &camera, v3 lightPos, ProgramBase *program, MeshObject buffers = BranchTrack)
{
   RenderObject(command->obj, buffers, (ShaderProgram *)program, camera.view, lightPos, NORMAL_COLOR);
}

static
void RenderLinear(DrawLinearCommand *command, Camera &camera, v3 lightPos, ProgramBase *current)
{
   RenderObject(command->obj, LinearTrack, (ShaderProgram *)current, camera.view, lightPos, NORMAL_COLOR);
}

static
void RenderSpeedup(DrawSpeedupCommand *command, Camera &camera, v3 lightPos, ProgramBase *program)
{
   RenderObject(command->obj, LinearTrack, (ShaderProgram *)program, camera.view, lightPos, SPEEDUP_COLOR);
}

static
void RenderBreak(DrawBreakCommand *command, Camera &camera, v3 lightPos, ProgramBase *currentProgram)
{
   RenderObject(command->obj, BreakTrack, (ShaderProgram *)currentProgram, camera.view, lightPos, NORMAL_COLOR);

   m4 translation = Translate(V3(command->obj.worldPos.x, command->obj.worldPos.y + 0.5f * TRACK_SEGMENT_SIZE, command->obj.worldPos.z));
   m4 orientation = M4(Rotation(V3(-1.0f, 0.0f, 0.0f), 1.5708f));
   m4 scale = Scale(V3(5.0f, 5.0f, 5.0f));

   m4 model = translation * orientation * scale;
   m4 mvp = InfiniteProjection * camera.view * model;

   glBindBuffer(GL_ARRAY_BUFFER, RectangleVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glUseProgram(BreakBlockProgram.programHandle);
   glUniformMatrix4fv(BreakBlockProgram.MVPUniform, 1, GL_FALSE, mvp.e);
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glUseProgram(0);

   glUseProgram(currentProgram->programHandle);
}

static
void RenderButton(DrawButtonCommand *command, GLuint vbo, GLuint program)
{
   glDisable(GL_DEPTH_TEST);
   glUseProgram(ButtonProgram.programHandle);

   float left = command->position.x - command->scale.x;
   float right = command->position.x + command->scale.x;
   float bottom = command->position.y - command->scale.y;
   float top = command->position.y + command->scale.y;
   
   float verts[] =
   {
      left, bottom,
      right, bottom,
      left, top,

      right, bottom,
      right, top,
      left, top
   };

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 12, verts);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, command->texture);
   glUniform1i(glGetUniformLocation(program, "tex"), 0);
   glDrawArrays(GL_TRIANGLES, 0, 6);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glUseProgram(0);
   glEnable(GL_DEPTH_TEST);
}

void RenderTexture(GLuint texture, ShaderProgram &program)
{
   glUseProgram(program.programHandle);

   B_ASSERT(glIsTexture(texture));

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

static
void RenderText_stb(char *string, u32 count, float x, float y, stbFont &font, TextProgram &p)
{
   // convert clip coords to device coords
   x = (x + 1.0f) * ((float)SCREEN_WIDTH / 2.0f);
   y = (y - 1.0f) * -((float)SCREEN_HEIGHT / 2.0f);
   
   glDisable(GL_DEPTH_TEST);

   glBindVertexArray(textVao);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   glUseProgram(p.programHandle);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, font.textureHandle);
   glUniform1i(p.texUniform, 0);

   glUniformMatrix3fv(p.transformUniform, 1, GL_FALSE, TextProjection(SCREEN_WIDTH, SCREEN_HEIGHT).e);

   stbtt_aligned_quad quad;
   
   for(i32 i = 0; i < (i32)count; ++i)
   {
      char c = string[i];
      stbtt_GetPackedQuad(font.chars, font.width, font.height, c, &x, &y, &quad, 0);

      float uvs[] =
	 {
	    quad.s0, quad.t0,
	    quad.s0, quad.t1,
	    quad.s1, quad.t0,

	    quad.s1, quad.t0,
	    quad.s0, quad.t1,
	    quad.s1, quad.t1,
	 };
      
      float verts[] =
	 {
	    quad.x0, quad.y0,
	    quad.x0, quad.y1,
	    quad.x1, quad.y0,

	    quad.x1, quad.y0,
	    quad.x0, quad.y1,
	    quad.x1, quad.y1,
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
void RenderText(DrawTextCommand *command, stbFont &font, TextProgram &p)
{
   RenderText_stb(command->GetString(), command->textSize, command->position.x, command->position.y, font, p);
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

	 B_ASSERT(j != font.count-1);
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
   // allocator->pop();
   // allocator->pop();
   return result;
}

// Inits Mesh Object and uploads to vertex buffers
MeshObject InitMeshObject(u8 *buffer, StackAllocator *allocator)
{
   Mesh mesh;

   mesh.vcount = *((i32 *)buffer);
   mesh.vertices = (float *)(buffer + 8);
   mesh.normals = (v3 *)allocator->push(sizeof(v3) * mesh.vcount);
   v3 *flat_normals = (v3 *)allocator->push(sizeof(v3) * mesh.vcount);
   flat_normals = Normals(mesh.vertices, flat_normals, mesh.vcount);   
   SmoothNormals((v3 *)mesh.vertices, flat_normals, mesh.normals, mesh.vcount, allocator);
   // allocator->pop(); @spurreous?

   MeshBuffers handles = UploadStaticMesh(mesh.vertices, mesh.normals, mesh.vcount, 3);
   allocator->pop(); // pop normals
   allocator->pop(); // pop file

   MeshObject result;
   result.mesh = mesh;
   result.handles = handles;   

   return result;
}

#if defined(DEBUG) && defined(WIN32_BUILD)
void RenderBBoxes(GameState &state)
{  
   glLoadMatrixf((InfiniteProjection * state.camera.view).e);
   glLineWidth(1.0f);

   for(u32 i = 0; i < state.tracks.capacity; ++i)
   {
      BBox box;

      if(state.tracks.adjList[i].flags & Attribute::branch)
      {
	 box = BranchBBox(GlobalBranchCurve, state.tracks.elements[i].renderable.worldPos);
      }
      else if(state.tracks.adjList[i].flags & Attribute::linear ||
	      state.tracks.adjList[i].flags & Attribute::speedup)
      {
	 box = LinearBBox(state.tracks.elements[i].renderable.worldPos);
      }
      else if(state.tracks.adjList[i].flags & Attribute::breaks)
      {
	 box = BreakBBox(state.tracks.elements[i].renderable.worldPos);
      }
      else
      {
	 continue;
      }
      
      glBegin(GL_LINE_LOOP);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y + box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y + box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y - box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y - box.magnitude.y, box.position.z + box.magnitude.z);
      glEnd();

      glBegin(GL_LINE_LOOP);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y + box.magnitude.y, box.position.z - box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y + box.magnitude.y, box.position.z - box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y - box.magnitude.y, box.position.z - box.magnitude.z);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y - box.magnitude.y, box.position.z - box.magnitude.z);
      glEnd();

      glBegin(GL_LINES);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y + box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y + box.magnitude.y, box.position.z - box.magnitude.z);

      glVertex3f(box.position.x - box.magnitude.x, box.position.y + box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y + box.magnitude.y, box.position.z - box.magnitude.z);

      glVertex3f(box.position.x - box.magnitude.x, box.position.y - box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x - box.magnitude.x, box.position.y - box.magnitude.y, box.position.z - box.magnitude.z);
      
      glVertex3f(box.position.x + box.magnitude.x, box.position.y - box.magnitude.y, box.position.z + box.magnitude.z);
      glVertex3f(box.position.x + box.magnitude.x, box.position.y - box.magnitude.y, box.position.z - box.magnitude.z);
      glEnd();
   }   
}
#endif

void
CommandState::ExecuteCommands(Camera &camera, v3 lightPos, stbFont &font, TextProgram &p, RenderState &renderer, StackAllocator *allocator)
{
   CommandBase *current = first;
   for(u32 i = 0; i < count; ++i)
   {
      switch(current->command)
      {
	 case DrawLinear:
	 {
	    RenderLinear((DrawLinearCommand *)current, camera, lightPos, currentProgram);
	 }break;

	 case DrawBranch:
	 {
	    RenderBranch((DrawBranchCommand *)current, camera, lightPos, currentProgram);
	 }break;

	 case DrawSpeedup:
	 {
	    RenderSpeedup((DrawSpeedupCommand *)current, camera, lightPos, currentProgram);
	 }break;

	 case DrawBreak:
	 {
	    RenderBreak((DrawBreakCommand *)current, camera, lightPos, currentProgram);
	 }break;

	 case DrawLinearInstances:
	 {
	    RenderLinearInstances(allocator, lightPos, camera.view);
	 }break;

	 case BindProgram:
	 {
	    BindProgramCommand *programCommand = (BindProgramCommand *)current;
	    glUseProgram(programCommand->program->programHandle);
	 }break;

	 case DrawString:
	 {
	    RenderText((DrawTextCommand *)current, font, p);
	 }break;

	 case DrawBlur:
	 {
	    RenderBlur(renderer, camera);
	 }break;

	 case DrawButton:
	 {
	    RenderButton((DrawButtonCommand *)current, renderer.buttonVbo, currentProgram->programHandle);
	 }break;

	 case DrawLockedBranch:
	 {
	    RenderBranch((DrawLockedBranchCommand *)current, camera, lightPos, currentProgram,
			 *((DrawLockedBranchCommand *)current)->mesh);
	 }break;
#ifdef DEBUG
	 default:
	 {
	    B_ASSERT(false);
	 }break;
#endif
      }
      current = current->next;
   }

   // if no DrawLinearInstances command was issued, then clean command list
   linearInstanceCount = 0;
}

void
CommandState::RenderLinearInstances(StackAllocator *allocator, v3 lightPos, m4 &view)
{
   glUseProgram(DefaultInstanced.programHandle);

   m4 *transforms = (m4 *)allocator->push(sizeof(m4) * linearInstanceCount);
   m4 *MVPs = (m4 *)allocator->push(sizeof(m4) * linearInstanceCount);
   v3 *color = (v3 *)allocator->push(sizeof(v3) * linearInstanceCount);

   m4 vp = InfiniteProjection * view;

   glUniform3fv(DefaultInstanced.lightPosUniform, 1, lightPos.e);
   glUniformMatrix4fv(DefaultInstanced.viewUniform, 1, GL_FALSE, view.e);

   for(u32 i = 0; i < linearInstanceCount; ++i)
   {
      color[i] = linearInstances[i].color;

      m4 orientation = M4(linearInstances[i].rotation);
      m4 scale = Scale(linearInstances[i].scale);
      m4 translation = Translate(linearInstances[i].position);   

      transforms[i] = (translation * scale * orientation);
      MVPs[i] = vp * transforms[i];
   }

   glBindBuffer(GL_ARRAY_BUFFER, instanceMVPBuffer);
   glBufferSubData(GL_ARRAY_BUFFER, 0, linearInstanceCount * sizeof(m4), MVPs);

   glBindBuffer(GL_ARRAY_BUFFER, instanceModelMatrixBuffer);
   glBufferSubData(GL_ARRAY_BUFFER, 0, linearInstanceCount * sizeof(m4), transforms);

   glBindBuffer(GL_ARRAY_BUFFER, instanceColorBuffer);
   glBufferSubData(GL_ARRAY_BUFFER, 0, linearInstanceCount * sizeof(v3),  color);

   glBindVertexArray(instanceVao);   
   glDrawArraysInstanced(GL_TRIANGLES, 0, LinearTrack.mesh.vcount, linearInstanceCount);
   glBindVertexArray(0);
   linearInstanceCount = 0;

   allocator->pop();
   allocator->pop();
   allocator->pop();
}

i32 RenderTracks(GameState &state, StackAllocator *allocator)
{
   BEGIN_TIME();   
   i32 rendered = 0;

   TrackFrustum frustum = CreateTrackFrustum(InfiniteProjection * state.camera.view);

   state.renderer.commands.PushBindProgram(&DefaultShader, allocator);
   for(i32 i = 0; i < state.tracks.capacity; ++i)
   {	    
      if(!(state.tracks.adjList[i].flags & Attribute::invisible) &&
	 !(state.tracks.adjList[i].flags & Attribute::unused))
      {
	 if(state.tracks.adjList[i].flags & Attribute::branch)
	 {
	    BBox box = BranchBBox(GlobalBranchCurve, state.tracks.elements[i].renderable.worldPos);
	    if(BBoxFrustumTest(frustum, box))
	    {
	       if(state.tracks.adjList[i].flags & Attribute::lockedMask)
	       {
		  state.renderer.commands.PushDrawLockedBranch(state.tracks.elements[i].renderable, LockedBranchTrack, allocator);
	       }
	       else
	       {
		  state.renderer.commands.PushDrawBranch(state.tracks.elements[i].renderable, allocator);
	       }
	    }
	 }
	 else if(state.tracks.adjList[i].flags & Attribute::linear)
	 {
	    BBox box = LinearBBox(state.tracks.elements[i].renderable.worldPos);
	    if(BBoxFrustumTest(frustum, box))
	    {	       
	       state.renderer.commands.PushLinearInstance(state.tracks.elements[i].renderable, NORMAL_COLOR);
	    }	    
	 }	    
	 else if(state.tracks.adjList[i].flags & Attribute::breaks)
	 {	       
	    BBox box = BreakBBox(state.tracks.elements[i].renderable.worldPos);
	    if(BBoxFrustumTest(frustum, box))
	    {
	       state.renderer.commands.PushDrawBreak(state.tracks.elements[i].renderable, allocator);
	    }
	 }
	 else if(state.tracks.adjList[i].flags & Attribute::speedup)
	 {
	    BBox box = LinearBBox(state.tracks.elements[i].renderable.worldPos);
	    if(BBoxFrustumTest(frustum, box))
	    {	       
	       state.renderer.commands.PushLinearInstance(state.tracks.elements[i].renderable, SPEEDUP_COLOR);
	    }
	 }
	 else
	 {
	    B_ASSERT(false);
	 }	 
      }	 
   }

   state.renderer.commands.PushRenderLinearInstances(allocator);   
   END_TIME();
   READ_TIME(state.TrackRenderTime);

   return rendered;
}
