/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion.  All rights reserved.
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.  See the
 *  file LICENSE.GPL at the root directory of this source distribution
 *  for more details.
 *
 *  If you desire to use Coin with software that is incompatible
 *  licensewise with the GPL, and / or you would like to take
 *  advantage of the additional benefits with regard to our support
 *  services, please contact Systems in Motion about acquiring a Coin
 *  Professional Edition License.  See <URL:http://www.coin3d.org> for
 *  more information.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  <URL:http://www.sim.no>, <mailto:support@sim.no>
 *
\**************************************************************************/

/*!
  \class SoOffscreenRenderer SoOffscreenRenderer.h Inventor/SoOffscreenRenderer.h
  \brief The SoOffscreenRenderer class is used for rendering scenes in offscreen buffers.
  \ingroup general

  If you want to render to a memory buffer instead of an on-screen
  OpenGL context, use this class. Rendering to a memory buffer can be
  used to generate texture maps on-the-fly, or for saving snapshots of
  the scene to disk files (as pixel bitmaps or as Postscript files for
  sending to a Postscript-capable printer).

  Currently offscreen rendering can be done with GLX (i.e. OpenGL on
  X11), WGL (i.e. OpenGL on Win32) and AGL (i.e. OpenGL on Mac OS X).

  Here's a dead simple usage example:

  \code
  SoOffscreenRenderer * myRenderer = new SoOffscreenRenderer(vpregion);
  SoNode * root = myViewer->getSceneManager()->getSceneGraph();
  SbBool ok = myRenderer->render(root);
  // [then use image buffer in a texture, or write it to file, or whatever]
  \endcode
*/
// FIXME: include the following in documentation about how to use the
// SoOffscreenRenderer to write movies:
// 
//    You should write your application to control the global
//    "realTime" field, then you will be able to "step" with
//    appropriate time units for each render operation (eg first
//    render with realTime=0.0, then add 1/60s to the realTime field,
//    render again to a new frame, add another 1/60s to the realTime
//    field, render, and so on).
//
// 20020110 mortene.

#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/SoPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/SbVec2f.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <Inventor/system/gl.h>
#include <assert.h>
#include <string.h> // memset(), memcpy()
#include <coindefs.h> // COIN_STUB()
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <simage_wrapper.h>
#include "../misc/GLWrapper.h"
#include <math.h> // for ceil()

/*!
  \enum SoOffscreenRenderer::Components

  Enumerated values for the available image formats.

  \sa setComponents()
*/

#ifndef DOXYGEN_SKIP_THIS // Don't document internal classes.

class SoOffscreenInternalData {
public:
  SoOffscreenInternalData(void) {
    this->buffersize = SbVec2s(0, 0);
  }
  virtual ~SoOffscreenInternalData() {
  }

  // Return FALSE if the necessary resource for rendering are not
  // available.
  virtual SbBool makeContextCurrent(uint32_t contextid) = 0;

  virtual unsigned char * getBuffer(void) = 0;

  virtual void setBufferSize(const SbVec2s & size) {
    SbVec2s maxsize = this->getMax();

#if COIN_DEBUG
    if (size[0] > maxsize[0]) {
      SoDebugError::postWarning("SoOffscreenInternalData::setBufferSize",
                                "can't set width to %d, maximum is %d",
                                size[0], maxsize[0]);
    }
    else if (size[0] <= 0) {
      SoDebugError::postWarning("SoOffscreenInternalData::setBufferSize",
                                "can't set negative width %d", size[0]);
    }
    if (size[1] > maxsize[1]) {
      SoDebugError::postWarning("SoOffscreenInternalData::setBufferSize",
                                "can't set height to %d, maximum is %d",
                                size[1], maxsize[1]);
    }
    else if (size[1] <= 0) {
      SoDebugError::postWarning("SoOffscreenInternalData::setBufferSize",
                                "can't set negative height %d", size[1]);
    }
#endif // COIN_DEBUG

    // The explicit casts are done to humour the HPUX aCC compiler,
    // which will otherwise say ``Template deduction failed to find a
    // match for the call to 'SbMin'''. mortene.
    this->buffersize[0] = SbMax((short)1,
                                SbMin((short)size[0], (short)maxsize[0]));
    this->buffersize[1] = SbMax((short)1,
                                SbMin((short)size[1], (short)maxsize[1]));
  }

  SbVec2s getSize(void) const {
    return this->buffersize;
  }

  // Will be called right after a render operation has taken
  // place. Default method does nothing.
  virtual void postRender(void) { }

protected:
  SbVec2s buffersize;

  // Note: should _really_ be overloaded by subclasses.
  virtual SbVec2s getMax(void) { return SbVec2s(32767, 32767); }
};


#ifdef HAVE_AGL
#include <AGL/agl.h>  
#include <Carbon/Carbon.h>  // for quickdraw functions

class SoOffscreenAGLData : public SoOffscreenInternalData {
public:
  SoOffscreenAGLData(void)
    : drawable(NULL), context(NULL), pixformat(NULL)
  {
    this->buffer = NULL;

    // FIXME: not 100% sure which attributes are best, got to check
    // kyrah 20020223

    GLint attrib[] = {
      AGL_OFFSCREEN,
      AGL_RGBA,
      AGL_NO_RECOVERY,
      AGL_RED_SIZE, 8,
      AGL_GREEN_SIZE, 8,
      AGL_BLUE_SIZE, 8,
      AGL_DEPTH_SIZE, 32,
      AGL_STENCIL_SIZE, 1,
      AGL_NONE
    };

    this->pixformat = aglChoosePixelFormat( NULL, 0, attrib );
    if (!this->pixformat) {
      SoDebugError::postWarning("SoOffscreenAGLData::SoOffscreenAGLData",
                                "Couldn't get RGBA AGL pixelformat.");
      return;
    }

    this->context = aglCreateContext( pixformat, NULL );
    if (!this->context) {
      SoDebugError::postWarning("SoOffscreenAGLData::SoOffscreenAGLData",
                                "Couldn't create AGL context.");
    }
  }

