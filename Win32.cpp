
#define ErrorDialogue(str) MessageBoxEx(0, (LPCTSTR)(str), 0, MB_ICONEXCLAMATION, 0)

static
u64 bclock()
{
   return __rdtsc();
}

// only read right now
static
HANDLE Win32FileOpen(char *filename)
{
   HANDLE FileHandle = CreateFile(filename,
				  GENERIC_READ,
				  FILE_SHARE_READ,
				  0,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
				  0);

   return FileHandle;
}

static
size_t WinFileSize(char *filename)
{
   HANDLE FileHandle = Win32FileOpen(filename);

   if(FileHandle)
   {
      LARGE_INTEGER FileSize;
      GetFileSizeEx(FileHandle, &FileSize);

      CloseHandle(FileHandle);
      return (size_t)FileSize.QuadPart;
   }
   else
   {
      return 0;
   }
}

static
b32 WinReadFileHandle(HANDLE FileHandle, u8 *buffer, size_t fileSize, size_t offset)
{
   if(FileHandle)
   {
      LARGE_INTEGER lOffset;
      lOffset.QuadPart = offset;
      SetFilePointerEx(FileHandle, lOffset, 0, FILE_BEGIN);
      b32 result = ReadFile(FileHandle,
			    buffer,
			    (DWORD)fileSize,
			    0,
			    0);

      return result;
   }
   else
   {
      return false;
   }   
}

static
b32 WinReadFile(char *filename, u8 *buffer, size_t fileSize, size_t offset)
{
   HANDLE FileHandle = CreateFile(filename,
				  GENERIC_READ,
				  FILE_SHARE_READ,
				  0,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
				  0);

   return WinReadFileHandle(FileHandle, buffer, fileSize, offset);
}

static
bool Win32SaveGame(GameState &state, StackAllocator *allocator)
{

   HANDLE saveFile = CreateFile("branch.sav",
				GENERIC_WRITE,
			        0,
				0,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				0);

   if(saveFile != INVALID_HANDLE_VALUE)
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

      if(WriteFile(saveFile, buffer, size, 0, 0))
      {
	 allocator->pop();
	 CloseHandle(saveFile);
	 return true;
      }

      CloseHandle(saveFile);
      allocator->pop();
   }

   return false;
}

static
u8 *Win32GetSaveFileBuffer(GameState &state, StackAllocator *allocator, u32 *outSize)
{
   HANDLE file = Win32FileOpen("branch.sav");

   if(file != INVALID_HANDLE_VALUE)
   {
      LARGE_INTEGER fileSize;
      GetFileSizeEx(file, &fileSize);

      *outSize = fileSize.QuadPart;

      u8 *buffer = allocator->push(*outSize);

      if(WinReadFileHandle(file, buffer, *outSize, 0))
      {
	 CloseHandle(file);
	 return buffer;
      }

      CloseHandle(file);
      allocator->pop();
   }

   return 0;
}

