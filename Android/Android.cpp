
AAssetManager *manager;

// @implement!!!
static inline
u64 bclock()
{
   return 0x67123C4AA8F234AD;
}

static inline
AAsset *AndroidFileOpen(char *filename)
{
   return AAssetManager_open(manager, filename, AASSET_MODE_STREAMING);
}

static inline
size_t AndroidFileSize(char *filename)
{
   AAsset *file = AAssetManager_open(manager, filename, AASSET_MODE_UNKNOWN);
   return AAsset_getLength64(file);
}

static inline
void AndroidFileRead(AAsset *asset, u8 *dest, size_t size, size_t offset)
{
   AAsset_seek(asset, offset, SEEK_SET);
   AAsset_read(asset, dest, size);
}

static inline
u8 *AndroidAllocateSystemMemory(size_t size, size_t *outSize)
{
   // return enough memory pages to fit size
   size_t pageSize = getpagesize();
   size_t pages;

   if(size == pageSize)
   {
      pages = 1;
   }
   else
   {
      pages = (size / pageSize) + 1;
   }
   
   return (u8 *)malloc(pages * pageSize);
}

AndroidQueue CreateAndroidEventQueue()
{
   AndroidQueue result;
   
   // @ 20 for now, will re-adjust later to account for the actual number of commands that go through      
   result.head = 0;
   result.tail = 0;
   result.capacity = 20;
   result.commands = (AndroidCommand *)malloc(sizeof(AndroidCommand) * result.capacity);

   pthread_mutex_init(&result.mut, 0);

   return result;
}

void OnStart(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;

   if(state->started)
   {
      sem_post(&state->sleepingSem); // wake up the rendering/game thread.
   }
   else
   {
      state->started = true;
   }
   
   LOG_WRITE("START");

   AndroidCommand command;
   command.command = AndroidCommand::START;
   state->events.Push(command);
}

void OnResume(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;
   LOG_WRITE("RESUME");
   state->events.Push({AndroidCommand::RESUME});   
}

void *OnSaveInstanceState(ANativeActivity *activity, size_t *size)
{
   AndroidState *state = (AndroidState *)activity->instance;

   state->savedState = 0;
   state->events.Push({AndroidCommand::SAVESTATE});
   sem_wait(&state->savingStateSem);

   if(!state->savedState) LOG_WRITE("SAVE FAILED");
   RebuildState *saved = state->savedState;
   *size = sizeof(RebuildState);

   LOG_WRITE("State Saved YAY");

   return saved;
}

typedef void *(*pthread_func)(void *);

void SetScreenConfiguration(AndroidState *state)
{
   JNIEnv* env;
   state->activity->vm->AttachCurrentThread(&env, 0);
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   if(!env)
   {
      LOG_WRITE("Could not get java environment");
      return;
   }

   jclass activityClass = env->FindClass("android/app/NativeActivity");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   jclass windowClass = env->FindClass("android/view/Window");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   jclass viewClass = env->FindClass("android/view/View");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   jmethodID setSystemUiVisibility = env->GetMethodID(viewClass, "setSystemUiVisibility", "(I)V");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   jobject window = env->CallObjectMethod(state->activity->clazz, getWindow);
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   jobject decorView = env->CallObjectMethod(window, getDecorView);
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   jfieldID fullScreenField = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   jfieldID hideNavigationField = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   jfieldID immersiveField = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_IMMERSIVE", "I");
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   int fullScreenFlag = env->GetStaticIntField(viewClass, fullScreenField);
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   int hideNavigationFlag = env->GetStaticIntField(viewClass, hideNavigationField);
   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   int immersiveFlag = env->GetStaticIntField(viewClass, immersiveField);

   LOG_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);

   env->CallVoidMethod(decorView, setSystemUiVisibility, fullScreenFlag | hideNavigationFlag | immersiveFlag);
   // jthrowable excep = env->ExceptionOccurred();
   env->ExceptionDescribe();

   //log_WRITE("%i, %i", env->ExceptionCheck(), __LINE__);
   state->activity->vm->DetachCurrentThread();
}