  virtual ~SoOffscreenAGLData() {
    if (this->drawable) DisposeGWorld((GWorldPtr) drawable);
    if (this->context) aglDestroyContext(context);
    if (this->pixformat) aglDestroyPixelFormat(pixformat);
    delete[] this->buffer;
  }

  virtual void setBufferSize(const SbVec2s & size) {
    CGrafPtr savedport;
    GDHandle savedgdh;

    SoOffscreenInternalData::setBufferSize(size);

    delete[] this->buffer;
    this->buffer =
      new unsigned char[this->buffersize[0] * this->buffersize[1] * 4];

    SetRect(&bounds, 0, 0, size[0], size[1]);

    // We have to save (and later restore) the old graphics port and 
    // GHandle, since this function will probably called before the
    // Mac OS X viewer is fully set up.
    GetGWorld(&savedport, &savedgdh); 

    QDErr e = NewGWorld(&drawable, 32, &bounds, NULL /* cTable */, 
              NULL /*aGDevice */, 0);
    if(e != noErr) {
      SoDebugError::postWarning("SoOffscreenAGLData::SoOffscreenAGLData",
                                "Error creating GWorld: %d", e);
    }
    if (!this->drawable) {
      SoDebugError::postWarning("SoOffscreenAGLData::SoOffscreenAGLData",
                                "Couldn't create AGL drawable.");
    }
    SetGWorld(savedport, savedgdh);
  }

  virtual SbBool makeContextCurrent(void) {
    assert(this->buffer);
    if (this->context && this->drawable) {
      PixMapHandle pixmap = GetGWorldPixMap((GWorldPtr)drawable);
      aglSetOffScreen(context, bounds.right-bounds.left, 
                      bounds.bottom-bounds.top,
                      GetPixRowBytes(pixmap), GetPixBaseAddr(pixmap));     
      aglSetCurrentContext(context);
      return TRUE;
    }
    return FALSE;
  }

  virtual unsigned char * getBuffer(void) {
    return this->buffer;
  }

  virtual void postRender(void) {
    SbVec2s size = this->getSize();

    if (this->context && this->buffer) {
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, size[0], size[1], GL_RGBA, GL_UNSIGNED_BYTE,
                   this->buffer);
      glPixelStorei(GL_PACK_ALIGNMENT, 4);

      // FIXME: GLX version saves old context and drawable, and restores 
      // it after use, should i do that as well? kyrah 20020223
    }
  }

  static SbVec2s getMaxDimensions(void) {
    // FIXME: make sure we return the real limitations. 20020225 mortene.
    return SbVec2s(32767, 32767);
  }


private:
  virtual SbVec2s getMax(void) {
    return SoOffscreenAGLData::getMaxDimensions();
  }

  unsigned char *buffer;
  AGLDrawable drawable;
  AGLContext context;
  AGLPixelFormat pixformat;
  Rect bounds;
};

#endif // HAVE_AGL


#ifdef HAVE_GLX
#include <GL/glx.h>

class SoOffscreenGLXData : public SoOffscreenInternalData {
public:
  SoOffscreenGLXData(void)
    // set everything to NULL first to gracefully handle error conditions
    : display(NULL), visinfo(NULL), context(NULL), pixmap(0), glxpixmap(0),
      storedcontext(NULL)
  {
    this->buffer = NULL;

    this->display = XOpenDisplay(NULL);
    if (!this->display) {
      SoDebugError::postWarning("SoOffscreenGLXData::SoOffscreenGLXData",
                                "Couldn't connect to X11 DISPLAY.");
      return;
    }

    static int single_attribs[] = {
      GLX_RGBA,
      // Need this to make sure we get a visual with a depth buffer.
      GLX_DEPTH_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      None
    };

    static int double_attribs[] = {
      GLX_RGBA,
      // Need this to make sure we get a visual with a depth buffer.
      GLX_DEPTH_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      // Yes, I've actually seen displays which lack singlebuffer
      // visuals.
      GLX_DOUBLEBUFFER,
      None
    };

    static int * attribsettings[] = { single_attribs, double_attribs, NULL };

    // FIXME: using glXChooseVisual() like this picks a 8-bit
    // PseudoColor visual on SGI IRIX X11 servers with default
    // configuration settings. Should instead use some logic based on
    // XMatchVisualInfo() to find the best (i.e. deepest)
    // visual. This is Bugzilla #129. 20000622 mortene.

    int attribidx = 0;
    do {
      this->visinfo = glXChooseVisual(this->display,
                                      DefaultScreen(this->display),
                                      attribsettings[attribidx++]);
    } while (this->visinfo == NULL && attribsettings[attribidx] != NULL);

    // FIXME: should fallback to even less restrictive attribs
    // settings if all glXChooseVisual() attempts fails.
    // 20001003 mortene.

    if (!this->visinfo) {
      SoDebugError::postWarning("SoOffscreenGLXData::SoOffscreenGLXData",
                                "Couldn't get RGBA X11 visual.");
      return;
    }

    this->context = glXCreateContext(this->display, this->visinfo,
                                     0, GL_FALSE);
    if (!this->context) {
      SoDebugError::postWarning("SoOffscreenGLXData::SoOffscreenGLXData",
                                "Couldn't create GLX context.");
    }
  }

  virtual ~SoOffscreenGLXData() {
    if (this->context) glXDestroyContext(this->display, this->context);
    if (this->glxpixmap) glXDestroyGLXPixmap(this->display, this->glxpixmap);
    if (this->pixmap) XFreePixmap(this->display, this->pixmap);
    if (this->visinfo) XFree(this->visinfo);
    // The following XCloseDisplay() call has been known to crash the
    // application when running remotely from some old Mesa version on
    // Red Hat Linux 6.2 onto an IRIX6.5 display. It seems likely that
    // this was caused by a bug in that particular old Mesa
    // version. Since the leak caused by _not_ freeing up the Display
    // resource is a showstopper for applications doing lots of
    // offscreen rendering, we leave this in.
    //
    // If you see crashes, feel free to report the particulars about
    // your system to us (OpenGL implementation & version, X11 server
    // and client implementations and versions, etc).
    //
    // mortene@sim.no
    if (this->display) XCloseDisplay(this->display);

    delete[] this->buffer;
  }

