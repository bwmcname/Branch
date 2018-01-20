
enum AndroidSysCommand
{
   START,
   RESUME,
   SAVESTATE,
   PAUSE,
   STOP,
   DESTROY,
   WINDOWFOCUSCHANGE,
   NATIVEWINDOWCREATED,
   NATIVEWINDOWRESIZED,
   NATIVEWINDOWREDRAWNEEDED,
   NATIVEWINDOWDESTROYED,
   INPUTQUEUECREATED,
   INPUTQUEUEDESTROYED,
   CONTENTRECTCHANGED,
   CONFIGURATIONCHANGED,
   LOWMEMORY
};

enum dpi
{
   LDPI,
   MDPI,
   HDPI,
   XHDPI,
   XXHDPI,
   XXXHDPI,
};

struct AndroidCommand
{
   enum : u32
   {
      START,
      RESUME,
      SAVESTATE,
      PAUSE,
      STOP,
      DESTROY,
      WINDOWFOCUSCHANGE,
      NATIVEWINDOWCREATED,
      NATIVEWINDOWRESIZED,
      NATIVEWINDOWREDRAWNEEDED,
      NATIVEWINDOWDESTROYED,
      INPUTQUEUECREATED,
      INPUTQUEUEDESTROYED,
      CONTENTRECTCHANGED,
      CONFIGURATIONCHANGED,
      LOWMEMORY
   };

   u32 command;

   union
   {
   };
};

struct AndroidQueue
{
   pthread_mutex_t mut;
   AndroidCommand *commands;
   u32 head;
   u32 tail;
   u32 capacity;

   inline void Push(AndroidCommand command);
   inline bool Pop(AndroidCommand *out);
   inline u32 Increment(u32 ptr);
   inline bool Empty();
};

struct RebuildState; // from Main.h

struct AndroidState
{
   ANativeActivity *activity;
   AndroidQueue events;
   ANativeWindow *window;
   AConfiguration *configuration;
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   AInputQueue *iQueue;
   sem_t savingStateSem;
   sem_t sleepingSem; // rendering/game thread waits on this once app loses focus.

   size_t savedStateSize;
   RebuildState *savedState;
   bool started;
   int hasFocus;
   dpi density;
};