void OnPause(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance; 
   state->events.Push({AndroidCommand::PAUSE});   
}

void OnStop(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::STOP});
}

void OnDestroy(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::DESTROY});   
}

void OnWindowFocusChanged(ANativeActivity *activity, int hasFocus)
{
   AndroidState *state = (AndroidState *)activity->instance;
   state->hasFocus = hasFocus;
   state->events.Push({AndroidCommand::WINDOWFOCUSCHANGE});
}

void OnNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window)
{
   AndroidState *state = (AndroidState *)activity->instance;   

   state->window = window;
   state->events.Push({AndroidCommand::NATIVEWINDOWCREATED});   
}

void OnNativeWindowResized(ANativeActivity *activity, ANativeWindow *window)
{
   AndroidState *state = (AndroidState *)activity->instance;   

   state->events.Push({AndroidCommand::NATIVEWINDOWRESIZED});   
}

void OnNativeWindowRedrawNeeded(ANativeActivity *activity, ANativeWindow *window)
{
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::NATIVEWINDOWREDRAWNEEDED});   
}

void OnNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window)
{
   LOG_WRITE("WINDOW DESTROYED");
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::NATIVEWINDOWDESTROYED});   
}

void OnInputQueueCreated(ANativeActivity *activity, AInputQueue *queue)
{
   AndroidState *state = (AndroidState *)activity->instance;
   
   state->iQueue = queue;
   state->events.Push({AndroidCommand::INPUTQUEUECREATED});   
}

void OnInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue)
{
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::INPUTQUEUEDESTROYED});   
}

void OnContentRectChanged(ANativeActivity *activity, const ARect *rect)
{
   AndroidState *state = (AndroidState *)activity->instance;
   state->events.Push({AndroidCommand::CONTENTRECTCHANGED});
}

void OnConfigurationChanged(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;   
   state->events.Push({AndroidCommand::CONFIGURATIONCHANGED});
}

void OnLowMemory(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;
   state->events.Push({AndroidCommand::LOWMEMORY});
}

static dpi GetDPI(AndroidState *state)
{
   AConfiguration *config = AConfiguration_new();
   AConfiguration_fromAssetManager(config, manager);

   i32 density = AConfiguration_getDensity(config);

   dpi result;

   switch(density)
   {
      case ACONFIGURATION_DENSITY_LOW:
      {
	 result = LDPI;
      }break;

      case ACONFIGURATION_DENSITY_MEDIUM:
      {
	 result = MDPI;
      }break;
      
      case ACONFIGURATION_DENSITY_HIGH:
      {
	 result = HDPI;
      }break;
      
      case ACONFIGURATION_DENSITY_XHIGH:
      {
	 result = XHDPI;
      }break;
      
      case ACONFIGURATION_DENSITY_XXHIGH:
      {
	 result = XXHDPI;
      }break;
      
      case ACONFIGURATION_DENSITY_XXXHIGH:
      {
	 result = XXXHDPI;
      }break;

	 
      case ACONFIGURATION_DENSITY_ANY:
      case ACONFIGURATION_DENSITY_NONE:
      case ACONFIGURATION_DENSITY_DEFAULT:
      case ACONFIGURATION_DENSITY_TV:
      {
	 result = MDPI;
      }break;
   }

   AConfiguration_delete(config);

   return result;
}

static
char *AndroidFileNameToPath(char *filePath, char *name, StackAllocator *allocator)
{
   i32 pathLength = strlen(filePath);
   i32 nameLength = strlen(name);
   char *fullPath = (char *)allocator->push(pathLength + nameLength + 1);
   
   for(i32 i = 0; i < pathLength; ++i) fullPath[i] = filePath[i];
   for(i32 i = 0; i < nameLength; ++i) fullPath[pathLength + i] = name[i];
   fullPath[pathLength + nameLength] = '\0';

   return fullPath;
}

