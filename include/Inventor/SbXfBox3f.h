#ifndef COIN_SBXFBOX3F_H
#define COIN_SBXFBOX3F_H

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

#include <stdio.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbMatrix.h>

class COIN_DLL_API SbXfBox3f : public SbBox3f {
  typedef SbBox3f inherited;

public:
  SbXfBox3f();
  SbXfBox3f(const SbVec3f &_min, const SbVec3f &_max);
  SbXfBox3f(const SbBox3f &box);
  ~SbXfBox3f();

  void setTransform(const SbMatrix &m);
  const SbMatrix &getTransform() const;
  const SbMatrix &getInverse() const;
  SbVec3f getCenter() const;
  void extendBy(const SbVec3f &pt);
  void extendBy(const SbBox3f &bb);
  void extendBy(const SbXfBox3f &bb);
  SbBool intersect(const SbVec3f &pt) const;
  SbBool intersect(const SbBox3f &bb) const;
  void getSpan(const SbVec3f &direction, float &dMin, float &dMax) const;
  SbBox3f project() const;
  friend COIN_DLL_API int operator ==(const SbXfBox3f &b1, const SbXfBox3f &b2);
  friend COIN_DLL_API int operator !=(const SbXfBox3f &b1, const SbXfBox3f &b2);
  // Must override the transform() method from SbBox3f, as the box and
  // the transform matrix are supposed to be kept separate in
  // SbXfBox3f. --mortene
  void transform(const SbMatrix & m);
  // Overridden from SbBox3f
  float getVolume(void) const;

  void print(FILE * file) const;

private:
  // incorrect for SbXfBox3f. Hide them
  const SbVec3f & getMin(void) const {return SbBox3f::getMin(); }
  const SbVec3f & getMax(void) const { return SbBox3f::getMax(); }

  void calcInverse(void) const;
  void makeInvInvalid(void);

  SbMatrix matrix, invertedmatrix;
};

COIN_DLL_API int operator ==(const SbXfBox3f &b1, const SbXfBox3f &b2);
COIN_DLL_API int operator !=(const SbXfBox3f &b1, const SbXfBox3f &b2);

#endif // !COIN_SBXFBOX3F_H
