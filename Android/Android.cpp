
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
u8 *AndroidAllocateSystemMemory(size_t size, size_t *outSize)
{
   // return enough memory pages to fit size
   int pageSize = getpagesize();
   int pages;

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
   __android_log_print(ANDROID_LOG_INFO, "Branch", "START");

   AndroidCommand command;
   command.command = AndroidCommand::START;
   state->events.Push(command);
}

void OnResume(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)activity->instance;
   __android_log_print(ANDROID_LOG_INFO, "Branch", "RESUME");
   state->events.Push({AndroidCommand::RESUME});
}

void *OnSaveInstanceState(ANativeActivity *activity, unsigned long *size)
{
   AndroidState *state = (AndroidState *)activity->instance;

   state->events.Push({AndroidCommand::SAVESTATE});

   return 0;
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
   AndroidState *state = (AndroidState *)activity->instance;
   
   state->events.Push({AndroidCommand::NATIVEWINDOWDESTROYED});
}

void OnInputQueueCreated(ANativeActivity *activity, AInputQueue *queue)
{
   AndroidState *state = (AndroidState *)activity->instance;

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

typedef void *(*pthread_func)(void *);

void InitOpengl(AndroidState *state)
{
   __android_log_print(ANDROID_LOG_INFO, "Branch", "Beginning Opengl init.");
   EGLint attribs[] = {
      EGL_BUFFER_SIZE, 4, // rgba
      EGL_RED_SIZE, 8, // try to get hdr buffer.
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };

   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   eglInitialize(state->display, 0, 0);

   EGLConfig config;
   EGLint nConfigs;
   EGLint format;
   EGLBoolean success;
   eglChooseConfig(state->display, attribs, &config, 1, &nConfigs);
   __android_log_print(ANDROID_LOG_INFO, "Branch", "num configs: %d", nConfigs);
   eglGetConfigAttrib(state->display, config, EGL_NATIVE_VISUAL_ID, &format);
   
   ANativeWindow_setBuffersGeometry(state->window, 0, 0, format);
   __android_log_print(ANDROID_LOG_INFO, "Branch", "wut");
   state->surface = eglCreateWindowSurface(state->display, config, state->window, 0);
   state->context = eglCreateContext(state->display, config, 0, 0);      
   success = eglMakeCurrent(state->display, state->surface, state->surface, state->context);

#ifdef DEBUG
   if(!success)
   {
      __android_log_print(ANDROID_LOG_INFO, "Branch", "Unable to initialize opengl.");
      assert(false);
   }
#endif   
}

void AndroidMain(AndroidState *state)
{
   bool drawing = false;
   GameState gameState;
   for(;;)
   {
      AndroidCommand command;      
      if(state->events.Pop(&command))
      {
	 switch(command.command)
	 {
	    case START:
	    {
	       gameState.input = {};
	       GameInit(gameState);
	       delta = 1.0f;
	    }break;

	    case NATIVEWINDOWCREATED:
	    {
	       InitOpengl(state);
	       glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	       drawing = true;
	    }break;
	 }
      }

      if(drawing)
      {
	 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	 GameLoop(gameState);

	 eglSwapBuffers(state->display, state->surface);
      }
   }   
}

void *CreateApp(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)malloc(sizeof(AndroidState));
   state->events = CreateAndroidEventQueue();

   manager = activity->assetManager;

   pthread_t t;
   pthread_create(&t, 0, (pthread_func)AndroidMain, state);

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
   activity->instance = CreateApp(activity);
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