static
bool AndroidSaveGame(GameState &state, StackAllocator *allocator)
{
   const char *name = "/branch.sav";
   char *fullPath = AndroidFileNameToPath((char *)state.android->activity->externalDataPath, (char *)name, allocator);
   int fd = creat(fullPath, O_WRONLY);

   // free filename
   allocator->pop();

   if(fd != -1)
   {
      // right now we only want to store the save file and
      // highschore: strlen(branch) + sizeof(u32)
      u32 size = 6 + sizeof(float);
      u8 *buffer = allocator->push(size);

      buffer[0] = 'B';
      buffer[1] = 'R';
      buffer[2] = 'A';
      buffer[3] = 'N';
      buffer[4] = 'C';
      buffer[5] = 'H';

      *((u32 *)(buffer + 6)) = state.maxDistance;

      if(write(fd, buffer, size) == size)
      {
	 close(fd);
	 allocator->pop();
	 return true;
      }

      close(fd);
      allocator->pop();
   }

   return false;
}

static
u8 *AndroidGetSaveFileBuffer(GameState &state, StackAllocator *allocator, u32 *outSize)
{
   const char *name = "/branch.sav";
   char *fullPath = AndroidFileNameToPath((char *)state.android->activity->externalDataPath, (char *)name, allocator);

   if(access(fullPath, R_OK) == 0)
   {
      int fd = open(fullPath, O_RDONLY);

      // pop filename
      allocator->pop();

      if(fd != -1)
      {
	 struct stat fileInfo;
	 if(fstat(fd, &fileInfo) == 0)
	 {
	    *outSize = fileInfo.st_size;
	    u8 *buffer = allocator->push(*outSize);
	    if(read(fd, buffer, *outSize) == *outSize)
	    {
	       close(fd);
	       return buffer;
	    }
	    allocator->pop();
	 }
	 close(fd);
      }
   }
   else
   {
      // pop filename
      allocator->pop();
   }
   
   return 0;
}

// This can only be called after setting the screen configuration....
int IsNavigationBarUp(AndroidState *state)
{   
   JNIEnv* env;
   state->activity->vm->AttachCurrentThread(&env, 0);
   
   if(!env)
   {
      LOG_WRITE("Could not get java environment");
      return 0;
   }

   state->activity->vm->AttachCurrentThread(&env, 0);

   jclass activityClass = env->FindClass("android/app/NativeActivity");
   jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");

   jclass windowClass = env->FindClass("android/view/Window");
   jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");

   jclass viewClass = env->FindClass("android/view/View");
   jmethodID getSystemUiVisibility = env->GetMethodID(viewClass, "getSystemUiVisibility", "()I");

   jobject window = env->CallObjectMethod(state->activity->clazz, getWindow);
   jobject decorView = env->CallObjectMethod(window, getDecorView);

   jfieldID hideNavigationField = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");

   int flags = env->CallIntMethod(decorView, getSystemUiVisibility);
   int hideNavigationFlag = env->GetStaticIntField(viewClass, hideNavigationField);
   state->activity->vm->DetachCurrentThread();

   return !(flags & hideNavigationFlag);
}

