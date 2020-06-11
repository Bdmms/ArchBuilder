#pragma once

#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)

class Accelerator
{
    char* __glutProgramName = NULL;
    int __glutArgc = 0;
    char** __glutArgv = NULL;
    char* __glutGeometry = NULL;
    Display* __glutDisplay = NULL;
    int __glutScreen;
    Window __glutRoot;
    int __glutScreenHeight;
    int __glutScreenWidth;
    GLboolean __glutIconic = GL_FALSE;
    GLboolean __glutDebug = GL_FALSE;
    unsigned int __glutDisplayMode = GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;
    char* __glutDisplayString = NULL;
    int __glutConnectionFD;
    XSizeHints __glutSizeHints = { 0 };
    int __glutInitWidth = 300, __glutInitHeight = 300;
    int __glutInitX = -1, __glutInitY = -1;
    GLboolean __glutForceDirect = GL_FALSE;
    GLboolean _glutTryDirect = GL_TRUE;
    Atom __glutWMDeleteWindow;

    static LRESULT CALLBACK  __glutFakeWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        HDC dc;
        HGLRC glrc;
        int pixelFormat;

        switch (msg) {
        case WM_CREATE:
            dc = GetDC(wnd);

            /* Pick the most basic pixel format descriptor we can. */
            pixelFormat = ChoosePixelFormat(dc, &pfd);
            SetPixelFormat(dc, pixelFormat, &pfd);

            /* Create an OpenGL rendering context and make it current. */
            glrc = wglCreateContext(dc);
            wglMakeCurrent(dc, glrc);

            /* Now we are allowed to use wglGetProcAddress. */

            GET_EXTENSION_PROC(wglGetExtensionsStringARB,
                PFNWGLGETEXTENSIONSSTRINGARBPROC);

            /* Make sure that the WGL_ARB_extensions_string extension is
               really advertised (it is somewhat backward that we have to
               wglGetProcAddress the routine that returns the WGL extension
               string before we even check for the extension name). */
            has_WGL_ARB_extensions_string =
                __glutIsSupportedByWGL("WGL_ARB_extensions_string");
            if (has_WGL_ARB_extensions_string) {

                /* Check for WGL_ARB_pixel_format support. */
                has_WGL_ARB_pixel_format =
                    __glutIsSupportedByWGL("WGL_ARB_pixel_format");
                if (has_WGL_ARB_pixel_format) {
                    GET_EXTENSION_PROC(wglGetPixelFormatAttribivARB,
                        PFNWGLGETPIXELFORMATATTRIBIVARBPROC);
                    GET_EXTENSION_PROC(wglGetPixelFormatAttribfvARB,
                        PFNWGLGETPIXELFORMATATTRIBFVARBPROC);
                    GET_EXTENSION_PROC(wglChoosePixelFormatARB,
                        PFNWGLCHOOSEPIXELFORMATARBPROC);
                }

                /* Check for WGL_ARB_multisample support. */
                has_WGL_ARB_multisample =
                    __glutIsSupportedByWGL("WGL_ARB_multisample");
                has_GL_ARB_multisample =
                    glutExtensionSupported("GL_ARB_multisample");
                if (has_WGL_ARB_multisample && has_GL_ARB_multisample) {
                    GET_EXTENSION_PROC(glSampleCoverageARB,
                        PFNGLSAMPLECOVERAGEARBPROC);
                }
                /* We called glutExtensionSupported, but now we are going to
                   destroy the context so we need to invalidate our pointer
                   to the extension string returned by glGetString. */
                __glutInvalidateExtensionStringCacheIfNeeded(glrc);
            }

            /* Unbind from our context and delete the context once we have
               gotten the WGL information we need. */
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(glrc);
            ReleaseDC(wnd, dc);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }

        return DefWindowProc(wnd, msg, wParam, lParam);
    }

    void __glutOpenWin32Connection(char* display)
    {
        const static char* classname;
        WNDCLASS  wc;
        HINSTANCE hInstance = GetModuleHandle(NULL);
        HWND wnd;
        MSG msg;

        /* Make sure we register the window only once. */
        if (classname) 
            return;

        classname = "GLUT";

        /* Clear and then fill in the window class structure. */
        memset(&wc, 0, sizeof(WNDCLASS));
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = (WNDPROC)__glutFakeWindowProc;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(hInstance, L"GLUT_ICON");
        wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "FAKE_GLUT";  /* Poor name choice. */

        /* Fill in a default icon if one isn't specified as a resource. */
        if (!wc.hIcon) {
            wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        }

        /* Register fake window class. */
        if (RegisterClass(&wc) == 0) {
            __glutFatalError("RegisterClass1 failed: "
                "Cannot register fake GLUT window class.");
        }

        /* Register real window class. */
        wc.lpfnWndProc = (WNDPROC)__glutWindowProc;
        wc.lpszClassName = classname;
        if (RegisterClass(&wc) == 0) {
            __glutFatalError("RegisterClass2 failed: "
                "Cannot register GLUT window class.");
        }

        __glutScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        __glutScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        /* Set the root window to NULL because windows creates a top-level
           window when the parent is NULL.  X creates a top-level window
           when the parent is the root window. */
        __glutRoot = NULL;

        /* Set the display to 1 -- we shouldn't be using this anywhere
           (except as an argument to X calls). */
        __glutDisplay = (Display*)1;

        /* There isn't any concept of multiple screens in Win32, therefore,
           we don't need to keep track of the screen we're on... it's always
           the same one. */
        __glutScreen = 0;

        /* Create fake window */
        wnd = CreateWindow(L"FAKE_GLUT", L"GLUT",
            WS_OVERLAPPEDWINDOW,
            40, 40,
            40, 40,
            NULL, NULL, hInstance, NULL);
        if (!wnd) {
            __glutFatalError("CreateWindow failed: "
                "Cannot create fake GLUT window.");
        }

        /* Destroy fake window */
        DestroyWindow(wnd);
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DWORD __glutInitTime(void)
    {
        static int beenhere = 0;
        static DWORD genesis;

        if (!beenhere) {
            /* GetTickCount has 5 millisecond accuracy on Windows 98
               and 10 millisecond accuracy on Windows NT 4.0. */
            genesis = GetTickCount();
            beenhere = 1;
        }
        return genesis;
    }

    //void GLUTAPIENTRY
    void glutInit(int* argcp, char** argv)
    {
        char* display = NULL;
        char* str, * geometry = NULL;
        int i;

        if (__glutDisplay) 
        {
            __glutWarning("glutInit being called a second time.");
            return;
        }

        /* Determine temporary program name. */
        str = strrchr(argv[0], '/');
        if (str == NULL) {
            __glutProgramName = argv[0];
        }
        else {
            __glutProgramName = str + 1;
        }

        /* Make private copy of command line arguments. */
        __glutArgc = *argcp;
        __glutArgv = (char**)malloc(__glutArgc * sizeof(char*));
        if (!__glutArgv) {
            __glutFatalError("out of memory.");
        }
        for (i = 0; i < __glutArgc; i++) {
            __glutArgv[i] = __glutStrdup(argv[i]);
            if (!__glutArgv[i]) {
                __glutFatalError("out of memory.");
            }
        }

        /* determine permanent program name */
        str = strrchr(__glutArgv[0], '/');
        if (str == NULL) {
            __glutProgramName = __glutArgv[0];
        }
        else {
            __glutProgramName = str + 1;
        }

        /* parse arguments for standard options */
        for (i = 1; i < __glutArgc; i++) 
        {
            if (!strcmp(__glutArgv[i], "-display")) 
            {
                __glutWarning("-display option not supported by Win32 GLUT.");

                if (++i >= __glutArgc)
                    __glutFatalError("follow -display option with X display name.");

                display = __glutArgv[i];
                removeArgs(argcp, &argv[1], 2);
            }
            else if (!strcmp(__glutArgv[i], "-geometry")) 
            {
                if (++i >= __glutArgc) 
                    __glutFatalError("follow -geometry option with geometry parameter.");
                
                geometry = __glutArgv[i];
                removeArgs(argcp, &argv[1], 2);
            }
            else if (!strcmp(__glutArgv[i], "-direct"))
            {
                __glutWarning("-direct option not supported by Win32 GLUT.");

                if (!__glutTryDirect) 
                    __glutFatalError( "cannot force both direct and indirect rendering.");

                //__glutForceDirect = GL_TRUE;
                //removeArgs(argcp, &argv[1], 1);
            }
            else if (!strcmp(__glutArgv[i], "-indirect")) 
            {
                __glutWarning("-indirect option not supported by Win32 GLUT.");

                if (__glutForceDirect) {
                    __glutFatalError(
                        "cannot force both direct and indirect rendering.");
                }
                __glutTryDirect = GL_FALSE;
                removeArgs(argcp, &argv[1], 1);
            }
            else if (!strcmp(__glutArgv[i], "-iconic")) 
            {
                __glutIconic = GL_TRUE;
                removeArgs(argcp, &argv[1], 1);
            }
            else if (!strcmp(__glutArgv[i], "-gldebug")) 
            {
                __glutDebug = GL_TRUE;
                removeArgs(argcp, &argv[1], 1);
            }
            else if (!strcmp(__glutArgv[i], "-sync")) 
            {
                #if defined(_WIN32)
                __glutWarning("-sync option not supported by Win32 GLUT.");
                #endif
                synchronize = GL_TRUE;
                removeArgs(argcp, &argv[1], 1);
            }
            else {
                /* Once unknown option encountered, stop command line
                   processing. */
                break;
            }
        }

        __glutOpenWin32Connection(display);
        
        if (geometry) {
            int flags, x, y, width, height;

            /* Fix bogus "{width|height} may be used before set"
               warning */
            width = 0;
            height = 0;

            flags = XParseGeometry(geometry, &x, &y,
                (unsigned int*)&width, (unsigned int*)&height);
            if (WidthValue & flags) {
                /* Careful because X does not allow zero or negative
                   width windows */
                if (width > 0) {
                    __glutInitWidth = width;
                }
            }
            if (HeightValue & flags) {
                /* Careful because X does not allow zero or negative
                   height windows */
                if (height > 0) {
                    __glutInitHeight = height;
                }
            }
            glutInitWindowSize(__glutInitWidth, __glutInitHeight);
            if (XValue & flags) {
                if (XNegative & flags) {
                    x = DisplayWidth(__glutDisplay, __glutScreen) +
                        x - __glutSizeHints.width;
                }
                /* Play safe: reject negative X locations */
                if (x >= 0) {
                    __glutInitX = x;
                }
            }
            if (YValue & flags) {
                if (YNegative & flags) {
                    y = DisplayHeight(__glutDisplay, __glutScreen) +
                        y - __glutSizeHints.height;
                }
                /* Play safe: reject negative Y locations */
                if (y >= 0) {
                    __glutInitY = y;
                }
            }
            glutInitWindowPosition(__glutInitX, __glutInitY);
        }

        (void) __glutInitTime();
    }

    void glutInitWindowPosition(int x, int y)
    {
        __glutInitX = x;
        __glutInitY = y;
        if (x >= 0 && y >= 0) {
            __glutSizeHints.x = x;
            __glutSizeHints.y = y;
            __glutSizeHints.flags |= USPosition;
        }
        else {
            __glutSizeHints.flags &= ~USPosition;
        }
    }

    void glutInitWindowSize(int width, int height)
    {
        __glutInitWidth = width;
        __glutInitHeight = height;
        if (width > 0 && height > 0) {
            __glutSizeHints.width = width;
            __glutSizeHints.height = height;
            __glutSizeHints.flags |= USSize;
        }
        else {
            __glutSizeHints.flags &= ~USSize;
        }
    }
};
