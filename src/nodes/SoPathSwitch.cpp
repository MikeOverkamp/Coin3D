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
  \class SoPathSwitch SoPathSwitch.h Inventor/nodes/SoPathSwitch.h
  \brief The SoPathSwitch class traverses only when current path matches a configurable path.
  \ingroup nodes

*/


#include <Inventor/nodes/SoPathSwitch.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoSearchAction.h>


/*!
  \var SoSFPath SoPathSwitch::path

  The path that must match the current action path. A \c NULL path
  will never match. An empty path will always match. The path should
  go up to (not including) the SoPathSwitch node, but need not go all
  the way back to the root node.
*/

// *************************************************************************

SO_NODE_SOURCE(SoPathSwitch);

/*!
  Default constructor.
*/
SoPathSwitch::SoPathSwitch(void)
{
  this->commonConstructor();
}


static SbBool 
is_matching_paths(const SoPath * currentpath, const SoPath * pathswitchpath)
{
  if (pathswitchpath == NULL) return FALSE;

  const SoFullPath * current = (const SoFullPath*) currentpath;
  const SoFullPath * swpath = (const SoFullPath *) pathswitchpath;
  
  int swidx = swpath->getLength() - 1;
  if (swidx < 0) return TRUE; // an empty path will always match

  int curidx = current->getLength() - 2; // last node is this node. Skip it.
  
  // test if swpath is a valid path. Return FALSE if not.
  if (swidx > curidx) return FALSE;

  // we know curidx >= swidx
  while (swidx > 0) {
    if ((swpath->getNode(swidx) != current->getNode(curidx)) ||
        (swpath->getIndex(swidx) != current->getIndex(curidx))) break;
    swidx--;
    curidx--;
  }
  
  if (swidx == 0) { // don't test index for head node
    return swpath->getHead() == current->getNode(curidx);
  }
  return FALSE;
}


/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoPathSwitch::SoPathSwitch(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

// private
void
SoPathSwitch::commonConstructor(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPathSwitch);
  SO_NODE_ADD_FIELD(path, (NULL));
}

/*!
  Destructor.
*/
SoPathSwitch::~SoPathSwitch()
{
}

// doc in parent
void
SoPathSwitch::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPathSwitch);
}

// doc in parent
void
SoPathSwitch::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getBoundingBox(action);
  }
}

// doc in parent
void
SoPathSwitch::doAction(SoAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::doAction(action);
  }
}

// doc in parent
void
SoPathSwitch::GLRender(SoGLRenderAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::GLRender(action);
  }
}

// doc in parent
void
SoPathSwitch::pick(SoPickAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::pick(action);
  }
}

// doc in parent
void
SoPathSwitch::handleEvent(SoHandleEventAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::handleEvent(action);
  }
}

// doc in parent
void
SoPathSwitch::getMatrix(SoGetMatrixAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getMatrix(action);
  }
}

// doc in parent
void
SoPathSwitch::search(SoSearchAction * action)
{
  if (action->isSearchingAll()) inherited::search(action);
  else {
    SoNode::search(action);
    if (!action->isFound()) {
      SoPathSwitch::doAction(action);
    }
  }
}

// doc in parent
void
SoPathSwitch::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getPrimitiveCount(action);
  }
}