float NavigationBarHeight(AndroidState *state)
{
   JNIEnv *env;
   state->activity->vm->AttachCurrentThread(&env, 0);

   jclass contextClass = env->FindClass("android/content/Context");
   jclass resourcesClass = env->FindClass("android/content/res/Resources");
   
   jmethodID getResources = env->GetMethodID(contextClass, "getResources", "()Landroid/content/res/Resources;");
   jobject resourcesObject = env->CallObjectMethod(state->activity->clazz, getResources);

   jmethodID getIdentifier = env->GetMethodID(resourcesClass, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
   
   jstring identifier = env->NewStringUTF("navigation_bar_height");
   jstring defType = env->NewStringUTF("dimen");
   jstring defPackage = env->NewStringUTF("android");

   int heightIdentifier = env->CallIntMethod(resourcesObject, getIdentifier, identifier, defType, defPackage);

   jmethodID getDimensionPixelSize = env->GetMethodID(resourcesClass, "getDimensionPixelSize", "(I)I");
   float height = (float)env->CallIntMethod(resourcesObject, getDimensionPixelSize, heightIdentifier);

   state->activity->vm->DetachCurrentThread();   

   float screenHeight = (float)ANativeWindow_getHeight(state->window);
   float ndcSize = (height / screenHeight);
   return ndcSize;
}

static inline
float AndroidUsableBottom(AndroidState *state)
{
   if(IsNavigationBarUp(state))
   {
      float height = NavigationBarHeight(state);

      return -1.0f + height;
   }
   else
   {
      return -1.0f;
   }
}

void InitOpengl(AndroidState *state)
{
   LOG_WRITE("Beginning Opengl init.");

   EGLint attribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      //EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
      EGL_NONE
   };

   EGLint glAttribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
   };

   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   eglInitialize(state->display, 0, 0);   
   
   EGLConfig config;
   EGLint nConfigs;
   EGLint format;
   EGLBoolean success;
   eglChooseConfig(state->display, attribs, &config, 1, &nConfigs);   
   eglGetConfigAttrib(state->display, config, EGL_NATIVE_VISUAL_ID, &format);
   state->surface = eglCreateWindowSurface(state->display, config, state->window, 0);
   state->context = eglCreateContext(state->display, config, 0, glAttribs);
   success = eglMakeCurrent(state->display, state->surface, state->surface, state->context);

   LOG_WRITE("%s", eglQueryString(state->display, EGL_EXTENSIONS));   

#ifdef DEBUG
   if(success == EGL_FALSE)
   {      
      assert(false);
   }
#endif   
}

void RebindContext(AndroidState *state)
{
   eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglDestroySurface(state->display, state->surface);
   
   EGLint attribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_NONE
   };

   EGLint glAttribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
   };
   
   EGLConfig config;
   EGLint nConfigs;
   EGLBoolean success;
   eglChooseConfig(state->display, attribs, &config, 1, &nConfigs);   
   state->surface = eglCreateWindowSurface(state->display, config, state->window, 0);
   success = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
}

void DestroyDisplay(AndroidState *state)
{
   if(state->display != EGL_NO_DISPLAY)
   {
      eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if(state->context != EGL_NO_CONTEXT)
      {
	 eglDestroyContext(state->display, state->context);
      }
      if(state->surface != EGL_NO_SURFACE)
      {
	 eglDestroySurface(state->display, state->surface);
      }
      eglTerminate(state->display);
   }

   state->display = EGL_NO_DISPLAY;
   state->context = EGL_NO_CONTEXT;
   state->surface = EGL_NO_SURFACE;

   LOG_WRITE("DISPLAY DESTROYED");
}

// Android state flags
#define BRANCH_INITIALIZED 0x1
#define BRANCH_GL_LOADED 0x2
#define BRANCH_ACTIVE 0x4
#define BRANCH_INPUT_READY 0x8

void SetupAds(AndroidState *state)
{
   JNIEnv* env;
   jint result = state->activity->vm->AttachCurrentThread(&env, 0);   

   if(result == JNI_OK && !env->ExceptionCheck())
   {
      firebase::App *app = firebase::App::Create(env,
						 state->activity->clazz);
      const char* kAdMobAppID = "ca-app-pub-XXXXXXXXXXXXXXXX~NNNNNNNNNN";
      firebase::admob::Initialize(*app, kAdMobAppID);
      state->activity->vm->DetachCurrentThread();
   }
}

