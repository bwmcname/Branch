
GLuint LoadImageIntoTexture(Branch_Image_Header *header)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, header->width, header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)(header + 1));
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

static
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

static
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
	 B_ASSERT(!"unsupported channel format");
	 type = 0; // shut the compiler up
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, type, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

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

#define TRACK_RADIUS (0.2f)

static
void GenerateTrackSegmentVertices(MeshObject &meshBuffers, Curve bezier, StackAllocator *alloc)
{
   // 10 segments, 8 tris per segment --- 80 tries   
   tri *tris = (tri *)meshBuffers.mesh.vertices;

   float t = 0.0f;
   v2 sample = CubicBezier(bezier, t);

   v2 direction = Tangent(bezier, t);
   v2 perpindicular = V2(-direction.y, direction.x) * TRACK_RADIUS;         

   v3 top1 = V3(sample.x, sample.y, TRACK_RADIUS);
   v3 right1 = V3(sample.x + perpindicular.x, sample.y, 0.0f);
   v3 bottom1 = V3(sample.x, sample.y, -TRACK_RADIUS);
   v3 left1 = V3(sample.x - perpindicular.x, sample.y, 0.0f);   
   
   for(i32 i = 0; i < 10; ++i)
   {
      t += 0.1f;
      sample = CubicBezier(bezier, t);

      direction = Tangent(bezier, t);
      
      perpindicular = V2(-direction.y, direction.x) * TRACK_RADIUS;      

      v3 top2 = V3(sample.x, sample.y, TRACK_RADIUS);
      v3 right2 = V3(sample.x + perpindicular.x, sample.y + perpindicular.y, 0.0f);
      v3 bottom2 = V3(sample.x, sample.y, -TRACK_RADIUS);
      v3 left2 = V3(sample.x - perpindicular.x, sample.y - perpindicular.y, 0.0f);
      
      i32 j = i * 8;

      // good
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

   v3 *flat_normals = (v3 *)alloc->push(80 * 3 * sizeof(v3));
   v3 *smooth_normals = (v3 *)alloc->push(80 * 3 * sizeof(v3));
   Normals((float *)tris, flat_normals, 80 * 3); // 3 vertices per tri
   SmoothNormals((v3 *)tris, flat_normals, smooth_normals, 80 * 3, alloc);   
   
   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, (sizeof(tri) * 80), (void *)tris);
   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.nbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri) * 80, (void *)smooth_normals);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   alloc->pop();
   alloc->pop();
}

static B_INLINE
void GenerateTrackSegmentVertices(Track &track, MeshObject meshBuffers, StackAllocator *allocator)
{
   GenerateTrackSegmentVertices(meshBuffers, *track.bezier, allocator);
}

static
stbFont InitFont_stb(Asset font, StackAllocator *allocator)
{
   stbFont result;

   PackedFont *cast = (PackedFont *)font.mem;
   result.chars = (stbtt_packedchar *)(font.mem + sizeof(PackedFont));
   result.width = cast->width;
   result.height = cast->height;

   glGenTextures(1, &result.textureHandle);
   glBindTexture(GL_TEXTURE_2D, result.textureHandle);

   GLint value;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
   LOG_WRITE("size: %d\n", value);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, cast->width, cast->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
		font.mem + cast->imageOffset);
   glBindTexture(GL_TEXTURE_2D, 0);

   return result;
}

