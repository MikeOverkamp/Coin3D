#ifndef COIN_SBGLUTESSELLATOR_H
#define COIN_SBGLUTESSELLATOR_H

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2005 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org/> for more information.
 *
 *  Systems in Motion, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  <URL:http://www.sim.no/>.
 *
\**************************************************************************/

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* ! COIN_INTERNAL */

// *************************************************************************

#include <Inventor/SbVec3f.h>
#include <Inventor/C/glue/GLUWrapper.h>
#include <Inventor/lists/SbList.h>

// *************************************************************************

class SbGLUTessellator {
public:
  static SbBool available(void);

  SbGLUTessellator(void (*callback)(void * v0, void * v1, void * v2,
                                    void * data) = NULL, void * userdata = NULL);
  ~SbGLUTessellator(void);

  void beginPolygon(const SbVec3f & normal = SbVec3f(0.0f, 0.0f, 0.0f));
  void addVertex(const SbVec3f & v, void * data);
  void endPolygon(void);

  static SbBool preferred(void);

private:
  static void APIENTRY cb_begin(GLenum primitivetype, void * x);
  static void APIENTRY cb_vertex(void * vertex_data, void * x);
  static void APIENTRY cb_error(GLenum err, void * x);

  void (* callback)(void *, void *, void *, void *);
  void * cbdata;
  coin_GLUtessellator * tessobj;

  struct v { GLdouble c[3]; };
  SbList<struct v> coords;

  GLenum triangletessmode;
  unsigned int vertexidx;
  void * vertexdata[2];
  SbBool stripflipflop;
};

#endif // !COIN_SBGLUTESSELLATOR_H
