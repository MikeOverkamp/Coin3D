/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 2001 by Systems in Motion. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation. See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin for applications not compatible with the
 *  LGPL, please contact SIM to acquire a Professional Edition license.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

/*!
  \class SoVRMLTransform SoVRMLTransform.h Inventor/VRMLnodes/SoVRMLTransform.h
  \brief The SoVRMLTransform class is a grouping node that defines a transformation for its children.

*/

/*!
  \var SoSFVec3f SoVRMLTransform::translation
  The translation vector. Default value is (0, 0, 0).
*/

/*!
  \var SoSFRotation SoVRMLTransform::rotation
  The rotation around the center point. Default value is null-rotation.
*/

/*!
  \var SoSFVec3f SoVRMLTransform::scale
  The scale vector about the center point. Default value is (1, 1, 1).
*/

/*!
  \var SoSFRotation SoVRMLTransform::scaleOrientation
  The scale orientation. Default value is a null-rotation.
*/

/*!
  \var SoSFVec3f SoVRMLTransform::center
  The center point. Default value is (0, 0, 0).
*/


#include <Inventor/VRMLnodes/SoVRMLTransform.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>

SO_NODE_SOURCE(SoVRMLTransform);

// Doc in parent
void
SoVRMLTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLTransform, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLTransform::SoVRMLTransform(void)
{
  this->commonConstructor();
}

/*!
  Constructor. \a numchildren is the expected number of children.
*/
SoVRMLTransform::SoVRMLTransform(int numchildren)
  : inherited(numchildren)

{
  this->commonConstructor();
}

void
SoVRMLTransform::commonConstructor(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoVRMLTransform);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(rotation, (SbRotation::identity()));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(scale, (1.0f, 1.0f, 1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(scaleOrientation, (SbRotation::identity()));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(center, (0.0f, 0.0f, 0.0f));
}

/*!
  Destructor
*/
SoVRMLTransform::~SoVRMLTransform()
{
}

/*!
  Sets the transformation to translate to \a frompoint, with a rotation
  so that the (0,0,-1) vector is rotated into the vector from \a frompoint
  to \a topoint.
*/
void
SoVRMLTransform::pointAt(const SbVec3f & from,
                         const SbVec3f & to)
{
  this->scale = SbVec3f(1.0f, 1.0f, 1.0f);
  this->center = SbVec3f(0.0f, 0.0f, 0.0f);
  this->scaleOrientation = SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f);
  
  this->translation = from;  
  SbVec3f dir = to - from;
  dir.normalize();
  
  SbRotation rot(SbVec3f(0.0f, 0.0f, -1.0f), dir);
  this->rotation = rot;
}

/*!
  Calculates the matrices to/from scale space.
*/
void
SoVRMLTransform::getScaleSpaceMatrix(SbMatrix & matrix,
                                     SbMatrix & inverse) const
{
  SbMatrix tmp;
  matrix.setTranslate(-center.getValue());
  tmp.setRotate(scaleOrientation.getValue().inverse());
  matrix.multRight(tmp);
  tmp.setScale(scale.getValue());
  matrix.multRight(tmp);
  inverse = matrix.inverse();
}

/*!
  Calculates the matrices to/from rotation space.
*/
void
SoVRMLTransform::getRotationSpaceMatrix(SbMatrix & matrix,
                                        SbMatrix & inverse) const
{
  SbMatrix tmp;
  matrix.setTranslate(-this->center.getValue());
  tmp.setRotate(this->scaleOrientation.getValue().inverse());
  matrix.multRight(tmp);
  tmp.setScale(this->scale.getValue());
  matrix.multRight(tmp);
  tmp.setRotate(this->scaleOrientation.getValue());
  matrix.multRight(tmp);
  tmp.setRotate(this->rotation.getValue());
  matrix.multRight(tmp);
  inverse = matrix.inverse();
}

/*!
  Calculates the matrices to/from translation space.
*/
void
SoVRMLTransform::getTranslationSpaceMatrix(SbMatrix & matrix,
                                           SbMatrix & inverse) const
{
  SbMatrix tmp;
  matrix.setTranslate(-this->center.getValue());
  tmp.setRotate(this->scaleOrientation.getValue().inverse());
  matrix.multRight(tmp);
  tmp.setScale(this->scale.getValue());
  matrix.multRight(tmp);
  tmp.setRotate(this->scaleOrientation.getValue());
  matrix.multRight(tmp);
  tmp.setRotate(this->rotation.getValue());
  matrix.multRight(tmp);
  tmp.setTranslate(this->translation.getValue());
  matrix.multRight(tmp);
  inverse = matrix.inverse();
}