static
u8 *Win32AllocateMemory(size_t desired, size_t *actual)
{
   SYSTEM_INFO info;
   GetSystemInfo(&info);

   size_t pageSize = info.dwPageSize;
   size_t requestedPages;
   
   if(desired > pageSize)
   {
      // for example, if pageSize = 1024 and desired = 2500, then we need 3 pages
      // (2500 / 1024) + 1 = 3
      requestedPages = (desired / pageSize) + 1;      
   }
   else
   {
      requestedPages = 1;
   }

   *actual = requestedPages * pageSize;

   void *chunk = VirtualAlloc(0, *actual, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

   return (u8 *)chunk;
}

static
b32 Win32FreeMemory(u8 *address)
{
   return VirtualFree(address, 0, MEM_RELEASE);
}

LRESULT CALLBACK
WindowProc(HWND hwnd,
	   UINT message,
	   WPARAM wParam,
	   LPARAM lParam)
{
   switch(message)
   {
      case WM_CLOSE:
      {
	 PostQuitMessage(0);
	 return 0;
      }break;
      default:
      {
	 return DefWindowProc(hwnd, message, wParam, lParam);
      }
   }
}

static void *GetGlExtension(const char *name)
{
   void *p = (void *)wglGetProcAddress(name);
   if(p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
      (p == (void*)-1) )
   {      
      p = (void *)GetProcAddress(glDll, name);
   }
   
   return p;
}
 
static void OnWindowsExit()
{
   if(!glDll)
   {
      FreeLibrary(glDll);
   }
}

void GetGlExtensions(HINSTANCE Instance)
{
   // Create Dummy window to create a Dummy gl context to get wgl functions.
   WNDCLASSEX DummyWindowClass = {sizeof(WNDCLASSEX),
				  CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
				  WindowProc,
				  0, 0,
				  Instance,
				  0, 0, 0, 0,
				  "Opengl Dummy Window Class",
				  0};

   RegisterClassEx(&DummyWindowClass);
   
   HWND DummyWindowHandle = CreateWindowEx(0,
					   "Opengl Dummy Window Class",
					   "",
					   WS_OVERLAPPEDWINDOW,
					   CW_USEDEFAULT,
					   CW_USEDEFAULT,
					   0, 0,
					   0, 0, Instance, 0);

   PIXELFORMATDESCRIPTOR DummyPfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,
      32,
      0, 0, 0, 0, 0, 0,
      0,
      0,
      0,
      0, 0, 0, 0,
      24,
      8,
      0,
      PFD_MAIN_PLANE,
      0,
      0, 0, 0
   };
   HDC DummyDc = GetDC(DummyWindowHandle);
   int FormatNumber = ChoosePixelFormat(DummyDc, &DummyPfd);
   SetPixelFormat(DummyDc, FormatNumber, &DummyPfd);
   HGLRC DummyContext = wglCreateContext(DummyDc);
   wglMakeCurrent(DummyDc, DummyContext);
   
   // Now load functions;
   glDll = LoadLibrary("opengl32.dll");
   if(!glDll)
   {
      ErrorDialogue("opengl32.dll not found");
   }

   WinGetGlExtension(wglChoosePixelFormatARB);

   // now delete the context
   wglMakeCurrent(0, 0);
   wglDeleteContext(DummyContext);

   // also delete the window, since windows doesn't allow us to
   // change the pixel format after we set it.
   DestroyWindow(DummyWindowHandle);   
}

int OpenglCreate(HWND WindowHandle)
{   
   const int attributes[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
      WGL_ARB_framebuffer_sRGB, GL_TRUE,
      WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
      0,
   };

   PIXELFORMATDESCRIPTOR pfd = {};
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion = 1;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
   pfd.cColorBits = 32;
   pfd.cAlphaBits = 8;
   pfd.cDepthBits = 24;
   pfd.iLayerType = PFD_MAIN_PLANE;
   
   int pixelFormat;
   UINT numFormats;
   HDC WindowContext = GetDC(WindowHandle);   
   wglChoosePixelFormatARB(WindowContext, attributes, 0, 1, &pixelFormat, &numFormats);

   SetPixelFormat(WindowContext, pixelFormat, &pfd);
   HGLRC OpenglContext = wglCreateContext(WindowContext);
   wglMakeCurrent(WindowContext, OpenglContext);

   if(!OpenglContext)
   {
      ErrorDialogue("Could Not Initialize Opengl");
   }

   WinGetGlExtension(glCreateProgram);
   WinGetGlExtension(glCreateShader);
   WinGetGlExtension(glShaderSource);
   WinGetGlExtension(glAttachShader);
   WinGetGlExtension(glLinkProgram);
   WinGetGlExtension(glGetAttribLocation);
   WinGetGlExtension(glGetUniformLocation);
   WinGetGlExtension(glUseProgram);
   WinGetGlExtension(glVertexAttrib4fv);
   WinGetGlExtension(glVertexAttribPointer);
   WinGetGlExtension(glCompileShader);
   WinGetGlExtension(glGenVertexArrays);
   WinGetGlExtension(glBindVertexArray);
   WinGetGlExtension(glGenBuffers);
   WinGetGlExtension(glBindBuffer);
   WinGetGlExtension(glBufferData);
   WinGetGlExtension(glEnableVertexAttribArray);
   WinGetGlExtension(glUniform2ui);
   WinGetGlExtension(glUniform2f);
   WinGetGlExtension(glUniform3fv);
   WinGetGlExtension(glUniformMatrix2fv);
   WinGetGlExtension(glUniformMatrix3fv);
   WinGetGlExtension(glUniformMatrix4fv);
   WinGetGlExtension(glUniform1i);
   WinGetGlExtension(glActiveTexture);
   WinGetGlExtension(glUniform1f);
   WinGetGlExtension(glBufferSubData);
   WinGetGlExtension(glDeleteBuffers);
   WinGetGlExtension(glGenFramebuffers);
   WinGetGlExtension(glBindFramebuffer);
   WinGetGlExtension(glFramebufferTexture2D);
   WinGetGlExtension(glCheckFramebufferStatus);
   WinGetGlExtension(glBlitFramebuffer);
   WinGetGlExtension(glGenRenderbuffers);
   WinGetGlExtension(glBindRenderbuffer);
   WinGetGlExtension(glFramebufferRenderbuffer);
   WinGetGlExtension(glRenderbufferStorage);
   WinGetGlExtension(glDrawBuffers);
   WinGetGlExtension(glDrawArraysInstanced);
   WinGetGlExtension(glVertexAttribDivisor);
   WinGetGlExtension(glDisableVertexAttribArray);
   WinGetGlExtension(glInvalidateFramebuffer);
   WinGetGlExtension(glMapBufferRange);
   WinGetGlExtension(glUnmapBuffer);
   WinGetGlExtension(glBufferStorage);
   WinGetGlExtension(glDeleteFramebuffers);
   WinGetGlExtension(glDeleteRenderbuffers);
   WinGetGlExtension(glDeleteProgram);

   return true;
}

