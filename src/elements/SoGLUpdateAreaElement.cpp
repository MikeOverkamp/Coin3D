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
  \class SoGLUpdateAreaElement Inventor/elements/SoGLUpdateAreaElement.h
  \brief The SoGLUpdateAreaElement class is yet to be documented.

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLUpdateAreaElement.h>

#include <coindefs.h> // COIN_STUB()

#include <assert.h>

/*!
  \fn SoGLUpdateAreaElement::origin

  FIXME: write doc
*/

/*!
  \fn SoGLUpdateAreaElement::size

  FIXME: write doc
*/

SO_ELEMENT_SOURCE(SoGLUpdateAreaElement);

/*!
  This static method initializes static data for the
  SoGLUpdateAreaElement class.
*/

void
SoGLUpdateAreaElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLUpdateAreaElement, inherited);
}

/*!
  The destructor.
*/

SoGLUpdateAreaElement::~SoGLUpdateAreaElement()
{
}

//! FIXME: write doc.

void
SoGLUpdateAreaElement::init(SoState * state)
{
  inherited::init(state);
  origin = getDefaultOrigin();
  size = getDefaultSize();
}

//! FIXME: write doc.

void
SoGLUpdateAreaElement::push(SoState * state)
{
    inherited::push(state);
}

//! FIXME: write doc.

void
SoGLUpdateAreaElement::pop(SoState * state,
                           const SoElement * prevTopElement)
{
    inherited::pop(state, prevTopElement);
}

//! FIXME: write doc.

SbBool
SoGLUpdateAreaElement::matches(const SoElement * /* element */) const
{
  COIN_STUB();
  return FALSE;
}

//! FIXME: write doc.

SoElement *
SoGLUpdateAreaElement::copyMatchInfo() const
{
  COIN_STUB();
  return NULL;
}

//! FIXME: write doc.

void
SoGLUpdateAreaElement::set(SoState * const state,
                           const SbVec2f & origin,
                           const SbVec2f & size)
{
  SoGLUpdateAreaElement * e = (SoGLUpdateAreaElement *)
    inherited::getElement(state, SoGLUpdateAreaElement::classStackIndex);
  e->origin = origin;
  e->size = size;
}

//! FIXME: write doc.

SbBool
SoGLUpdateAreaElement::get(SoState * const state,
                           SbVec2f & origin,
                           SbVec2f & size)
{
  const SoGLUpdateAreaElement * e = (const SoGLUpdateAreaElement *)
    inherited::getConstElement(state, SoGLUpdateAreaElement::classStackIndex);
  origin = e->origin;
  size = e->size;

  // FIXME: keep a seperate flag instead? 990314 mortene.
  if (origin == SbVec2f(0.0f, 0.0f) && size == SbVec2f(1.0f, 1.0f))
    return TRUE;
  else
    return FALSE;
}

//! FIXME: write doc.

/*!
  FIXME: write doc.
*/

SbVec2f
SoGLUpdateAreaElement::getDefaultOrigin(void)
{
  return SbVec2f(0.0f, 0.0f);
}

/*!
  FIXME: write doc.
*/

SbVec2f
SoGLUpdateAreaElement::getDefaultSize(void)
{
  return SbVec2f(1.0f, 1.0f);
}