/*!
  Premultiplies this transformation by \a mat.
*/
void
SoVRMLTransform::multLeft(const SbMatrix & matrix)
{
  SbMatrix tmp;
  tmp.setTransform(this->translation.getValue(),
                   this->rotation.getValue(),
                   this->scale.getValue(),
                   this->scaleOrientation.getValue(),
                   this->center.getValue());
  
  tmp.multLeft(matrix);
  this->setMatrix(tmp);
}

/*!
  Postmultiplies this transformation by \a mat.
*/
void
SoVRMLTransform::multRight(const SbMatrix & matrix)
{
  SbMatrix tmp;
  tmp.setTransform(this->translation.getValue(),
                   this->rotation.getValue(),
                   this->scale.getValue(),
                   this->scaleOrientation.getValue(),
                   this->center.getValue());
  tmp.multRight(matrix);
  this->setMatrix(tmp);
}

void
/*!
  Premultiplies this transformation by the transformation in \a leftnode.
*/
SoVRMLTransform::combineLeft(SoVRMLTransform * leftnode)
{
  SoGetMatrixAction ma(SbViewportRegion(100,100));
  ma.apply(leftnode);
  this->multLeft(ma.getMatrix());
}

/*!
  Postmultiplies this transformation by the transformation in \a rightnode.
*/
void
SoVRMLTransform::combineRight(SoVRMLTransform * rightnode)
{
  SoGetMatrixAction ma(SbViewportRegion(100,100));
  ma.apply(rightnode);
  this->multRight(ma.getMatrix());
}

/*!
  Sets the fields to create a transformation equal to \a mat.
*/
void
SoVRMLTransform::setMatrix(const SbMatrix & matrix)
{
  SbVec3f t, s, c = this->center.getValue();
  SbRotation r, so;
  matrix.getTransform(t,r,s,so,c);
  
  this->translation = t;
  this->rotation = r;
  this->scale = s;
  this->scaleOrientation = so;
}

/*!  
  Sets the \e center field to \a newcenter. This might affect one
  or more of the other fields.  
*/
void
SoVRMLTransform::recenter(const SbVec3f & newcenter)
{
  SbMatrix matrix;
  matrix.setTransform(this->translation.getValue(),
                      this->rotation.getValue(),
                      this->scale.getValue(),
                      this->scaleOrientation.getValue(),
                      this->center.getValue());
  SbVec3f t, s;
  SbRotation r, so;
  matrix.getTransform(t, r, s, so, newcenter);
  this->translation = t;
  this->rotation = r;
  this->scale = s;
  this->scaleOrientation = so;
  this->center = newcenter;
}

// Doc in parent
void
SoVRMLTransform::doAction(SoAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  SoGroup::doAction(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::callback(SoCallbackAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  SoGroup::callback(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  SoGroup::getBoundingBox(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::getMatrix(SoGetMatrixAction * action)
{
  int numindices;
  const int * indices;
  SbMatrix m;
  m.setTransform(this->translation.getValue(),
                 this->rotation.getValue(),
                 this->scale.getValue(),
                 this->scaleOrientation.getValue(),
                 this->center.getValue());
  action->getMatrix().multLeft(m);
  SbMatrix mi = m.inverse();
  action->getInverse().multRight(mi);

  SoGroup::getMatrix(action);
}

// Doc in parent
void
SoVRMLTransform::pick(SoPickAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  SoGroup::pick(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  SoGroup::getPrimitiveCount(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  this->applyMatrix(state);
  inherited::GLRenderBelowPath(action);
  state->pop();
}

// Doc in parent
void
SoVRMLTransform::GLRenderInPath(SoGLRenderAction * action)
{
  if (action->getCurPathCode() == SoAction::IN_PATH) {
    SoState * state = action->getState();
    state->push();
    this->applyMatrix(state);
    inherited::GLRenderInPath(action);
    state->pop();
  }
  else {
    // we got to the end of the path
    this->GLRenderBelowPath(action);
  }
}

// Doc in parent
void
SoVRMLTransform::notify(SoNotList * list)
{
  inherited::notify(list);
}

//
// applies transformation to state.
//
void
SoVRMLTransform::applyMatrix(SoState * state)
{
  SbMatrix matrix;
  matrix.setTransform(this->translation.getValue(),
                      this->rotation.getValue(),
                      this->scale.getValue(),
                      this->scaleOrientation.getValue(),
                      this->center.getValue());
  if (matrix != SbMatrix::identity()) {
    SoModelMatrixElement::mult(state, this, matrix);
  }
}
