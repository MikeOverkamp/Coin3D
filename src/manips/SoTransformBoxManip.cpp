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

/*!
  \class SoTransformBoxManip SoTransformBoxManip.h Inventor/manips/SoTransformBoxManip.h
  \brief The SoTransformBoxManip wraps an SoTransformBoxDragger for convenience.
  \ingroup manips

  The manipulator class takes care of wrapping up the
  SoTransformBoxDragger in a simple and convenient API for the
  application programmer, making it automatically surround the
  geometry it influences and taking care of the book-keeping routines
  for it's interaction with the relevant fields of an SoTransformation
  node.
*/

#include <Inventor/manips/SoTransformBoxManip.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoTransformBoxDragger.h>

SO_NODE_SOURCE(SoTransformBoxManip);


// doc in super
void
SoTransformBoxManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTransformBoxManip, SO_FROM_INVENTOR_2_1);
}

/*!
  Default constructor. Allocates an SoTransformBoxDragger and an
  SoSurroundScale node to surround the geometry within our part of the
  scenegraph.
*/
SoTransformBoxManip::SoTransformBoxManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransformBoxManip);

  SoTransformBoxDragger *dragger = new SoTransformBoxDragger;
  this->setDragger(dragger);

  SoSurroundScale *ss = (SoSurroundScale*) dragger->getPart("surroundScale", TRUE);
  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}

/*!
  Destructor.
*/
SoTransformBoxManip::~SoTransformBoxManip()
{
}