  virtual void setBufferSize(const SbVec2s & size) {
    SoOffscreenInternalData::setBufferSize(size);

    delete[] this->buffer;
    this->buffer =
      new unsigned char[this->buffersize[0] * this->buffersize[1] * 4];

    if (this->pixmap) XFreePixmap(this->display, this->pixmap);
    if (this->glxpixmap) glXDestroyGLXPixmap(this->display, this->glxpixmap);

    if (this->context) {
      this->pixmap = XCreatePixmap(this->display,
                                   DefaultRootWindow(this->display),
                                   size[0], size[1],
                                   this->visinfo->depth);
      if (!this->pixmap) {
        SoDebugError::postWarning("SoOffscreenGLXData::SoOffscreenGLXData",
                                  "Couldn't create %dx%dx%d Pixmap.");
      }
      else {
        this->glxpixmap = glXCreateGLXPixmap(this->display, this->visinfo,
                                             this->pixmap);
        if (!this->glxpixmap) {
          SoDebugError::postWarning("SoOffscreenGLXData::SoOffscreenGLXData",
                                    "Couldn't create GLX Pixmap.");
        }
      }

    }
  }

  virtual SbBool makeContextCurrent(uint32_t contextid) {
    assert(this->buffer);
    if (this->context && this->glxpixmap) {
      const GLWrapper_t * glw = GLWrapper(contextid);
      if (glw->glXGetCurrentDisplay) {
        this->storedcontext = glXGetCurrentContext();
        if (this->storedcontext) {
          this->storeddrawable = glXGetCurrentDrawable();
          this->storeddisplay = glw->glXGetCurrentDisplay();
        }
      }

      glXMakeCurrent(this->display, this->glxpixmap, this->context);
      return TRUE;
    }
    return FALSE;
  }

  virtual unsigned char * getBuffer(void) {
    return this->buffer;
  }

  virtual void postRender(void) {
    SbVec2s size = this->getSize();

    if (this->context && this->buffer) {
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, size[0], size[1], GL_RGBA, GL_UNSIGNED_BYTE,
                   this->buffer);
      glPixelStorei(GL_PACK_ALIGNMENT, 4);
      (void) glXMakeCurrent(this->display, 0, 0);

      if (this->storedcontext && this->storeddrawable && this->storeddisplay) {
        (void) glXMakeCurrent(this->storeddisplay, this->storeddrawable,
                              this->storedcontext);
      }
    }
  }

  static SbVec2s getMaxDimensions(void) {
    // FIXME: where can we get hold of the _real_ max values for
    // Pixmap and/or GLXPixmap? 20000417 mortene.
    return SbVec2s(32767, 32767);
  }


private:
  virtual SbVec2s getMax(void) {
    return SoOffscreenGLXData::getMaxDimensions();
  }

  unsigned char * buffer;
  Display * display;
  XVisualInfo * visinfo;
  GLXContext context;
  Pixmap pixmap;
  GLXPixmap glxpixmap;

  Display * storeddisplay;
  GLXDrawable storeddrawable;
  GLXContext storedcontext;
};

#endif // HAVE_GLX

#ifdef HAVE_WGL

class SoOffscreenWGLData : public SoOffscreenInternalData {
public:
  SoOffscreenWGLData(void)
  {
    this->buffer = NULL;
    this->context = NULL;
    this->devicecontext = NULL;
    this->bitmap = NULL;
    this->oldbitmap = NULL;
    this->storedcontext = NULL;
  }

  virtual ~SoOffscreenWGLData() {
    this->cleanupWGL();
    delete[] this->buffer;
  }

  virtual void setBufferSize(const SbVec2s & size) {
    SoOffscreenInternalData::setBufferSize(size);

    delete[] this->buffer;
    this->buffer =
      new unsigned char[this->buffersize[0] * this->buffersize[1] * 4];

    this->initWGL();
  }

  virtual SbBool makeContextCurrent(uint32_t contextid) {
    assert(this->buffer);
    if (this->context && this->bitmap) {
      this->storedcontext = wglGetCurrentContext();
      if (this->storedcontext) {
        this->storeddc = wglGetCurrentDC();
      }
      wglMakeCurrent(this->devicecontext, this->context);
      return TRUE;
    }
    return FALSE;
  }

  virtual unsigned char * getBuffer(void) {
    return this->buffer;
  }