void LoadAndShowAds(AndroidState *state)
{
   static const char *testIDS[] = {
      "2077ef9a63d2b398840261c8221a0c9b",
      "098fe087d987c9a878965454a65654d7"
   };

   static const char *testKeywords[] = {
      "Arcade", "Action", "Infinite Runner"
   };

   const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/1033173712";

   firebase::admob::AdRequest request;
   request.test_device_ids = testIDS;
   request.test_device_id_count = 2;
   request.keywords = testKeywords;
   request.keyword_count = 3;
   request.extras = 0;
   request.extras_count = 0;
   request.birthday_day = 20;
   request.birthday_month = 10;
   request.birthday_year = 1995;
   request.gender = firebase::admob::kGenderUnknown;
   request.tagged_for_child_directed_treatment = firebase::admob::kChildDirectedTreatmentStateNotTagged;

   firebase::admob::InterstitialAd *ad = new firebase::admob::InterstitialAd();
   ad->Initialize(state->activity->clazz,
		  kInterstitialAdUnit);

   while(ad->InitializeLastResult().status() != firebase::kFutureStatusComplete);
   if(ad->InitializeLastResult().error() == firebase::admob::kAdMobErrorNone)
   {
      ad->LoadAd(request);
   }

   while(ad->InitializeLastResult().status() != firebase::kFutureStatusComplete);
   if(ad->InitializeLastResult().error() == firebase::admob::kAdMobErrorNone)
   {
      ad->Show();
   }
}

