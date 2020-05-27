#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/timer.hpp>

#ifdef NANA_WINDOWS

#   include <windows.h>
#   include <GL/gl.h>
#pragma comment(lib, "opengl32.lib")
HGLRC EnableOpenGL(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd;

    memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    auto iFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, iFormat, &pfd);

    HGLRC hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
    return hRC;
}
#elif defined(NANA_X11)
#   include <GL/glx.h>
#   include <X11/Xlib.h>

GLXContext EnableOpenGL(nana::native_window_type wd)
{
    GLint attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    auto disp = const_cast<Display*>(reinterpret_cast<const Display*>(nana::API::x11::get_display()));
    auto vi = glXChooseVisual(disp, 0, attr);

    auto glctx = glXCreateContext(disp, vi, nullptr, GL_TRUE);
    glXMakeCurrent(disp, reinterpret_cast<Window>(wd), glctx);
    return glctx;
}
#endif

int main()
{
    using namespace nana;
    form fm;

    nested_form nfm(fm, rectangle{ 10, 10, 100, 20 }, form::appear::bald<>());
    nfm.show();

    button btn(nfm, rectangle{ 0, 0, 100, 20 });
    btn.caption(L"Exit");
    btn.events().click([&fm]{
        fm.close();
    });

#ifdef NANA_WINDOWS
    auto hwnd = reinterpret_cast<HWND>(fm.native_handle());
    HDC hDC = ::GetDC(hwnd);

    auto glctx = EnableOpenGL(hDC);
#elif defined(NANA_X11)

    auto glctx = EnableOpenGL(fm.native_handle());

#endif

    float theta = 0.0f;

#ifdef NANA_WINDOWS
    auto gl_swap_buf = [hDC]{
        SwapBuffers(hDC);
    };
#elif defined(NANA_X11)
    auto gl_swap_buf = [&fm]{
        auto disp = const_cast<Display*>(reinterpret_cast<const Display*>(API::x11::get_display()));

        glXSwapBuffers(disp, reinterpret_cast<Window>(fm.native_handle()));
    };
#endif

    //Define a opengl rendering function
    fm.draw_through([gl_swap_buf, theta]() mutable
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glRotatef(theta, 0.0f, 0.0f, 1.0f);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);   glVertex2f(0.0f, 1.0f);
        glColor3f(0.0f, 1.0f, 0.0f);   glVertex2f(0.87f, -0.5f);
        glColor3f(0.0f, 0.0f, 1.0f);   glVertex2f(-0.87f, -0.5f);
        glEnd();
        glPopMatrix();
        gl_swap_buf();

        glFinish();

        theta += 1.0f;
    });

#ifdef NANA_WINDOWS
    fm.events().destroy([hwnd, hDC, glctx]
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glctx);
        ReleaseDC(hwnd, hDC);
    });
#elif defined(NANA_X11)
    fm.events().destroy([glctx]
    {
        auto disp = const_cast<Display*>(reinterpret_cast<const Display*>(API::x11::get_display()));

        glXMakeCurrent(disp, None, nullptr);
        glXDestroyContext(disp, glctx);
    });
#endif

    timer tmr;

#ifdef NANA_WINDOWS
    tmr.elapse([hwnd]{
        RECT r;
        ::GetClientRect(hwnd, &r);
        ::InvalidateRect(hwnd, &r, FALSE);
    });
#elif defined(NANA_X11)
    tmr.elapse([&fm]{
        auto wd = reinterpret_cast<Window>(fm.native_handle());
        auto disp = const_cast<Display*>(reinterpret_cast<const Display*>(API::x11::get_display()));

        XClearArea(disp, wd, 0, 0, 1, 1, true);
        XFlush(disp);

    });
#endif

    tmr.interval(std::chrono::milliseconds(20));
    tmr.start();

    fm.show();

    exec();
    return 0;
}
