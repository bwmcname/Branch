
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
      clickUp = 0x3,
   };

   u32 flags;
   v2i clickCoords;

   inline u32 Touched()
   {
      return flags & clickDown;
   }
};

typedef Win32_Input_State Platform_Input_State;

#endif