  virtual void postRender(void) {
    SbVec2s size = this->getSize();

    if (this->context && this->buffer) {
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, size[0], size[1], GL_RGBA, GL_UNSIGNED_BYTE,
                   this->buffer);
      glPixelStorei(GL_PACK_ALIGNMENT, 4);

      if (this->storedcontext && this->storeddc) {
        wglMakeCurrent(this->storeddc, this->storedcontext);
        this->storedcontext = NULL;
      }
      else {
        wglMakeCurrent(NULL, NULL);
      }
    }
  }

  static SbVec2s getMaxDimensions(void) {
    // FIXME: where can we get hold of the _real_ max values for
    // Pixmap and/or GLXPixmap? 20000417 mortene.
    return SbVec2s(32767, 32767);
  }

  void cleanupWGL() {
    // cleanup member variabler
    if(this->devicecontext != NULL) {
      if(this->oldbitmap != NULL) {
        SelectObject(this->devicecontext, this->oldbitmap);
        this->oldbitmap = NULL;
      }
      if(this->bitmap != NULL) {
        DeleteObject(this->bitmap);
        this->bitmap = NULL;
      }
      DeleteDC(this->devicecontext);
      this->devicecontext = NULL;
    }
  }

  void initWGL() {
    // initialization of member variables

    this->cleanupWGL();

    this->devicecontext = CreateCompatibleDC(NULL);
    if (this->devicecontext == NULL) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "Couldn't create DC. CreateCompatibleDC() failed with error code %d.", dwError);
      return;
    }

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = this->buffersize[0];
    bmi.bmiHeader.biHeight = this->buffersize[1];
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed  = 0;
    bmi.bmiHeader.biClrImportant = 0;
    bmi.bmiColors[0].rgbBlue = 0;
    bmi.bmiColors[0].rgbGreen;
    bmi.bmiColors[0].rgbRed;
    bmi.bmiColors[0].rgbReserved;

    // make a bitmap to draw to
    void *pvBits;
    this->bitmap = CreateDIBSection(this->devicecontext, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (this->bitmap == NULL) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "Couldn't create DIB. CreateDIBSection() failed with error code %d.", dwError);
      return;
    }

    this->oldbitmap = (HBITMAP) SelectObject(this->devicecontext, this->bitmap);
    if (this->oldbitmap == NULL) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "Couldn't select bitmap into device context. SelectObject() failed with error code %d.", dwError);
    }

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd
        1,                     // version number
        PFD_DRAW_TO_BITMAP |   // support bitmap
        PFD_SUPPORT_OPENGL,    // support OpenGL
        PFD_TYPE_RGBA,         // RGBA type
        24,                    // 24-bit color depth
        0, 0, 0, 0, 0, 0,      // color bits ignored
        0,                     // no alpha buffer
        0,                     // shift bit ignored
        0,                     // no accumulation buffer
        0, 0, 0, 0,            // accum bits ignored
        32,                    // 32-bit z-buffer
        0,                     // no stencil buffer
        0,                     // no auxiliary buffer
        PFD_MAIN_PLANE,        // main layer
        0,                     // reserved
        0, 0, 0                // layer masks ignored
    };

    int iPixelFormat;

    // get the best available match of pixel format for the device context
    iPixelFormat = ChoosePixelFormat(this->devicecontext, &pfd);
    if (iPixelFormat == 0) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "ChoosePixelFormat failed with error code %d.", dwError);
    }

    // make that the pixel format of the device context
    bool ret = SetPixelFormat(this->devicecontext, iPixelFormat, &pfd);
    if (!ret)
    {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "SetPixelFormat failed with error code %d.", dwError);
    }

    // verify that the pixel format of the device context supports OpenGL
    iPixelFormat = GetPixelFormat(this->devicecontext);
    if (iPixelFormat == 0) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "GetPixelFormat failed with error code %d.", dwError);
    }

    iPixelFormat = DescribePixelFormat(this->devicecontext, iPixelFormat,  sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    if (iPixelFormat == 0) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "DescribePixelFormat failed with error code %d.", dwError);
    }

    if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "Device Context doesn't support OpenGL, according to DescribePixelFormat.");
    }

    // Create WGL context
    this->context = wglCreateContext(this->devicecontext);
    if (this->context == NULL) {
      DWORD dwError = GetLastError();
      SoDebugError::postWarning("SoOffscreenWGLData::initWGL",
                                "wglCreateContext failed with error code %d.", dwError);
      return;
    }
  }

private:
  virtual SbVec2s getMax(void) {
    return SoOffscreenWGLData::getMaxDimensions();
  }

  unsigned char * buffer;
  HGLRC context;
  HDC devicecontext;
  HBITMAP bitmap;
  HBITMAP oldbitmap;

  HGLRC storedcontext;
  HDC storeddc;
};

#endif // HAVE_WGL

#endif // DOXYGEN_SKIP_THIS



/*!
  Constructor. Argument is the \a viewportregion we should use when
  rendering. An internal SoGLRenderAction will be constructed.
*/
SoOffscreenRenderer::SoOffscreenRenderer(const SbViewportRegion & viewportregion)
  : backgroundcolor(0.0f, 0.0f, 0.0f),
    components(RGB),
    renderaction(new SoGLRenderAction(viewportregion)),
    didallocaction(TRUE),
    internaldata(NULL),
    buffer(NULL)
{
#ifdef HAVE_GLX
  this->internaldata = new SoOffscreenGLXData();
#elif defined(HAVE_WGL)
  this->internaldata = new SoOffscreenWGLData();
#elif defined(HAVE_AGL)
  this->internaldata = new SoOffscreenAGLData();
#endif // HAVE_AGL

  this->renderaction->setCacheContext(SoGLCacheContextElement::getUniqueCacheContext());
  this->setViewportRegion(viewportregion);
}

/*!
  Constructor. Argument is the \a action we should apply to the
  scene graph when rendering the scene. Information about the
  viewport is extracted from the \a action.
*/
SoOffscreenRenderer::SoOffscreenRenderer(SoGLRenderAction * action)
  : viewport(256, 256),
    backgroundcolor(0.0f, 0.0f, 0.0f),
    components(RGB),
    renderaction(action),
    didallocaction(FALSE),
    internaldata(NULL),
    buffer(NULL)
{
#ifdef HAVE_GLX
  this->internaldata = new SoOffscreenGLXData();
#elif defined(HAVE_WGL)
  this->internaldata = new SoOffscreenWGLData();
#elif defined(HAVE_AGL)
  this->internaldata = new SoOffscreenAGLData();
#endif // HAVE_AGL
  assert(action);
  this->setViewportRegion(action->getViewportRegion());
}

/*!
  Destructor.
*/
SoOffscreenRenderer::~SoOffscreenRenderer()
{
  delete[] this->buffer;
  delete this->internaldata;
  if (this->didallocaction) delete this->renderaction;
}

/*!
  Returns the screen pixels per inch resolution of your monitor.
*/
float
SoOffscreenRenderer::getScreenPixelsPerInch(void)
{
  COIN_STUB();
  return 72.0f; // default value
}

/*!
  Get maximum dimensions (width, height) of the offscreen buffer.
*/
SbVec2s
SoOffscreenRenderer::getMaximumResolution(void)
{
#ifdef HAVE_GLX
  SoOffscreenGLXData::getMaxDimensions();
#elif defined(HAVE_WGL)
  SoOffscreenWGLData::getMaxDimensions();
#elif defined(HAVE_AGL)
  SoOffscreenAGLData::getMaxDimensions();
#endif // HAVE_AGL
  return SbVec2s(0, 0);
}

