
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

void *OnSaveInstanceState(ANativeActivity *activity, size_t *size)
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
   SCREEN_WIDTH = ANativeWindow_getWidth(window);
   SCREEN_HEIGHT = ANativeWindow_getHeight(window);
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

typedef void *(*pthread_func)(void *);

void InitOpengl(AndroidState *state)
{
   __android_log_print(ANDROID_LOG_INFO, "Branch", "Beginning Opengl init.");
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

#ifdef DEBUG
   if(success == EGL_FALSE)
   {
      assert(false);
   }
#endif   
}

void AndroidMain(AndroidState *state)
{
   LOG_WRITE("Worker thread: %ld", pthread_self());
   bool drawing = false;
   GameState gameState;
   bool queueReady = false;

   // state.aLooper = ALooperPrepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

   timespec begin, end;

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
	       gameState.input = {};	       
	       delta = 1.0f;
	    }break;

	    case NATIVEWINDOWCREATED:
	    {
	       InitOpengl(state);
	       glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	       LOG_WRITE("BEGIN INIT");
	       GameInit(gameState);
	       
	       drawing = true;
	    }break;

	    case RESUME:
	    {
	       LOG_WRITE("RESUME");
	    }break;

	    case SAVESTATE:
	    {
	       LOG_WRITE("SAVESTATE");
	    }break;

	    case PAUSE:
	    {
	       LOG_WRITE("PAUSE");
	    }break;

	    case STOP:
	    {
	       LOG_WRITE("STOP");
	    }break;

	    case DESTROY:
	    {
	       LOG_WRITE("DESTROY");
	    }break;

	    case WINDOWFOCUSCHANGE:
	    {
	       LOG_WRITE("WINDOWFOCUSCHANGE");
	    }break;

	    case NATIVEWINDOWDESTROYED:
	    {
	       LOG_WRITE("NATIVEWINDOWDESTROYED");
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
      
      if(drawing)
      {
	 GameLoop(gameState);
	 eglSwapBuffers(state->display, state->surface);
	 // eglSwapInterval(state->display, 0);
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
   }   
}

void *CreateApp(ANativeActivity *activity)
{
   AndroidState *state = (AndroidState *)malloc(sizeof(AndroidState));
   state->events = CreateAndroidEventQueue();

   manager = activity->assetManager;

   LOG_WRITE("Main thread: %ld", pthread_self());

   pthread_t t;
   pthread_attr_t attributes;
   pthread_attr_init(&attributes);
   pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
   pthread_create(&t, &attributes, (pthread_func)AndroidMain, state);

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