void AndroidMain(AndroidState *state)
{
   bool drawing = false;
   GameState gameState;
   bool queueReady = false;
   timespec begin, end;
   // resume always gets called twice in a row for some reason.
   bool resumeToggle = true;

   bool running = false;
   bool reloadGL = false;
   bool resumed = false;

   i32 flags = 0;

   LOG_WRITE("SETUP FIREBASE");
   SetupAds(state);
   
   for(;;)
   {
      clock_gettime(CLOCK_MONOTONIC, &begin);
      AndroidCommand command;

      if(state->events.Pop(&command))
      {
	 switch(command.command)
	 {
	    case START:
	    {
	       if((flags & BRANCH_INITIALIZED) == 0)
	       {
		  LOG_WRITE("BEGIN INIT");
		  gameState.android = state;
		  GameInit(gameState, state->savedState, state->savedStateSize);
		  flags |= BRANCH_INITIALIZED;
	       }

	       if(state->savedState)
	       {
		  // Have to re-initialize our opengl context.
		  // flags &= ~BRANCH_GL_LOADED;

		  if(!running)
		  {
		     LOG_WRITE("Reloading state");
		     // have to do a full reload
		     ReloadState((RebuildState *)state->savedState, gameState);
		  }
	       }

	       running = true;
	    }break;

	    case NATIVEWINDOWCREATED:
	    {
	       LOG_WRITE("NATIVE WINDOW CREATED");
	       if((flags & BRANCH_GL_LOADED) == 0)
	       {
		  InitOpengl(state);
		  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		  gameState.input = {};
		  gameState.framerate = 60;
		  delta = 1.0f;

		  SCREEN_WIDTH = ANativeWindow_getWidth(state->window);
		  SCREEN_HEIGHT = ANativeWindow_getHeight(state->window);
		  CreateProjection();
		  LoadGLState(gameState, (StackAllocator *)gameState.mainArena.base, gameState.assetManager);
   
		  flags |= BRANCH_GL_LOADED;
	       }
	       else
	       {
		  EGLint value;
		  if(!eglQueryContext(state->display, state->context, EGL_CONTEXT_CLIENT_VERSION, &value))
		  {
		     LOG_WRITE("Bad context");
		     InitOpengl(state);
		     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		     gameState.input = {};
		     gameState.framerate = 60;
		     delta = 1.0f;

		     LoadGLState(gameState, (StackAllocator *)gameState.mainArena.base, gameState.assetManager);	       
		  }
		  else
		  {
		     RebindContext(state);
		  }
	       }
	    }break;

	    case RESUME:
	    {
	       delta = 1.0f;
	       resumed = true;
	       flags |= BRANCH_ACTIVE;
	    }break;

	    case SAVESTATE:
	    {
	       LOG_WRITE("SAVESTATE");

	       state->savedState = SaveState(&gameState);
	       state->savedStateSize = sizeof(RebuildState);

	       B_ASSERT(state->savedState);
	       sem_post(&state->savingStateSem);
	    }break;

	    case PAUSE:
	    {
	       LOG_WRITE("PAUSE");
	       flags &= ~BRANCH_ACTIVE;

	       if(gameState.state == GameState::LOOP)
	       {
		  gameState.state = GameState::PAUSE;
	       }
	    }break;

	    case STOP:
	    {
	       LOG_WRITE("STOP");
	       resumed = false;
	       sem_wait(&state->sleepingSem);
	    }break;

	    case DESTROY:
	    {
	       LOG_WRITE("DESTROY");
	    }break;

	    case WINDOWFOCUSCHANGE:
	    {
	       drawing = true;
	       LOG_WRITE("WINDOWFOCUSCHANGE");
	       if(eglGetCurrentSurface(EGL_READ) == EGL_NO_SURFACE)
	       {
		  LOG_WRITE("This does not work ehhh");
	       }

	       if(state->hasFocus)
	       {
		  // SetScreenConfiguration(state);
	       }
	    }break;

	    case NATIVEWINDOWDESTROYED:
	    {
	       LOG_WRITE("DISPLAY DESTROYED");
	       drawing = false;
	       DestroyDisplay(state);
	    }break;

	    case NATIVEWINDOWRESIZED:
	    {
	       LOG_WRITE("NATIVEWINDOWRESIZED");
	    }break;

	    case NATIVEWINDOWREDRAWNEEDED:
	    {
	       LOG_WRITE("NATIVEREDRAWNEEDED");
	    }break;

	    case INPUTQUEUECREATED:
	    {
	       queueReady = true;
	       LOG_WRITE("INPUTQUEUECREATED");
	    }break;

	    case INPUTQUEUEDESTROYED:
	    {
	       LOG_WRITE("INPUTQUEUEDESTROYED");
	    }break;

	    case CONTENTRECTCHANGED:
	    {
	       LOG_WRITE("CONTENTRECTCHANGED");
	    }break;

	    case CONFIGURATIONCHANGED:
	    {
	       LOG_WRITE("CONFIGURATIONCHANGED");
	    }break;

	    case LOWMEMORY:
	    {
	       LOG_WRITE("LOWMEMORY");
	    }break;

	    default:
	    {
	       LOG_WRITE("WUT");
	       assert(false);
	    }break;
	 }
      }

      // Handle input
      if(queueReady)
      {	 
	 AInputEvent *event;
	 i32 success;
	 bool interacted = false;

	 if(gameState.input.Touched())
	 {
	    gameState.input.flags |= AndroidInputState::held;
	 }

	 if(gameState.input.UnTouched())
	 {
	    gameState.input.flags = 0;
	 }

	 while(AInputQueue_hasEvents(state->iQueue))
	 {
	    AInputQueue_getEvent(state->iQueue, &event);	    
	    if(!AInputQueue_preDispatchEvent(state->iQueue, event))
	    {	       
	       switch (AInputEvent_getType(event))
	       {
		  case AINPUT_EVENT_TYPE_MOTION:
		  {
		     switch(AMotionEvent_getAction(event))
		     {
			case AMOTION_EVENT_ACTION_DOWN:
			{
			   gameState.input.flags |= AndroidInputState::touched;

			   gameState.input.touchCoords = {(i32)AMotionEvent_getX(event, 0),
							  (i32)AMotionEvent_getY(event, 0)};
			}break;

			case AMOTION_EVENT_ACTION_UP:
			{
			   gameState.input.flags &= ~AndroidInputState::touched;
			   gameState.input.flags |= AndroidInputState::released;

			   gameState.input.touchCoords = {(i32)AMotionEvent_getX(event, 0),
							  (i32)AMotionEvent_getY(event, 0)};
			}break;
		     }
		  } break;
	       }
	       /* handle input */
	       AInputQueue_finishEvent(state->iQueue, event, 1);
	    }
	 }
      }

      if((flags & (BRANCH_GL_LOADED | BRANCH_INITIALIZED | BRANCH_ACTIVE)) == (BRANCH_GL_LOADED | BRANCH_INITIALIZED | BRANCH_ACTIVE))
      {	 
	 GameLoop(gameState);
	 eglSwapBuffers(state->display, state->surface);
      }

      clock_gettime(CLOCK_MONOTONIC, &end);

      float total;
      if(end.tv_sec == begin.tv_sec)
      {	 
	 total = (float)(end.tv_nsec - begin.tv_nsec);	 
      }
      else
      {
	 long seconds = end.tv_sec - begin.tv_sec;
	 total = (float)(((seconds * 1000000000) - begin.tv_nsec) + end.tv_nsec);
      }
      float totalMillis = total / 1000000.0f;
      delta = totalMillis / 16.6666666667f;
      gameState.framerate = 60.0f / delta;

      // we dont want framerate delta to be less than 24fps
      delta = min(delta, 2.5f);
   }   
}