THREAD_FUNC TestThread(THREAD_PARAMS)
{
   return THREAD_RETURN_VALUE;
}

#if 0
void WinInitializeThreads(GameState &state, StackAllocator *allocator)
{
   SYSTEM_INFO info;
   GetSystemInfo(&info);

   // state.threadCount = info.dwNumberOfProcessors;
   // state.threadPool = (Thread *)allocator->push(sizeof(Thread));

   for(i32 i = 0; i < state.threadCount; ++i)
   {
      HANDLE handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TestThread, 0, 0, 0);
      if(handle)
      {
	 state.threadPool[i] = {handle};
      }
      else
      {
	 assert(0);
      }
   }
}
#endif

int CALLBACK WinMain(HINSTANCE Instance,
		     HINSTANCE Previous,
		     LPSTR CommandLine,
		     int CommandShow)
{
   //@Temporary!!!
   SCREEN_WIDTH = 480;
   SCREEN_HEIGHT = 640;

   GetGlExtensions(Instance);
   
   WNDCLASSEX WindowClass = {sizeof(WNDCLASSEX),
 			     CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
			     WindowProc,
			     0, 0,
			     Instance,
			     0, 0, 0, 0,
			     "Branch Debug Window Class",
			     0};

   WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);

   if(!RegisterClassEx(&WindowClass))
   {
      ErrorDialogue("Unable to Register WindowClass");
      return 0;
   }


   RECT WindowRect = {0, 0, (long)SCREEN_WIDTH, (long)SCREEN_HEIGHT};
   AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);

   HWND WindowHandle = CreateWindowEx(0,
				      "Branch Debug Window Class",
				      "BRANCH DEBUG",
				      WS_OVERLAPPEDWINDOW,
				      CW_USEDEFAULT,
				      CW_USEDEFAULT,
				      WindowRect.right - WindowRect.left,
				      WindowRect.bottom - WindowRect.top,
				      0, 0, Instance, 0);

   if(!WindowHandle)
   {
      ErrorDialogue("Unable to Create Window");
      OnWindowsExit();
      return 0;
   }

   if(!OpenglCreate(WindowHandle))
   {
      ErrorDialogue("Unable to Create an Opengl context");
      OnWindowsExit();
      return 0;
   }

   ShowWindow(WindowHandle, CommandShow);
   b32 running = true;
   HDC hdc = GetDC(WindowHandle);

   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   
   GameState state;
   state.input = {};
   GameInit(state, 0, 0);
   
   MSG Message = {};

   LARGE_INTEGER Frequency;
   QueryPerformanceFrequency(&Frequency);
   LARGE_INTEGER Begin, Elapsed;

   delta = 1.0f;

   while(running)
   {
      QueryPerformanceCounter(&Begin);

      if(state.input.flags & Win32InputState::clickDown)
      {
	 state.input.flags &= ~Win32InputState::clickDown;
	 state.input.flags |= Win32InputState::clickHold;
      }

      if(state.input.flags & Win32InputState::escapeDown)
      {
	 state.input.flags &= ~Win32InputState::escapeDown;
	 state.input.flags |= Win32InputState::escapeHeld;
      }

      state.input.flags &= ~(Win32InputState::clickUp | Win32InputState::escapeUp);

      POINT p;			      
      GetCursorPos(&p);
      ScreenToClient(WindowHandle, &p);
      state.input.clickCoords.x = p.x;
      state.input.clickCoords.y = p.y;

      while(PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE))
      {
	 switch(Message.message)
	 {
	    case WM_CREATE:
	    {	       
	    }break;
	    case WM_QUIT:
	    {
	       running = false;
	    }break;
	    default:
	    {
	       DispatchMessage(&Message);
	    }break;

	    case VK_SPACE:
	    {
	       running = false;
	    }break;

	    case WM_KEYDOWN:
	    {
	       switch(Message.wParam)
	       {
		  case VK_SPACE:
		  {
		     if(!(state.input.flags & Win32InputState::clickHold))
		     {
			state.input.flags |= Win32InputState::clickDown;			
		     }
		  }break;

		  case VK_ESCAPE:
		  {
		     if(!(state.input.flags & Win32InputState::escapeHeld))
		     {
			state.input.flags |= Win32InputState::escapeHeld;
		     }
		  }break;

		  #ifdef DEBUG
		  case VK_UP:
		  case 0x57: // W
		  {
		     state.input.flags |= Win32InputState::clickw;
		  }break;

		  case VK_LEFT:
		  case 0x41: // A
		  {
		     state.input.flags |= Win32InputState::clicka;
		  }break;

		  case VK_DOWN:
		  case 0x53: // S
		  {
		     state.input.flags |= Win32InputState::clicks;
		  }break;

		  case VK_RIGHT:
		  case 0x44: // D
		  {
		     state.input.flags |= Win32InputState::clickd;
		  }break;
		  #endif
	       }
	    }break;

	    case WM_KEYUP:
	    {
	       switch(Message.wParam)
	       {
		  case VK_SPACE:
		  {		     
		     state.input.flags &= ~(Win32InputState::clickDown | Win32InputState::clickHold);
		     state.input.flags |= Win32InputState::clickUp;		     
		  }break;

		  case VK_ESCAPE:
		  {
		     state.input.flags &= ~(Win32InputState::escapeDown | Win32InputState::escapeHeld);
		     state.input.flags |= Win32InputState::escapeUp;
		  }break;

#ifdef DEBUG
		  case VK_UP:
		  case 0x57: // W
		  {
		     state.input.flags &= ~Win32InputState::clickw;
		  }break;

		  case VK_LEFT:
		  case 0x41: // A
		  {
		     state.input.flags &= ~Win32InputState::clicka;
		  }break;

		  case VK_DOWN:
		  case 0x53: // S
		  {
		     state.input.flags &= ~Win32InputState::clicks;
		  }break;

		  case VK_RIGHT:
		  case 0x44: // D
		  {
		     state.input.flags &= ~Win32InputState::clickd;
		  }break;
#endif
	       }
	    }break;

	    case WM_LBUTTONDOWN:
	    {
	       if(!(state.input.flags & Win32InputState::clickHold))
	       {
		  state.input.flags |= Win32InputState::clickDown;
	       }
	    }break;

	    case WM_LBUTTONUP:
	    {
	       assert(state.input.flags & (Win32InputState::clickDown | Win32InputState::clickHold));
	       state.input.flags &= ~(Win32InputState::clickDown | Win32InputState::clickHold);
	       state.input.flags |= Win32InputState::clickUp;
	    }break;
	 }
      }           

      GameLoop(state);
      SwapBuffers(hdc);      

      QueryPerformanceCounter(&Elapsed);
      Elapsed.QuadPart -= Begin.QuadPart;
      Elapsed.QuadPart *= 1000000;
      Elapsed.QuadPart /= Frequency.QuadPart;

      float time = (float)Elapsed.QuadPart;
      float target = 16666.7f;

      delta = time / target;
   }

   OnWindowsExit();
   GameEnd(state);
   return 0;
}
 
