/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2007 by Systems in Motion.  All rights reserved.
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
 *  See http://www.coin3d.org/ for more information.
 *
 *  Systems in Motion, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  http://www.sim.no/  sales@sim.no  coin-support@coin3d.org
 *
\**************************************************************************/

/*!
  \class SoGeoSeparator SoGeoSeparator.h Inventor/nodes/SoGeoSeparator.h
  \brief The SoGeoSeparator class is used to georeference a scene graph.
  \ingroup nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    GeoSeparator {
      geoSystem ["GD", "WE"]
      geoCoords ""
    }
  \endcode

  This node specifies an absolute geographic coordinate system for the
  children. When rendering (or applying other actions), Coin
  will add a transformation which transforms the geometry into the
  SoGeoOrigin coordinate system.

  \sa SoGeoOrigin
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/nodes/SoGeoSeparator.h>
#include <Inventor/nodes/SoGeoOrigin.h>
#include <Inventor/elements/SoGeoElement.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>

#include "SoGeo.h"

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

// *************************************************************************

/*!
  \var SoSFString SoGeoSeparator::geoCoords

  Used for specifying the geographic coordinates.

  \sa SoGeoOrigin::geoCoords
*/

/*!
  \var SoMFString SoGeoSeparator::geoSystem

  Used to specify a spatial reference frame.

  \sa SoGeoOrigin::geoSystem
*/


// *************************************************************************

SO_NODE_SOURCE(SoGeoSeparator);

/*!
  Constructor.
*/
SoGeoSeparator::SoGeoSeparator(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGeoSeparator);

  SO_NODE_ADD_FIELD(geoCoords, (0.0, 0.0, 0.0));
  SO_NODE_ADD_FIELD(geoSystem, (""));

  this->geoSystem.setNum(2);
  this->geoSystem.set1Value(0, "GD");
  this->geoSystem.set1Value(1, "WE");
  this->geoSystem.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoGeoSeparator::~SoGeoSeparator()
{
}

// Doc from superclass.
void
SoGeoSeparator::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGeoSeparator, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc from superclass.
void
SoGeoSeparator::applyTransformation(SoAction * action)
{
  SoState * state = action->getState();
  SbMatrix m = this->getTransform(state);

  SoModelMatrixElement::set(state, this, m);
}

// Doc from superclass.
void
SoGeoSeparator::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyTransformation((SoAction*) action);
  SoSeparator::GLRenderBelowPath(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::GLRenderInPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyTransformation((SoAction*) action);
  SoSeparator::GLRenderInPath(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();
  state->push();
  SbMatrix m = this->getTransform(state);

  SoModelMatrixElement::mult(state,
                             this,
                             SoModelMatrixElement::get(state).inverse());
  SoModelMatrixElement::mult(state,
                             this,
                             m);

  SoSeparator::getBoundingBox(action);
  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix m = this->getTransform(action->getState());
  action->getMatrix() = m;
  action->getInverse() = m.inverse();
}

// Doc from superclass.
void
SoGeoSeparator::callback(SoCallbackAction * action)
{
  SoState * state = action->getState();
  state->push();

  this->applyTransformation((SoAction *)action);
  SoSeparator::callback(action);

  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::pick(SoPickAction * action)
{
  SoState * state = action->getState();
  state->push();

  this->applyTransformation((SoAction *)action);
  SoSeparator::pick(action);

  state->pop();
}

// Doc from superclass.
void
SoGeoSeparator::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoState * state = action->getState();
  state->push();

  this->applyTransformation((SoAction *)action);
  SoSeparator::getPrimitiveCount(action);

  state->pop();
}

// *************************************************************************

SbMatrix
SoGeoSeparator::getTransform(SoState * state) const
{
  SoGeoOrigin * origin = SoGeoElement::get(state);

  if (origin) {
    return SoGeo::calculateTransform(origin->geoSystem.getValues(0),
                                     origin->geoSystem.getNum(),
                                     origin->geoCoords.getValue(),

                                     this->geoSystem.getValues(0),
                                     this->geoSystem.getNum(),
                                     this->geoCoords.getValue());
  }
  
  SoDebugError::post("SoGeoSeparator::getTransform",
                     "No SoGeoOrigin node found on stack.");
  return SbMatrix::identity();
}
