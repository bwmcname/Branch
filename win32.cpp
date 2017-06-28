
#define ErrorDialogue(str) MessageBoxEx(0, (LPCTSTR)(str), 0, MB_ICONEXCLAMATION, 0)

size_t
WinFileSize(char *filename)
{
   HANDLE FileHandle = CreateFile(filename,
				  GENERIC_READ,
				  FILE_SHARE_READ,
				  0,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
				  0);

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

b32
WinReadFile(char *filename, u8 *buffer, size_t fileSize)
{
   HANDLE FileHandle = CreateFile(filename,
				  GENERIC_READ,
				  FILE_SHARE_READ,
				  0,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
				  0);


   if(FileHandle)
   {
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

   glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
   return true;
}

int CALLBACK WinMain(HINSTANCE Instance,
		     HINSTANCE Previous,
		     LPSTR CommandLine,
		     int CommandShow)
{
   GetGlExtensions(Instance);
   
   WNDCLASSEX WindowClass = {sizeof(WNDCLASSEX),
 			     CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
			     WindowProc,
			     0, 0,
			     Instance,
			     0, 0, 0, 0,
			     "Branch Debug Window Class",
			     0};

   if(!RegisterClassEx(&WindowClass))
   {
      ErrorDialogue("Unable to Register WindowClass");
      return 0;
   }


   RECT WindowRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
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
   GameInit(state);
   
   MSG Message = {};

   LARGE_INTEGER Frequency;
   QueryPerformanceFrequency(&Frequency);
   LARGE_INTEGER Begin, Elapsed;

   delta = 1.0f;

   while(running)
   {
      QueryPerformanceCounter(&Begin);
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
		     // OnKeyDown(state);
		  }break;
	       }
	    }break;

	    case WM_KEYUP:
	    {
	       switch(Message.wParam)
	       {
		  case VK_SPACE:
		  {
		     // OnKeyUp(state);
		  }break;
	       }
	    }break;
	 }
      }

      GameLoop(state);
      SwapBuffers(hdc);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      QueryPerformanceCounter(&Elapsed);
      Elapsed.QuadPart -= Begin.QuadPart;
      Elapsed.QuadPart *= 1000000;
      Elapsed.QuadPart /= Frequency.QuadPart;

      float time = (float)Elapsed.QuadPart;
      float target = 16666.7f;

      delta = time / target;
   }

   OnWindowsExit();
   return 0;
}
