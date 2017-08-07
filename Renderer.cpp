
static m4 Projection = Projection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 100.0f, 60.0f);
static m4 InfiniteProjection = InfiniteProjection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 60.0f);

static ShaderProgram BreakBlockProgram;

static const u32 RectangleAttribCount = 6;
static GLuint RectangleUVBuffer;
static GLuint textVao;
static GLuint textUVVbo;
static GLuint textVbo;

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
RenderState InitRenderState(StackAllocator *stack)
{
   RenderState result;   
   // init scene framebuffer with light attachment
   glGenFramebuffers(1, &result.fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);

   glGenTextures(1, &result.mainColorTexture);
   glBindTexture(GL_TEXTURE_2D, result.mainColorTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.mainColorTexture, 0);

   glGenRenderbuffers(1, &result.depthBuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, result.depthBuffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, result.depthBuffer);

   glGenTextures(1, &result.blurTexture);
   glBindTexture(GL_TEXTURE_2D, result.blurTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, result.blurTexture, 0);         
   
   assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   //glBindFramebuffer(GL_FRAMEBUFFER, 0);   

   // init horizontal and vertical blur framebuffers
   glGenFramebuffers(1, &result.horizontalFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.horizontalFbo);
   glGenTextures(1, &result.horizontalColorBuffer);
   glBindTexture(GL_TEXTURE_2D, result.horizontalColorBuffer);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.horizontalColorBuffer, 0);

   glGenFramebuffers(1, &result.verticalFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, result.verticalFbo);
   glGenTextures(1, &result.verticalColorBuffer);
   glBindTexture(GL_TEXTURE_2D, result.verticalColorBuffer);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.verticalColorBuffer, 0);         

   {
      size_t vertSize;
      size_t fragSize;
      char *vertSource;
      char *fragSource;
      GLuint vertHandle;
      GLuint fragHandle;

      LoadProgramFiles("assets\\ScreenTexture.vertp", "assets\\ScreenTexture.fragp",
			&vertSize, &fragSize, &vertSource, &fragSource, stack);

      result.fullScreenProgram = MakeProgram(vertSource, vertSize, fragSource, fragSize,
					     &vertHandle, &fragHandle);

      stack->pop();
      stack->pop();
   }

   {
      size_t vertSize;
      size_t fragSize;
      char *vertSource;
      char *fragSource;
      GLuint vertHandle;
      GLuint fragHandle;

      LoadProgramFiles("assets\\ApplyBlur.vertp", "assets\\ApplyBlur.fragp",
			&vertSize, &fragSize, &vertSource, &fragSource, stack);

      result.blurProgram = MakeProgram(vertSource, vertSize, fragSource, fragSize,
				       &vertHandle, &fragHandle);

      stack->pop();
      stack->pop();
   }

   
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


static inline
m3 TextProjection(float screenWidth, float screenHeight)
{
   return {2.0f / screenWidth, 0.0f, 0.0f,
	 0.0f, 2.0f / screenHeight, 0.0f,
	 -1.0f, -1.0f, 1.0f};
}

void RenderBackground(GameState &state)
{   
   glDisable(GL_DEPTH_TEST);

   glUseProgram(state.backgroundProgram);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glUseProgram(0);

   glEnable(GL_DEPTH_TEST);
}

void BeginFrame(GameState &state)
{
   glBindFramebuffer(GL_FRAMEBUFFER, state.renderer.fbo);
   GLuint  attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};   
   glDrawBuffers(2, attachments);

   // @we really only need to clear the color of the secondary buffer, can we do this?
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
   glDisable(GL_DEPTH_TEST);
   RenderBackground(state);
   glEnable(GL_DEPTH_TEST);
}