/*!
  Sets the component format of the offscreen buffer.

  If set to \c LUMINANCE, a grayscale image is rendered, \c
  LUMINANCE_TRANSPARENCY gives us a grayscale image with transparency,
  \c RGB will give us a 24-bit image with 8 bits each for the red,
  green and blue component, and \c RGB_TRANSPARENCY yields a 32-bit
  image (\c RGB plus transparency).

  The default format to render to is \c RGB.
*/
void
SoOffscreenRenderer::setComponents(const Components components)
{
  this->components = components;

  SbVec2s dims = this->getViewportRegion().getViewportSizePixels();

  delete[] this->buffer;
  this->buffer = new unsigned char[dims[0] * dims[1] * this->components];
}

/*!
  Returns the component format of the offscreen buffer.

  \sa setComponents()
 */
SoOffscreenRenderer::Components
SoOffscreenRenderer::getComponents(void) const
{
  return this->components;
}

/*!
  Sets the viewport region.
*/
void
SoOffscreenRenderer::setViewportRegion(const SbViewportRegion & region)
{
  this->viewport = region;

  if (this->renderaction)
    this->renderaction->setViewportRegion(region);

  SbVec2s dims = region.getViewportSizePixels();
  if (this->internaldata) this->internaldata->setBufferSize(dims);

  delete[] this->buffer;
  this->buffer = new unsigned char[dims[0] * dims[1] * this->getComponents()];
}

/*!
  Returns the viewerport region.
*/
const SbViewportRegion &
SoOffscreenRenderer::getViewportRegion(void) const
{
  return this->viewport;
}

/*!
  Sets the background color. The buffer is cleared to this color
  before rendering.
*/
void
SoOffscreenRenderer::setBackgroundColor(const SbColor & color)
{
  this->backgroundcolor = color;
}

/*!
  Returns the background color.
*/
const SbColor &
SoOffscreenRenderer::getBackgroundColor(void) const
{
  return this->backgroundcolor;
}

/*!
  Sets the render action. Use this if you have special rendering needs.
*/
void
SoOffscreenRenderer::setGLRenderAction(SoGLRenderAction * action)
{
  if (this->didallocaction) delete this->renderaction;
  this->renderaction = action;
  this->didallocaction = FALSE;
}

/*!
  Returns the rendering action currently used.
*/
SoGLRenderAction *
SoOffscreenRenderer::getGLRenderAction(void) const
{
  return this->renderaction;
}

static void
pre_render_cb(void * userdata, SoGLRenderAction * action)
{
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  action->removePreRenderCallback(pre_render_cb, userdata);
}

// Collects common code from the two render() functions.
SbBool
SoOffscreenRenderer::renderFromBase(SoBase * base)
{
  uint32_t oldcontext = this->renderaction->getCacheContext();
  if (this->internaldata && 
      this->internaldata->makeContextCurrent(oldcontext)) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(this->backgroundcolor[0],
                 this->backgroundcolor[1],
                 this->backgroundcolor[2],
                 0.0f);

    // needed to clear viewport after glViewport is called
    this->renderaction->addPreRenderCallback(pre_render_cb, NULL);

    if (!this->didallocaction) {
      this->renderaction->setCacheContext(SoGLCacheContextElement::getUniqueCacheContext());
    }
    if (base->isOfType(SoNode::getClassTypeId()))
      this->renderaction->apply((SoNode *)base);
    else if (base->isOfType(SoPath::getClassTypeId()))
      this->renderaction->apply((SoPath *)base);
    else assert(FALSE && "impossible");

    this->internaldata->postRender();
    this->convertBuffer();

    if (!this->didallocaction) {
      this->renderaction->setCacheContext(oldcontext);
    }
    return TRUE;
  }
  return FALSE;
}

// Convert from RGBA format to the application programmer's requested
// format.
void
SoOffscreenRenderer::convertBuffer(void)
{
  SbVec2s dims = this->getViewportRegion().getViewportSizePixels();
  int pixels = dims[0] * dims[1];
  int depth = this->getComponents();
  unsigned char * nativebuffer = this->internaldata->getBuffer();

  switch (this->getComponents()) {
  case SoOffscreenRenderer::RGB_TRANSPARENCY:
    memcpy(this->buffer, nativebuffer, pixels * depth);
    break;

  case SoOffscreenRenderer::RGB:
    {
      unsigned char * native = nativebuffer;
      unsigned char * local = this->buffer;
      for (int i=0; i < pixels; i++) {
        *local++ = *native++;
        *local++ = *native++;
        *local++ = *native++;
        native++;
      }
    }
    break;

  case SoOffscreenRenderer::LUMINANCE_TRANSPARENCY:
    {
      unsigned char * native = nativebuffer;
      unsigned char * local = this->buffer;
      for (int i=0; i < pixels; i++) {
        int val = (int(*native++) + int(*native++) + int(*native++)) / 3;
        *local++ = (unsigned char)val;
        *local++ = *native++;
      }
    }
    break;

  case SoOffscreenRenderer::LUMINANCE:
    {
      unsigned char * native = nativebuffer;
      unsigned char * local = this->buffer;
      for (int i=0; i < pixels; i++) {
        uint32_t val = 76*native[0]+155*native[1]+26*native[2];
        *local++ = (unsigned char)(val>>8);
        native += 4;
      }
    }
    break;

  default:
    assert(FALSE && "unknown buffer format"); break;
  }
}

/*!
  Render the scenegraph rooted at \a scene into our internal pixel
  buffer.


  Important note: make sure you pass in a \a scene node pointer which
  has both a camera and at least one lightsource below it -- otherwise
  you are likely to end up with just a blank or black image buffer.

  This mistake is easily made if you use an SoOffscreenRenderer on a
  scenegraph from one of the standard viewer components, as you will
  often just leave the addition of a camera and a headlight
  lightsource to the viewer to set up. This camera and lightsource are
  then part of the viewer's private "super-graph" outside of the scope
  of the scenegraph passed in by the application programmer. To make
  sure the complete scenegraph (including the viewer's "private parts"
  (*snicker*)) are passed to this method, you can get the scenegraph
  root from the viewer's internal SoSceneManager instance instead of
  from the viewer's own getSceneGraph() method, like this:

  \code
  SoOffscreenRenderer * myRenderer = new SoOffscreenRenderer(vpregion);
  SoNode * root = myViewer->getSceneManager()->getSceneGraph();
  SbBool ok = myRenderer->render(root);
  // [then use image buffer in a texture, or write it to file, or whatever]
  \endcode

  If you do this and still get a blank buffer, another common problem
  is to have a camera which is not actually pointing at the scene
  geometry you want a snapshot of. If you suspect that could be the
  cause of problems on your end, take a look at SoCamera::pointAt()
  and SoCamera::viewAll() to see how you can make a camera node
  guaranteed to be directed at the scene geometry.

  \sa writeToRGB()
*/
SbBool
SoOffscreenRenderer::render(SoNode * scene)
{
  return this->renderFromBase(scene);
}

