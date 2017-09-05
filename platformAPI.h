
#ifdef WIN32_BUILD
#define FileSize(string) (WinFileSize(string))
#define FileRead(string, dest, size) (WinReadFile(string, dest, size))
#define FileReadHandle(handle, dest, size, offset) (WinReadFileHandle(handle, dest, size, offset))
#define AllocateSystemMemory(size, dest) (Win32AllocateMemory(size, dest))
#define FreeSystemMemory(mem) (Win32FreeMemory(mem))
#define FileOpen(fileName) (Win32FileOpen(fileName))
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

#endif
