/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
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
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class SoInterpolateVec4f SoInterpolateVec4f.h Inventor/engines/SoInterpolateVec4f.h
  \brief The SoInterpolateVec4f class is used to interpolate between pairs of 4D vectors.
  \ingroup engines
*/

#include <Inventor/engines/SoInterpolateVec4f.h>
#include <Inventor/engines/SoSubEngineP.h>

/*!
  \var SoMFVec4f SoInterpolateVec4f::input0
  First set of input values.
*/
/*!
  \var SoMFVec4f SoInterpolateVec4f::input1
  Second set of input values.
*/

SO_INTERPOLATE_INTERNAL_INIT_CLASS(SoInterpolateVec4f);

SO_INTERPOLATE_INTERNAL_SOURCE(SoInterpolateVec4f,
                               SoMFVec4f,
                               SbVec4f,
                               (0.0f,0.0f,0.0f,0.0f),
                               (0.0f,0.0f,0.0f,0.0f),
                               (v1-v0)*a+v0);