/*!
  Render the \a scene path into our internal memory buffer.
*/
SbBool
SoOffscreenRenderer::render(SoPath * scene)
{
  return this->renderFromBase(scene);
}

/*!
  Returns the offscreen memory buffer.
*/
unsigned char *
SoOffscreenRenderer::getBuffer(void) const
{
  return this->buffer;
}

//
// avoid endian problems (little endian sucks, right? :)
//
static int
write_short(FILE * fp, unsigned short val)
{
  unsigned char tmp[2];
  tmp[0] = (unsigned char)(val >> 8);
  tmp[1] = (unsigned char)(val & 0xff);
  return fwrite(&tmp, 2, 1, fp);
}

/*!
  Writes the buffer in SGI RGB format by appending it to the already
  open file. Returns \c FALSE if writing fails.

  Important note: do \e not use this method when the Coin library has
  been compiled as an MSWindows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for MSWindows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToRGB(FILE * fp) const
{
  // FIXME: add code to rle rows, pederb 2000-01-10
  // FIXME: errorchecking when writing! 20010625 mortene.

  if (this->internaldata) {
    SbVec2s size = this->internaldata->getSize();
    write_short(fp, 0x01da); // imagic
    write_short(fp, 0x0001); // raw (no rle yet)

    unsigned short comp = (unsigned short)this->getComponents();
    if (comp == 1)
      write_short(fp, 0x0002); // 2 dimensions (heightmap)
    else
      write_short(fp, 0x0003); // 3 dimensions

    write_short(fp, (unsigned short) size[0]);
    write_short(fp, (unsigned short) size[1]);
    write_short(fp, (unsigned short) comp);

    unsigned char buf[500];
    memset(buf, 0, 500);
    buf[7] = 255; // set maximum pixel value to 255
    strcpy((char *)buf+8, "http://www.coin3d.org");
    fwrite(buf, 1, 500, fp);

    unsigned char * tmpbuf = new unsigned char[size[0]];

    unsigned char * ptr = this->buffer;
    for (int c = 0; c < comp; c++) {
      for (int y = 0; y < size[1]; y++) {
        for (int x = 0; x < size[0]; x++) {
          tmpbuf[x] = ptr[x * comp + y * comp * size[0] + c];
        }
        fwrite(tmpbuf, 1, size[0], fp);
      }
    }
    delete [] tmpbuf;
    return TRUE;
  }
  return FALSE;
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  SGI RGB format to the new file. If the file already exists, it will
  be overwritten (if permitted by the filesystem).

  Returns \c TRUE if all went ok, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToRGB(const char * filename) const
{
  FILE * rgbfp = fopen(filename, "wb");
  if (!rgbfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToRGB",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToRGB(rgbfp);
  (void)fclose(rgbfp);
  return result;
}

static int
encode_ascii85(const unsigned char * in, unsigned char * out)
{
  uint32_t data =
    (uint32_t(in[0])<<24) |
    (uint32_t(in[1])<<16) |
    (uint32_t(in[2])<< 8) |
    (uint32_t(in[3]));

  if (data == 0) {
    out[0] = 'z';
    return 1;
  }
  out[4] = (unsigned char) (data%85 + '!');
  data /= 85;
  out[3] = (unsigned char) (data%85 + '!');
  data /= 85;
  out[2] = (unsigned char) (data%85 + '!');
  data /= 85;
  out[1] = (unsigned char) (data%85 + '!');
  data /= 85;
  out[0] = (unsigned char) (data%85 + '!');
  return 5;
}

static void
output_ascii85(FILE * fp,
               const unsigned char val,
               unsigned char * tuple,
               unsigned char * linebuf,
               int & tuplecnt, int & linecnt,
               const int rowlen,
               const SbBool flush = FALSE)
{
  int i;
  if (flush) {
    // fill up tuple
    for (i = tuplecnt; i < 4; i++) tuple[i] = 0;
  }
  else {
    tuple[tuplecnt++] = val;
  }
  if (flush || tuplecnt == 4) {
    if (tuplecnt) {
      int add = encode_ascii85(tuple, linebuf + linecnt);
      if (flush) {
        if (add == 1) {
          for (i = 0; i < 5; i++) linebuf[linecnt+i] = '!';
        }
        linecnt += tuplecnt + 1;
      }
      else linecnt += add;
      tuplecnt = 0;
    }
    if (linecnt >= rowlen) {
      unsigned char store = linebuf[rowlen];
      linebuf[rowlen] = 0;
      fprintf(fp, "%s\n", linebuf);
      linebuf[rowlen] = store;
      for (i = rowlen; i < linecnt; i++) {
        linebuf[i-rowlen] = linebuf[i];
      }
      linecnt -= rowlen;
    }
    if (flush && linecnt) {
      linebuf[linecnt] = 0;
      fprintf(fp, "%s\n", linebuf);
    }
  }
}

static void
flush_ascii85(FILE * fp,
              unsigned char * tuple,
              unsigned char * linebuf,
              int & tuplecnt, int & linecnt,
              const int rowlen)
{
  output_ascii85(fp, 0, tuple, linebuf, tuplecnt, linecnt, rowlen, TRUE);
}

/*!
  Writes the buffer in Postscript format by appending it to the
  already open file. Returns \c FALSE if writing fails.

  Important note: do \e not use this method when the Coin library has
  been compiled as an MSWindows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for MSWindows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(FILE * fp) const
{
  // just choose a page size of 8.5 x 11 inches (A4)
  return this->writeToPostScript(fp, SbVec2f(8.5f, 11.0f));
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  Postscript format to the new file. If the file already exists, it
  will be overwritten (if permitted by the filesystem).

  Returns \c TRUE if all went ok, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(const char * filename) const
{
  FILE * psfp = fopen(filename, "wb");
  if (!psfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToPostScript",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToPostScript(psfp);
  (void)fclose(psfp);
  return result;
}

/*!
  Writes the buffer to a file in Postscript format, with \a printsize
  dimensions.

  Important note: do \e not use this method when the Coin library has
  been compiled as an MSWindows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for MSWindows DLLs.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(FILE * fp,
                                       const SbVec2f & printsize) const
{
  if (this->internaldata) {
    const SbVec2s size = this->internaldata->getSize();
    const int nc = this->getComponents();
    const float defaultdpi = 72.0f; // we scale against this value 
    const float dpi = this->getScreenPixelsPerInch();
    const SbVec2s pixelsize((short)(printsize[0]*defaultdpi),
                            (short)(printsize[1]*defaultdpi));
    
    const unsigned char * src = this->buffer;
    const int chan = nc <= 2 ? 1 : 3;
    const SbVec2s scaledsize((short) ceil(size[0]*defaultdpi/dpi),
                             (short) ceil(size[1]*defaultdpi/dpi));
    
    fprintf(fp, "%%!PS-Adobe-2.0 EPSF-1.2\n");
    fprintf(fp, "%%%%Pages: 1\n");
    fprintf(fp, "%%%%PageOrder: Ascend\n");
    fprintf(fp, "%%%%BoundingBox: 0 %d %d %d\n", 
            pixelsize[1]-scaledsize[1], 
            scaledsize[0],
            pixelsize[1]);
    fprintf(fp, "%%%%Creator: Coin <http://www.coin3d.org\n");
    fprintf(fp, "%%%%EndComments\n");

    fprintf(fp, "\n");
    fprintf(fp, "/origstate save def\n");
    fprintf(fp, "\n");
    fprintf(fp, "%% workaround for bug in some PS interpreters\n"); 
    fprintf(fp, "%% which doesn't skip the ASCII85 EOD marker.\n");     
    fprintf(fp, "/~ {currentfile read pop pop} def\n\n");
    fprintf(fp, "/image_wd %d def\n", size[0]);
    fprintf(fp, "/image_ht %d def\n", size[1]);
    fprintf(fp, "/pos_wd %d def\n", size[0]);
    fprintf(fp, "/pos_ht %d def\n", size[1]);
    fprintf(fp, "/image_dpi %g def\n", dpi);
    fprintf(fp, "/image_scale %g image_dpi div def\n", defaultdpi);
    fprintf(fp, "/image_chan %d def\n", chan);
    fprintf(fp, "/xpos_offset 0 image_scale mul def\n");
    fprintf(fp, "/ypos_offset 0 image_scale mul def\n");
    fprintf(fp, "/pix_buf_size %d def\n\n", size[0]*chan);
    fprintf(fp, "/page_ht %g %g mul def\n", printsize[1], defaultdpi);
    fprintf(fp, "/page_wd %g %g mul def\n", printsize[0], defaultdpi);
    fprintf(fp, "/image_xpos 0 def\n");
    fprintf(fp, "/image_ypos page_ht pos_ht image_scale mul sub def\n");
    fprintf(fp, "image_xpos xpos_offset add image_ypos ypos_offset add translate\n");
    fprintf(fp, "\n");
    fprintf(fp, "/pix pix_buf_size string def\n");
    fprintf(fp, "image_wd image_scale mul image_ht image_scale mul scale\n");
    fprintf(fp, "\n");
    fprintf(fp, "image_wd image_ht 8\n");
    fprintf(fp, "[image_wd 0 0 image_ht 0 0]\n");
    fprintf(fp, "currentfile\n");
    fprintf(fp, "/ASCII85Decode filter\n");
    // fprintf(fp, "/RunLengthDecode filter\n"); // FIXME: add later
    if (chan == 3) fprintf(fp, "false 3\ncolorimage\n");
    else fprintf(fp,"image\n");

    const int rowlen = 72;
    int num = size[0] * size[1];
    unsigned char tuple[4];
    unsigned char linebuf[rowlen+5];
    int tuplecnt = 0;
    int linecnt = 0;
    int cnt = 0;
    while (cnt < num) {
      switch (nc) {
      default: // avoid warning
      case 1:
        output_ascii85(fp, src[cnt], tuple, linebuf, tuplecnt, linecnt, rowlen);
        break;
      case 2:
        output_ascii85(fp, src[cnt*2], tuple, linebuf, tuplecnt, linecnt, rowlen);
        break;
      case 3:
        output_ascii85(fp, src[cnt*3], tuple, linebuf, tuplecnt, linecnt, rowlen);
        output_ascii85(fp, src[cnt*3+1], tuple, linebuf, tuplecnt, linecnt, rowlen);
        output_ascii85(fp, src[cnt*3+2], tuple, linebuf, tuplecnt, linecnt, rowlen);
        break;
      case 4:
        output_ascii85(fp, src[cnt*4], tuple, linebuf, tuplecnt, linecnt, rowlen);
        output_ascii85(fp, src[cnt*4+1], tuple, linebuf, tuplecnt, linecnt,rowlen);
        output_ascii85(fp, src[cnt*4+2], tuple, linebuf, tuplecnt, linecnt, rowlen);
        break;
      }
      cnt++;
    }

    // flush data in ascii85 encoder
    flush_ascii85(fp, tuple, linebuf, tuplecnt, linecnt, rowlen);

    fprintf(fp, "~>\n\n"); // ASCII85 EOD marker
    fprintf(fp, "origstate restore\n");
    fprintf(fp, "\n");
    fprintf(fp, "%%%%Trailer\n");
    fprintf(fp, "\n");
    fprintf(fp, "%%%%EOF\n");
    return (SbBool) (ferror(fp) == 0);
  }
  return FALSE;
}

/*!
  Opens a file with the given name and writes the offscreen buffer in
  Postscript format with \a printsize dimensions to the new file. If
  the file already exists, it will be overwritten (if permitted by the
  filesystem).

  Returns \c TRUE if all went ok, otherwise \c FALSE.
*/
SbBool
SoOffscreenRenderer::writeToPostScript(const char * filename,
                                       const SbVec2f & printsize) const
{
  FILE * psfp = fopen(filename, "wb");
  if (!psfp) {
    SoDebugError::postWarning("SoOffscreenRenderer::writeToPostScript",
                              "couldn't open file '%s'", filename);
    return FALSE;
  }
  SbBool result = this->writeToPostScript(psfp, printsize);
  (void)fclose(psfp);
  return result;
}

