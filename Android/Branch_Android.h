
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

struct AndroidState
{
   AndroidQueue events;
   ANativeWindow *window;
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
};