void *CreateApp(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
   if(savedState)
   {
      LOG_WRITE("Load State");
   }

   AndroidState *state = (AndroidState *)malloc(sizeof(AndroidState));
   state->activity = activity;
   state->events = CreateAndroidEventQueue();
   state->savedState = (RebuildState *)savedState;
   state->savedStateSize = savedStateSize;
   sem_init(&state->savingStateSem, 0, 0);
   sem_init(&state->sleepingSem, 0, 0);
   manager = activity->assetManager;
   state->started = false;
   state->hasFocus = 0;
   state->density = GetDPI(state);

   pthread_t t;
   pthread_attr_t attributes;
   pthread_attr_init(&attributes);
   pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
   pthread_create(&t, &attributes, (pthread_func)AndroidMain, state);

   /*
   sched_param sp;
   sp.sched_priority = sched_get_priority_max(SCHED_OTHER);
   int result = pthread_setschedparam(t, SCHED_OTHER, &sp);
   
   if(result != 0)
   {
      LOG_WRITE("Could not set priority.");
   }
   */
   
   return (void *)state;
}

void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
   activity->callbacks->onDestroy = OnDestroy;
   activity->callbacks->onStart = OnStart;
   activity->callbacks->onResume = OnResume;
   activity->callbacks->onSaveInstanceState = OnSaveInstanceState;
   activity->callbacks->onPause = OnPause;
   activity->callbacks->onStop = OnStop;
   activity->callbacks->onConfigurationChanged = OnConfigurationChanged;
   activity->callbacks->onLowMemory = OnLowMemory;
   activity->callbacks->onWindowFocusChanged = OnWindowFocusChanged;
   activity->callbacks->onNativeWindowCreated = OnNativeWindowCreated;
   activity->callbacks->onInputQueueCreated = OnInputQueueCreated;
   activity->callbacks->onInputQueueDestroyed = OnInputQueueDestroyed;
   activity->instance = CreateApp(activity, savedState, savedStateSize);

   LOG_WRITE("START FROM ONCREATE");
}

inline u32
AndroidQueue::Increment(u32 ptr)
{
   if(ptr == capacity-1)
   {
      return 0;
   }
   else
   {
      return ptr + 1;
   }
}

inline void
AndroidQueue::Push(AndroidCommand command)
{
   pthread_mutex_lock(&mut);
   commands[tail] = command;
   tail = Increment(tail);

   pthread_mutex_unlock(&mut);
}

inline bool
AndroidQueue::Pop(AndroidCommand *out)
{
   pthread_mutex_lock(&mut);

   if(head == tail)
   {
      pthread_mutex_unlock(&mut);
      return false;
   }
   
   *out = commands[head];
   head = Increment(head);

   pthread_mutex_unlock(&mut);

   return true;
}
