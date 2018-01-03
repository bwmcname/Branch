/* PlatformAPI.h */
/* Each platform must implement the platform API
 *
 * Each platform must provide these aliases
 * BranchFileHandle
 * BranchSaveFileHandle
 *
 * Each platform must implement these structures
 * PlatformInputstate
 *
 * Each platform must provide these functions/macros
 * FileSize(string)
 * FileRead(string, dest, size)
 * FileReadHandle(handle, dest, size, offset)
 * AllocateSystemMemory(size, dest)
 * FreeSystemMemory(mem)
 * FileOpen(filename)
 * OpenSaveFile()
 * CloseSaveFile()
 * WriteSaveFile()
 * ReadSaveFile()
 * ASSET_PATH(filename)
 * FRAMEBUFFER_FORMAT
 * B_ASSERT(val)
 * BEGIN_TIME()
 * END_TIME()
 * READ_TIME(dest)
 *
 * Each platform is also responsible for calling GameInit, and GameLoop 
 * at the appropriate times
 * Each platform should also fill its respective PlatformInputState every frame
 */

#ifdef WIN32_BUILD
// Some functions defined in win32.cpp
#define FileSize(string) (WinFileSize(string))
#define FileRead(string, dest, size) (WinReadFile(string, dest, size))
#define FileReadHandle(handle, dest, size, offset) (WinReadFileHandle(handle, dest, size, offset))
#define AllocateSystemMemory(size, dest) (Win32AllocateMemory(size, dest))
#define FreeSystemMemory(mem) (Win32FreeMemory(mem))
#define FileOpen(fileName) (Win32FileOpen(fileName))
#define ASSET_PATH(filename) ("ProcessedAssets/" ## filename)
#define FRAMEBUFFER_FORMAT GL_RGBA16F
#define USABLE_SCREEN_BOTTOM(state) (-1.0f)
#define PLATFORM_NAVIGATION_GUI_UP(state) (0)

#ifdef DEBUG
#define B_ASSERT(val) ((val) ? 0 : *((char *)0) = 'x')
#else
#define B_ASSERT(val)
#endif

#if defined(TIMERS)
#define BEGIN_TIME() u64 LOCAL_TIME = __rdtsc()
#define END_TIME() LOCAL_TIME = __rdtsc() - LOCAL_TIME
#define READ_TIME(into) into = LOCAL_TIME
#else
#define BEGIN_TIME() 
#define END_TIME() 
#define READ_TIME(into)
#endif

typedef HANDLE BranchFileHandle;

struct Win32InputState
{
   enum
   {
      clickDown = 0x1,
      clickHold = 0x2,
      clickUp = 0x4,
      escapeDown = 0x8,
      escapeHeld = 0x10,
      escapeUp = 0x20
   };

   u32 flags;
   v2i clickCoords;

   #ifdef DEBUG
   inline void SetReleased()
   {
      flags |= clickUp;
   }

   inline void SetTouched()
   {
      flags |= clickDown;
   }
   #endif

   inline u32 Touched()
   {
      return flags & clickDown;
   }

   inline u32 UnTouched()
   {
      return flags & clickUp;
   }

   inline v2i TouchPoint()
   {
      return clickCoords;
   }

   inline u32 Escaped()
   {
      return flags & escapeDown;
   }

   inline u32 UnEscaped()
   {
      return flags & escapeUp;
   }

   inline u32 EscapeHeld()
   {
      return flags & escapeHeld;
   }

   inline void reset()
   {
      flags = 0;
      clickCoords = {};
   }
};

typedef Win32InputState PlatformInputState;

#elif ANDROID_BUILD
// some functions defined in Android.cpp
static AAsset *AndroidFileOpen(char *filename);
static size_t AndroidFileSize(char *filename);

#define FileSize(fileName) AndroidFileSize(fileName)
#define FileRead(string, dest, size) LOG_WRITE("ERROR, deprecated read from filename: %d", __LINE__)
#define FileReadHandle(handle, dest, size, offset) AndroidFileRead(handle, dest, size, offset)
#define AllocateSystemMemory(size, dest) AndroidAllocateSystemMemory(size, dest)
#define FreeSystemMemory(mem)
#define FileOpen(fileName) AndroidFileOpen(fileName)
#define ASSET_PATH(filename) (filename)
#define USABLE_SCREEN_BOTTOM(state) (AndroidUsableBottom(state.android))
#define PLATFORM_NAVIGATION_GUI_UP(state) (IsNavigationBarUp(state.android))

// currently opengles3 does not support floating point framebuffers
#define FRAMEBUFFER_FORMAT GL_RGBA

#ifdef DEBUG
#define B_ASSERT(x) if (!(x)) __android_log_assert("Assertion Failed", "Branch", "%s: %d", __FILE__, __LINE__);
#else
#define B_ASSERT(x)
#endif

#if defined(TIMERS)
#define BEGIN_TIME() timespec LOCAL_BEGIN_TIME, LOCAL_END_TIME;	\
   clock_gettime(CLOCK_MONOTONIC, &LOCAL_BEGIN_TIME)		
#define END_TIME() clock_gettime(CLOCK_MONOTONIC, &LOCAL_END_TIME);	\
   LONG LOCAL_TIME = LOCAL_END_TIME->tv_nsec - LOCAL_BEGIN_TIME->tv_nsec
#else
#define BEGIN_TIME() 
#define END_TIME() 
#define READ_TIME(into)
#endif

typedef AAsset * BranchFileHandle;

struct AndroidInputState
{
   enum : u32
   {
      touched = 0x1,
      released = 0x2,
      held = 0x3
   };

   u32 flags;
   v2i touchCoords;



   inline u32 Touched()
   {
      return flags & touched;
   }

   inline u32 UnTouched()
   {
      return flags & released;
   }

   inline v2i TouchPoint()
   {
      return touchCoords;
   }

   inline u32 Escaped()
   {
      return 0;
   }

   inline u32 UnEscaped()
   {
      return 0;
   }

   inline u32 EscapeHeld()
   {
      return 0;
   }

   inline void reset()
   {
      flags = 0;
      touchCoords = {};
   }

#ifdef DEBUG
   inline void SetReleased()
   {
      flags |= released;
   }

   inline void SetTouched()
   {
      flags |= touched;
   }
#endif
};

typedef AndroidInputState PlatformInputState;

#endif
