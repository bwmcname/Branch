
#ifdef WIN32_BUILD
#define FileSize(string) (WinFileSize(string))
#define FileRead(string, dest, size) (WinReadFile(string, dest, size))
#define FileReadHandle(handle, dest, size, offset) (WinReadFileHandle(handle, dest, size, offset))
#define AllocateSystemMemory(size, dest) (Win32AllocateMemory(size, dest))
#define FreeSystemMemory(mem) (Win32FreeMemory(mem))
#define FileOpen(fileName) (Win32FileOpen(fileName))
#define ASSET_PATH(filename) ("ProcessedAssets/" ## filename)
typedef HANDLE BranchFileHandle;

struct Win32_Input_State
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
};

typedef Win32_Input_State Platform_Input_State;

#elif ANDROID_BUILD

static AAsset *AndroidFileOpen(char *filename);
static size_t AndroidFileSize(char *filename);

#define FileSize(fileName) AndroidFileSize(fileName)
#define FileRead(string, dest, size) LOG_WRITE("ERROR, deprecated read from filename: %d", __LINE__)
#define FileReadHandle(handle, dest, size, offset) AndroidFileRead(handle, dest, size, offset)
#define AllocateSystemMemory(size, dest) AndroidAllocateSystemMemory(size, dest)
#define FreeSystemMemory(mem)
#define FileOpen(fileName) AndroidFileOpen(fileName)
#define ASSET_PATH(filename) (filename)

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
};

typedef AndroidInputState Platform_Input_State;

#endif
