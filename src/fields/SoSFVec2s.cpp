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
  \class SoSFVec2s SoSFVec2s.h Inventor/fields/SoSFVec2s.h
  \brief The SoSFVec2s class is a container for an SbVec2s vector.
  \ingroup fields

  This field is used where nodes, engines or other field containers
  needs to store a single vector with two elements.

  \COIN_CLASS_EXTENSION

  \sa SoMFVec2s
  \since Coin 2.0
*/

#include <Inventor/fields/SoSFVec2s.h>
#include <Inventor/fields/SoSubFieldP.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG



SO_SFIELD_SOURCE(SoSFVec2s, SbVec2s, const SbVec2s &);


// Override from parent class.
void
SoSFVec2s::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFVec2s);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
SoSFVec2s::readValue(SoInput * in)
{
  return 
    in->read(this->value[0]) &&
    in->read(this->value[1]);
}

// Write integer value to output stream. Also used from SoMFVec2s
// class.
void
sosfvec2s_write_value(SoOutput * out, const SbVec2s & v)
{
  out->write(v[0]);
  if (!out->isBinary()) out->write(' ');
  out->write(v[1]);
}

void
SoSFVec2s::writeValue(SoOutput * out) const
{
  sosfvec2s_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS


/*!
  Set value of vector.
*/
void
SoSFVec2s::setValue(const short x, const short y)
{
  this->setValue(SbVec2s(x, y));
}

/*!
  Set value of vector.
*/
void
SoSFVec2s::setValue(const short xy[2])
{
  this->setValue(SbVec2s(xy));
}
