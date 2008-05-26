#ifndef COIN_SOTEXTURECOORDINATEPLANE_H
#define COIN_SOTEXTURECOORDINATEPLANE_H

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2008 by Kongsberg SIM.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Kongsberg SIM about acquiring
 *  a Coin Professional Edition License.
 *
 *  See http://www.coin3d.org/ for more information.
 *
 *  Kongsberg SIM, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  http://www.sim.no/  sales@sim.no  coin-support@coin3d.org
 *
\**************************************************************************/

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoTextureCoordinateFunction.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoTextureCoordinatePlaneP;

class COIN_DLL_API SoTextureCoordinatePlane : public SoTextureCoordinateFunction {
  typedef SoTextureCoordinateFunction inherited;

  SO_NODE_HEADER(SoTextureCoordinatePlane);

public:
  static void initClass(void);
  SoTextureCoordinatePlane(void);

  SoSFVec3f directionS;
  SoSFVec3f directionT;
  SoSFVec3f directionR;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void pick(SoPickAction * action);
  virtual void callback(SoCallbackAction * action);

protected:
  virtual ~SoTextureCoordinatePlane();

private:
  static const SbVec4f &generate(void *userdata,
                                 const SbVec3f &p,
                                 const SbVec3f &n);
  static void handleTexgen(void *data);

  SbPimplPtr<SoTextureCoordinatePlaneP> pimpl;
  void setupGencache(void);
};

#endif // !COIN_SOTEXTURECOORDINATEPLANE_H