// FIXME: the file format support checking could have been done
// better, for instance by using MIME types. 20020206 mortene.

/*!
  Returns \c TRUE if the buffer can be saved as a file of type \a
  filetypeextension, using SoOffscreenRenderer::writeToFile().  This
  function needs simage v1.1 or newer.

  Examples of possibly supported extensions are: "jpg", "png", "tiff",
  "gif", "bmp", etc. The extension match is not case sensitive.

  Which formats are \e actually supported depends on the capabilities
  of Coin's support library for handling import and export of
  pixel-data files: the simage library. If the simage library is not
  installed on your system, no extension output formats will be
  supported.

  Also, note that it is possible to build and install a simage library
  that lacks support for most or all of the file formats it is \e
  capable of supporting. This is so because the simage library depends
  on other, external 3rd party libraries -- in the same manner as Coin
  depends on the simage library for added file format support.

  The two built-in formats that are supported through the
  SoOffscreenRenderer::writeToRGB() and
  SoOffscreenRenderer::writeToPostScript() methods (for SGI RGB format
  and for Adobe Postscript files, respectively) are \e not considered
  by this method, as those two formats are guaranteed to \e always be
  supported through those functions.

  So if you want to be guaranteed to be able to export a screenshot in
  your wanted format, you will have to use either one of the above
  mentioned method for writing SGI RGB or Adobe Postscript directly,
  or make sure the Coin library has been built and is running on top
  of a version of the simage library (that you have preferably built
  yourself) with the file format you want known to have been included.


  This method is an extension versus the original SGI Open Inventor
  API.

  \sa  getNumWriteFiletypes(), getWriteFiletypeInfo(), writeToFile()
*/
SbBool
SoOffscreenRenderer::isWriteSupported(const SbName & filetypeextension) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoOffscreenRenderer::isWriteSupported",
                           "You need simage v1.1 for this functionality.");
