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
  \class SoSFRotation SoSFRotation.h Inventor/fields/SoSFRotation.h
  \brief The SoSFRotation class is a container for an SbRotation.
  \ingroup fields

  This field is used where nodes, engines or other field containers
  needs to store a single rotation definition.

  Fields of this type stores their value to file as a rotation axis
  vector plus a rotation angle: "axis0 axis1 axis2 angle".


  Note that there is one \e very common mistake that is easy to make
  when setting the value of an SoSFRotation field, and that is to
  inadvertently use the wrong SbRotation constructor. This example
  should clarify the problem:

  \code
  mytransformnode->rotation.setValue(0, 0, 1, 1.5707963f);
  \endcode

  The programmer clearly tries to set a PI/2 rotation around the Z
  axis, but this will fail, as the SbRotation constructor invoked
  above is the one that takes as arguments the 4 floats of a \e
  quaternion. What the programmer almost certainly wanted to do was to
  use the SbRotation constructor that takes a rotation vector and a
  rotation angle, which is invoked like this:

  \code
  mytransformnode->rotation.setValue(SbVec3f(0, 0, 1), 1.5707963f);
  \endcode

  \sa SoMFRotation
*/

#include <Inventor/fields/SoSFRotation.h>

#include <Inventor/SbVec3f.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoSubFieldP.h>

extern SbBool sofield_read_float_values(SoInput * in, float * val, int numvals);

SO_SFIELD_SOURCE(SoSFRotation, SbRotation, const SbRotation &);


// Override from parent.
void
SoSFRotation::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFRotation);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

// Read rotation value from input stream, return TRUE if
// successful. Also used from SoMFRotation class.
SbBool
sosfrotation_read_value(SoInput * in, SbRotation & r)
{
  float f[4];
  if (!sofield_read_float_values(in, f, 4)) { return FALSE; }

  SbVec3f axis(f[0], f[1], f[2]);
  const float angle = f[3];

  // vrml97 identity rotations are often specified with a null vector.
  // test for this and just set to z-axis.
  if (axis == SbVec3f(0.0f, 0.0f, 0.0f) && angle == 0.0f) {
    axis = SbVec3f(0.0f, 0.0f, 1.0f);
  }
  r.setValue(axis, angle);
  return TRUE;
}

SbBool
SoSFRotation::readValue(SoInput * in)
{
  SbRotation r;
  if (!sosfrotation_read_value(in, r)) return FALSE;
  this->setValue(r);
  return TRUE;
}

// Write SbRotation to output stream. Also used from SoMFRotation
// class.
void
sosfrotation_write_value(SoOutput * out, const SbRotation & r)
{
  SbVec3f axis;
  float angle;
  r.getValue(axis, angle);

  // Handle invalid rotation specifications.
  if (axis.length() == 0.0f) {
    axis.setValue(0.0f, 0.0f, 1.0f);
    angle = 0.0f;
  }

  out->write(axis[0]);
  if(!out->isBinary()) out->write(' ');
  out->write(axis[1]);
  if(!out->isBinary()) out->write(' ');
  out->write(axis[2]);
  if(!out->isBinary()) out->write("  ");
  out->write(angle);
}

void
SoSFRotation::writeValue(SoOutput * out) const
{
  sosfrotation_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS


/*!
  Return value of rotation as an \a axis and an \a angle around this
  \a axis.
*/
void
SoSFRotation::getValue(SbVec3f & axis, float & angle) const
{
  this->getValue().getValue(axis, angle);
}

/*!
  Set the rotation from a set of quaternion values.
*/
void
SoSFRotation::setValue(const float q0, const float q1,
                       const float q2, const float q3)
{
  this->setValue(SbRotation(q0, q1, q2, q3));
}

/*!
  Set the rotation from a set of quaternion values.
*/
void
SoSFRotation::setValue(const float q[4])
{
  this->setValue(SbRotation(q));
}

/*!
  Set the rotation from an \a axis and an \a angle around
  this \a axis.
*/
void
SoSFRotation::setValue(const SbVec3f & axis, const float angle)
{
  this->setValue(SbRotation(axis, angle));
}