void EndFrame(GameState &state)
{
   //apply blur
   /*glUseProgram(state.renderer.blurProgram);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, state.renderer.blurTexture);
   glUniform1i(glGetUniformLocation(state.renderer.blurProgram, "tex"), 0);
   glUniform1f(glGetUniformLocation(state.renderer.blurProgram, "xstep"), 1.0f / (float)(SCREEN_WIDTH >> 2));
   glUniform1f(glGetUniformLocation(state.renderer.blurProgram, "ystep"), 0.0f);
   glEnableVertexAttribArray(UV_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glDisable(GL_DEPTH_TEST);
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glDisable(GL_DEPTH_TEST);
   glUseProgram(0);
   */
   // blit buffer
   glBindFramebuffer(GL_FRAMEBUFFER, state.renderer.horizontalFbo);
   
   glUseProgram(state.renderer.fullScreenProgram);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, state.renderer.blurTexture);
   glUniform1i(glGetUniformLocation(state.renderer.fullScreenProgram, "image2"), 0);

   glUniform1f(glGetUniformLocation(state.renderer.fullScreenProgram, "xstep"), 1.0f / (float)(SCREEN_WIDTH));
   glUniform1f(glGetUniformLocation(state.renderer.fullScreenProgram, "ystep"), 0.0f);

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);

   glBindFramebuffer(GL_FRAMEBUFFER, state.renderer.verticalFbo);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, state.renderer.horizontalColorBuffer);
   glUniform1i(glGetUniformLocation(state.renderer.fullScreenProgram, "image2"), 0);

   glUniform1f(glGetUniformLocation(state.renderer.fullScreenProgram, "xstep"), 0.0f);
   glUniform1f(glGetUniformLocation(state.renderer.fullScreenProgram, "ystep"), 1.0f / (float)(SCREEN_HEIGHT));

   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);

   // finally blit it to the screen
   // @should change programs!!!   
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glUseProgram(state.renderer.blurProgram);
   glBindBuffer(GL_ARRAY_BUFFER, ScreenVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ARRAY_BUFFER, RectangleUVBuffer);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, state.renderer.mainColorTexture);
      
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, state.renderer.verticalColorBuffer);

   glUniform1i(glGetUniformLocation(state.renderer.blurProgram, "scene"), 0);
   glUniform1i(glGetUniformLocation(state.renderer.blurProgram, "blur"), 1);

   glEnable(GL_FRAMEBUFFER_SRGB);
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glDisable(GL_FRAMEBUFFER_SRGB);

   glUseProgram(0);
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
void RenderPushObject(Object &obj, m4 &camera, v3 lightPos, v3 diffuseColor)
{
   m4 orientation = M4(obj.orientation);
   m4 scale = Scale(obj.scale);
   m4 translation = Translate(obj.worldPos);
   
   m4 transform = translation * scale * orientation;
   
   RenderPushMesh(*obj.p, obj.meshBuffers, transform, camera, lightPos, diffuseColor);
}

static
void RenderPushBreak(Object &obj, m4 &camera, v3 lightPos, v3 diffuseColor)
{
   RenderPushObject(obj, camera, lightPos, diffuseColor);

   m4 translation = Translate(V3(obj.worldPos.x, obj.worldPos.y + 0.5f * TRACK_SEGMENT_SIZE, obj.worldPos.z));
   m4 orientation = M4(Rotation(V3(-1.0f, 0.0f, 0.0f), 1.5708f));
   m4 scale = Scale(V3(5.0f, 5.0f, 5.0f));

   m4 model = translation * orientation * scale;
   m4 mvp = InfiniteProjection * camera * model;

   glBindBuffer(GL_ARRAY_BUFFER, RectangleVertBuffer);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

   glUseProgram(BreakBlockProgram.programHandle);
   glUniformMatrix4fv(BreakBlockProgram.MVPUniform, 1, GL_FALSE, mvp.e);
   glDrawArrays(GL_TRIANGLES, 0, RectangleAttribCount);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glUseProgram(0);
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


i32 RenderTracks(GameState &state)
{
   BEGIN_TIME();
   i32 rendered = 0;
   for(i32 i = 0; i < state.tracks.capacity; ++i)
   {	    
      if(!(state.tracks.adjList[i].flags & Attribute::invisible) &&
	 !(state.tracks.adjList[i].flags & Attribute::unused))
      {	 
	 {
	    if(state.tracks.adjList[i].flags & Attribute::breaks)
	    {
	       RenderPushBreak(state.tracks.elements[i].renderable, state.camera.view, state.lightPos, V3(0.0f, 1.0f, 0.0f));
	    }
	    else
	    {
	       RenderPushObject(state.tracks.elements[i].renderable, state.camera.view, state.lightPos, V3(0.0f, 1.0f, 0.0f));
	    }
	 }
	 ++rendered;
      }
   }
   END_TIME();
   READ_TIME(state.TrackRenderTime);

   return rendered;
}