#endif // COIN_DEBUG
    return FALSE;
  }
  int ret = simage_wrapper()->simage_check_save_supported(filetypeextension.getString());
  return ret ? TRUE : FALSE;
}

/*!
  Returns the number of available exporters. Detailed information
  about the exporters can then be found using getWriteFiletypeInfo().

  See SoOffscreenRenderer::isWriteSupported() for information about
  which file formats you can expect to be present.

  Note that the two built-in export formats (SGI RGB and Adobe
  Postscript) are not counted.

  This method is an extension versus the original SGI Open Inventor
  API.

  \sa getWriteFiletypeInfo()
*/
int
SoOffscreenRenderer::getNumWriteFiletypes(void) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoOffscreenRenderer::getNumWriteFiletypes",
                           "You need simage v1.1 for this functionality.");
#endif // COIN_DEBUG
    return 0;
  }
  return simage_wrapper()->simage_get_num_savers();
}

/*!
  Returns information about an image exporter. \a extlist is a list of
  filenameextensions for a file format. E.g. for JPEG it is legal to
  use both jpg and jpeg. \a fullname is the full name of the image
  format. \a description is an optional string with more information
  about the file format.

  See SoOffscreenRenderer::isWriteSupported() for information about
  which file formats you can expect to be present.

  This method is an extension versus the original SGI Open Inventor
  API.

  \sa getNumWriteFiletypes(), writeToFile()
*/
void
SoOffscreenRenderer::getWriteFiletypeInfo(const int idx,
                                          SbList <SbName> & extlist,
                                          SbString & fullname,
                                          SbString & description)
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoOffscreenRenderer::getNumWriteFiletypes",
                           "You need simage v1.1 for this functionality.");
#endif // COIN_DEBUG
    return;
  }
  extlist.truncate(0);
  assert(idx >= 0 && idx < this->getNumWriteFiletypes());
  void * saver = simage_wrapper()->simage_get_saver_handle(idx);
  SbString allext(simage_wrapper()->simage_get_saver_extensions(saver));
  const char * start = allext.getString();
  const char * curr = start;
  const char * end = strchr(curr, ',');
  while (end) {
    SbString ext = allext.getSubString(curr-start, end-start-1);
    extlist.append(SbName(ext.getString()));
    curr = end+1;
    end = strchr(curr, ',');
  }
  SbString ext = allext.getSubString(curr-start);
  extlist.append(SbName(ext.getString()));
  const char * fullname_s = simage_wrapper()->simage_get_saver_fullname(saver);
  const char * desc_s = simage_wrapper()->simage_get_saver_description(saver);
  fullname = fullname_s ? SbString(fullname_s) : SbString("");
  description = desc_s ? SbString(desc_s) : SbString("");
}

/*!
  Saves the buffer to \a filename, in the filetype specified by \a
  filetypeextensions.

  This method is an extension versus the orignal SGI Open Inventor
  API.

  \sa isWriteSupported()
*/
SbBool
SoOffscreenRenderer::writeToFile(const SbString & filename, const SbName & filetypeextension) const
{
  if (!simage_wrapper()->versionMatchesAtLeast(1,1,0)) {
    return FALSE;
  }
  if (this->internaldata) {
    SbVec2s size = this->internaldata->getSize();
    int comp = (int) this->getComponents();
    unsigned char * bytes = this->buffer;
    int ret = simage_wrapper()->simage_save_image(filename.getString(),
                                                  bytes,
                                                  int(size[0]), int(size[1]), comp,
                                                  filetypeextension.getString());
    return ret ? TRUE : FALSE;
  }
  return FALSE;
}
