
#ifdef WIN32_BUILD
#define FileSize(string) (WinFileSize(string))
#define FileRead(string, dest, size) (WinReadFile(string, dest, size))
#define AllocateSystemMemory(size, dest) (Win32AllocateMemory(size, dest))
#define FreeSystemMemory(mem) (Win32FreeMemory(mem))
#endif
