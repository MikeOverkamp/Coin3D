/**************************************************************************\
 *
 *  Copyright (C) 1998-2000 by Systems in Motion.  All rights reserved.
 *
 *  This file is part of the Coin library.
 *
 *  This file may be distributed under the terms of the Q Public License
 *  as defined by Troll Tech AS of Norway and appearing in the file
 *  LICENSE.QPL included in the packaging of this file.
 *
 *  If you want to use Coin in applications not covered by licenses
 *  compatible with the QPL, you can contact SIM to aquire a
 *  Professional Edition license for Coin.
 *
 *  Systems in Motion AS, Prof. Brochs gate 6, N-7030 Trondheim, NORWAY
 *  http://www.sim.no/ sales@sim.no Voice: +47 22114160 Fax: +47 67172912
 *
\**************************************************************************/

/*!
  \class SoCreaseAngleElement Inventor/elements/SoCreaseAngleElement.h
  \brief The SoCreaseAngleElement class stores the crease angle during a
  scene graph traversal.
*/

#include <Inventor/elements/SoCreaseAngleElement.h>


#include <assert.h>

SO_ELEMENT_SOURCE(SoCreaseAngleElement);

/*!
  This static method initializes static data for the
  SoCreaseAngleElement class.
*/

void
SoCreaseAngleElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoCreaseAngleElement, inherited);
}

/*!
  The destructor.
*/

SoCreaseAngleElement::~SoCreaseAngleElement(// virtual protected
    void)
{
}

//! FIXME: write doc.

void
SoCreaseAngleElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

//$ EXPORT INLINE
void
SoCreaseAngleElement::set(SoState * const state, SoNode * const node,
                          const float complexity)
{
  SoFloatElement::set(classStackIndex,state,node, complexity);
}

//! FIXME: write doc.

//$ EXPORT INLINE
void
SoCreaseAngleElement::set(SoState * const state, const float complexity)
{
  set(state, NULL, complexity);
}

//! FIXME: write doc.

//$ EXPORT INLINE
float
SoCreaseAngleElement::get(SoState * const state)
{
  return SoFloatElement::get(classStackIndex, state);
}

//! FIXME: write doc.

//$ EXPORT INLINE
float
SoCreaseAngleElement::getDefault()
{
  return 0.0f;
}