void LoadGLState(GameState &state, StackAllocator *stack, AssetManager &assetManager)
{
   LinearTrack = AllocateMeshObject(80 * 3, stack);
   BranchTrack = AllocateMeshObject(80 * 3, stack);
   BreakTrack = AllocateMeshObject(80 * 3, stack);
   LeftBranchTrack = AllocateMeshObject(80 * 3, stack);
   RightBranchTrack = AllocateMeshObject(80 * 3, stack);

   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);

   glFrontFace(GL_CCW);
   glCullFace(GL_BACK);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   Asset &defaultVert = state.assetManager.LoadStacked(AssetHeader::default_vert_ID);
   Asset &defaultFrag = state.assetManager.LoadStacked(AssetHeader::default_frag_ID);
   DefaultShader = CreateProgramFromAssets(defaultVert, defaultFrag);

   state.assetManager.PopStacked(AssetHeader::default_frag_ID);
   state.assetManager.PopStacked(AssetHeader::default_vert_ID);

   Asset &defaultInstancedVirt = state.assetManager.LoadStacked(AssetHeader::Default_Instance_vert_ID);
   Asset &defaultInstancedFrag = state.assetManager.LoadStacked(AssetHeader::Default_Instance_frag_ID);
   DefaultInstanced = CreateProgramFromAssets(defaultInstancedVirt, defaultInstancedFrag);

   state.assetManager.PopStacked(AssetHeader::Default_Instance_frag_ID);
   state.assetManager.PopStacked(AssetHeader::Default_Instance_vert_ID);

   state.fontProgram = CreateTextProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::text_vert_ID),
						   state.assetManager.LoadStacked(AssetHeader::text_frag_ID));      

   state.bitmapFontProgram = CreateTextProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::bitmap_font_vert_ID),
							 state.assetManager.LoadStacked(AssetHeader::bitmap_font_frag_ID));   

   BreakBlockProgram = CreateProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::BreakerBlock_vert_ID),
					       state.assetManager.LoadStacked(AssetHeader::BreakerBlock_frag_ID));   
   
   ButtonProgram = CreateProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::Button_vert_ID),
					   state.assetManager.LoadStacked(AssetHeader::Button_frag_ID));

   SuperBrightProgram = CreateProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::Emissive_vert_ID),
						state.assetManager.LoadStacked(AssetHeader::Emissive_frag_ID));

   Branch_Image_Header *breakHeader = (Branch_Image_Header *)state.assetManager.LoadStacked(AssetHeader::Block_ID).mem;
   state.glState.blockTex = LoadImageIntoTexture(breakHeader);
   // stack->pop();

   Branch_Image_Header *guiTextureMap = (Branch_Image_Header *)state.assetManager.LoadStacked(AssetHeader::GUIMap_ID).mem;
   state.glState.guiTextureMap = LoadImageIntoTexture(guiTextureMap);
   // stack->pop();

   state.glState.startButtonVbo = UploadVertices((float *)GUIMap::GoSign_box_uvs, 6, 2);
   state.glState.playButtonVbo = UploadVertices((float *)GUIMap::play_box_uvs, 6, 2);
   state.glState.pauseButtonVbo = UploadVertices((float *)GUIMap::pause_box_uvs, 6, 2);

   state.glState.backgroundProgram = CreateSimpleProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::Background_vert_ID),
							   state.assetManager.LoadStacked(AssetHeader::Background_frag_ID));

   // stack->pop();
   // stack->pop();

   state.glState.bitmapFont = InitFont_stb(state.assetManager.LoadStacked(AssetHeader::wow_ID), stack);
   Sphere = InitMeshObject(state.assetManager.LoadStacked(AssetHeader::sphere_ID).mem, stack);
   state.sphereGuy.mesh = Sphere;

   RectangleUVBuffer = UploadVertices(RectangleUVs, 6, 2);
   RectangleVertBuffer = UploadVertices(RectangleVerts, 6, 2);
   ScreenVertBuffer = UploadVertices(ScreenVerts, 6, 2);

   GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve, stack);
   GenerateTrackSegmentVertices(LinearTrack, GlobalLinearCurve, stack);
   GenerateTrackSegmentVertices(BreakTrack, GlobalBreakCurve, stack);
   GenerateTrackSegmentVertices(LeftBranchTrack, LEFT_CURVE, stack);
   GenerateTrackSegmentVertices(RightBranchTrack, RIGHT_CURVE, stack);

   InitTextBuffers();

   glGenFramebuffers(1, &state.glState.fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, state.glState.fbo);
   GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
   glDrawBuffers(2, attachments);

   glGenTextures(1, &state.glState.mainColorTexture);
   glBindTexture(GL_TEXTURE_2D, state.glState.mainColorTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, FRAMEBUFFER_FORMAT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.glState.mainColorTexture, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenRenderbuffers(1, &state.glState.depthBuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, state.glState.depthBuffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, state.glState.depthBuffer);

   GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   LOG_WRITE("Framebuffer status %d", status); 

   status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   LOG_WRITE("Framebuffer status %d", status);

   state.glState.fullScreenProgram = CreateSimpleProgramFromAssets(assetManager.LoadStacked(AssetHeader::ScreenTexture_vert_ID),
								   assetManager.LoadStacked(AssetHeader::ScreenTexture_frag_ID));

   assetManager.PopStacked(AssetHeader::ScreenTexture_vert_ID);
   assetManager.PopStacked(AssetHeader::ScreenTexture_frag_ID);

   glGenBuffers(1, &state.glState.buttonVbo);
   glBindBuffer(GL_ARRAY_BUFFER, state.glState.buttonVbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * 6, 0, GL_STATIC_DRAW);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   state.glState.instanceBuffers[0] = CreateInstanceBuffers(LinearTrack.handles.vbo, LinearTrack.handles.nbo);
   state.glState.instanceBuffers[1] = CreateInstanceBuffers(BranchTrack.handles.vbo, BranchTrack.handles.nbo);
   state.glState.instanceBuffers[2] = CreateInstanceBuffers(BreakTrack.handles.vbo, BreakTrack.handles.nbo);

   state.glState.breakTextureInstanceBuffers = CreateTextureInstanceBuffers(RectangleVertBuffer, RectangleUVBuffer);
}

void DeleteResources(OpenglState &gl)
{
   glDeleteFramebuffers(1, &gl.fbo);
   glDeleteRenderbuffers(1, &gl.depthBuffer);
   glDeleteProgram(gl.backgroundProgram);
   glDeleteProgram(gl.fullScreenProgram);
   glDeleteTextures(1, &gl.mainColorTexture);
   glDeleteTextures(1, &gl.blockTex);
   glDeleteTextures(1, &gl.guiTextureMap);
   glDeleteBuffers(1, &gl.buttonVbo);
   glDeleteBuffers(1, &gl.startButtonVbo);
   glDeleteBuffers(1, &gl.playButtonVbo);
   glDeleteBuffers(1, &gl.pauseButtonVbo);
   glDeleteBuffers(1, &gl.bitmapFont.textureHandle);
}
